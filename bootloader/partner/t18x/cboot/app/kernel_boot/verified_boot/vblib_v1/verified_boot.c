/*
 * Copyright (c) 2015-2018, NVIDIA Corporation.	All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_VERIFIED_BOOT

#include <tegrabl_error.h>
#include <verified_boot.h>
#include <signature_parser.h>
#include <mincrypt/rsa.h>
#include <mbedtls/bignum.h>
#include <debug.h>
#include <err.h>
#include <sm_err.h>
#include <malloc.h>
#include <sys/types.h>
#include <mincrypt/sha256.h>
#include <tegrabl_bootimg.h>
#include <arse0.h>
#include <string.h>
#include <tegrabl_debug.h>
#include <tegrabl_odmdata_soc.h>
#include <tegrabl_pkc_ops.h>
#include <tegrabl_wdt.h>
#include <tegrabl_cache.h>
#include <tegrabl_fuse.h>
#include <libfdt.h>
#include <nvboot_crypto_param.h>
#include <verified_boot_ui.h>

#if defined(IS_T186)
#include <tegrabl_se.h>
#else
#include <tegrabl_crypto_se.h>
#endif

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define HASH_SZ 32
#define SHA_INPUT_BLOCK_SZ (8 * 1024 * 1024)

/* r = 2^4096, big endian, hard coded*/
static uint8_t _r[513] = {0x01};

static status_t hash_payload_authattr(uintptr_t payload, size_t payload_size,
									  uintptr_t authaddr, size_t auth_size,
									  uint8_t *output)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	uint8_t *buf_payload_auth = NULL;
	size_t size;
#if defined(IS_T186)
	struct se_sha_input_params sha_input;
	struct se_sha_context sha_context;
#endif

	if (!payload || !authaddr || !output)
		return ERR_INVALID_ARGS;

	size = payload_size + auth_size;
	buf_payload_auth = malloc(size);
	if (buf_payload_auth == NULL) {
		pr_error("Failed to allocate memory for payload and authattr\n");
		return ERR_NO_MEMORY;
	}

	memcpy((void *)buf_payload_auth, (const void *)payload, payload_size);
	memcpy((void *)(buf_payload_auth + payload_size), (const void *)authaddr,
		   auth_size);

#if defined(IS_T186)
	/* TODO:
	 * Calculate the hash in one go when SE bug is fixed
	 * Tracked in Bug 200199108
	 */
	sha_context.input_size = (uint32_t)size;
	sha_context.hash_algorithm = SE_SHAMODE_SHA256;
	sha_input.block_addr = (uintptr_t)buf_payload_auth;
	sha_input.block_size = (uint32_t)size;
	sha_input.size_left = (uint32_t)size;
	sha_input.hash_addr = (uintptr_t)output;

	ret = tegrabl_se_sha_process_payload(&sha_input, &sha_context);
#else
	ret = tegrabl_crypto_compute_sha2(buf_payload_auth, size, output);
#endif
	if (ret != TEGRABL_NO_ERROR)
		return ERR_GENERIC;

	free(buf_payload_auth);
	return NO_ERROR;
}

static status_t fill_rsa_publickey(struct rsa_public_key *key)
{
	mbedtls_mpi modulus, mask_32bits , n0inv, n0inv_t;
	mbedtls_mpi r, rr;
	int ret, i;
	unsigned char pkey[VERITY_KEY_SIZE];

	if (key == NULL)
		return ERR_INVALID_ARGS;

	/* Convert little endian modulus to big endian*/
	for (i = 0; i < VERITY_KEY_SIZE; i++)
		pkey[i] = ((uint8_t *)(key->n))[VERITY_KEY_SIZE - 1 - i];

	mbedtls_mpi_init(&mask_32bits);
	mbedtls_mpi_init(&modulus);
	mbedtls_mpi_init(&n0inv_t);
	mbedtls_mpi_init(&n0inv);
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&rr);

	/* mask = 2^32*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&mask_32bits, 16, "100000000"));
	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&modulus, pkey, VERITY_KEY_SIZE));
	/* Figure out n0inv*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_inv_mod(&n0inv_t, &modulus, &mask_32bits));
	MBEDTLS_MPI_CHK(mbedtls_mpi_sub_abs(&n0inv, &mask_32bits, &n0inv_t));
	memcpy(&(key->n0inv), (uint8_t *)n0inv.p, sizeof(uint32_t));
	/* Figure out rr*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&r, _r, sizeof(_r)));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&rr, &r, &modulus));
	memcpy((uint8_t *)(key->rr), (uint8_t *)rr.p, VERITY_KEY_SIZE);

cleanup:
	mbedtls_mpi_free(&mask_32bits);
	mbedtls_mpi_free(&modulus);
	mbedtls_mpi_free(&n0inv_t);
	mbedtls_mpi_free(&n0inv);
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&rr);

	if (ret != 0) {
		pr_info("An error occured in %s.\n", __func__);
		return ERR_GENERIC;
	}

	return NO_ERROR;
}

static status_t get_pub_key_from_image(uint8_t *vb_sign_der,
									   struct rsa_public_key *pub_key)
{
	/* Steps:
	 * Extract signature
	 * Extract certificate
	 * Get modulus*/
	uint8_t *cert_der;
	uint8_t *modulus;
	int i;

	if (get_cert(vb_sign_der, &cert_der) != NO_ERROR)
		return ERR_INVALID_ARGS;

	if (get_pubkey(cert_der, &modulus, VERITY_KEY_SIZE) != NO_ERROR)
		return ERR_INVALID_ARGS;

	/* Convert public modulus from big endian to little endian*/
	for (i = 0; i < VERITY_KEY_SIZE; i++)
		((uint8_t *)pub_key->n)[i] = modulus[VERITY_KEY_SIZE - 1 - i];

	/* Fill other parts of public key*/
	if (fill_rsa_publickey(pub_key) != NO_ERROR)
		return ERR_INVALID_ARGS;

	return NO_ERROR;
}

/* Compare the keys k1 and k2. They are both expected to be in little endian
 * format
 */
static inline bool are_keys_identical(uint8_t *k1, uint8_t *k2)
{
	return !memcmp(k1, k2, VERITY_KEY_SIZE);
}

/* Refer $TOP/system/core/mkbootimg/bootimg.h for android boot image layout */
static inline uint32_t total_boot_pages(union tegrabl_bootimg_header *hdr)
{
	uint32_t kernel_pages, ramdisk_pages, second_pages, header_pages;

	header_pages = 1;
	kernel_pages = DIV_ROUND_UP(hdr->kernelsize, hdr->pagesize);
	ramdisk_pages = DIV_ROUND_UP(hdr->ramdisksize, hdr->pagesize);
	second_pages = DIV_ROUND_UP(hdr->secondsize, hdr->pagesize);

	return header_pages + kernel_pages + ramdisk_pages + second_pages;
}

static status_t __verify_signature(struct rsa_public_key *pubkey,
								   uintptr_t payload, size_t payload_size,
								   uint8_t *sig_section)
{
	uint8_t *signature;
	size_t sig_len;

	uint8_t *auth_attr;
	size_t auth_attr_len;

	uint8_t hash[HASH_SZ];
	status_t ret = NO_ERROR;

	/* Read authenticated attribute from signature section */
	ret = get_auth_attr(sig_section, &auth_attr, &auth_attr_len);
	if (ret != NO_ERROR) {
		pr_info("Can't get auth_attr from signature section\n");
		return ERR_NOT_VALID;
	}

    /* Calculate hash digest */
	ret = hash_payload_authattr((uintptr_t)payload,
								payload_size, (uintptr_t)auth_attr,
								auth_attr_len, hash);
	if (ret != NO_ERROR) {
		pr_info("Failed to hash vb payload and auth_attr\n");
		return ret; /* This should fail boot */
	}

	/* Get boot.img signature */
	ret = get_vb_signature(sig_section, &signature, &sig_len);
	if (ret != NO_ERROR) {
		pr_info("Failed to get signature DER\n");
		return ERR_NOT_VALID;
	}

	/* Read user public key from certificate */
	ret = get_pub_key_from_image(sig_section, pubkey);
	if (ret != NO_ERROR) {
		pr_info("Failed to get public key from signature\n");
		return ERR_NOT_VALID;
	}

	/* Verify boot.img signature with public key. If fails, go to red state*/
	if (!rsa_verify(pubkey, signature, sig_len, hash, HASH_SZ))
		return ERR_NOT_VALID;

	return NO_ERROR;
}

static tegrabl_error_t validate_pkc_modulus_hash(uint8_t *bct_key_mod)
{
	tegrabl_error_t ret = TEGRABL_NO_ERROR;
	uint8_t key_hash[HASH_SZ];
	uint8_t fuse_hash[HASH_SZ];
	NvBootPublicCryptoParameters pcp;

	TEGRABL_ASSERT(bct_key_mod != NULL);

	/* PKC hash fuse value is sha256sum of pcp */
	memset(&pcp, 0, sizeof(NvBootPublicCryptoParameters));
#if defined(IS_T186)
	memcpy(&pcp.RsaPublicParams.RsaPublicKey.RsaKey2048NvU8.Modulus,
		   bct_key_mod,
		   sizeof(pcp.RsaPublicParams.RsaPublicKey.RsaKey2048NvU8.Modulus));
#else
	memcpy(&pcp.RsaPublicParams.Modulus,
		   bct_key_mod,
		   sizeof(pcp.RsaPublicParams.Modulus));
#endif
	sha256_hash(&pcp, sizeof(NvBootPublicCryptoParameters), key_hash);

	ret = tegrabl_fuse_read(FUSE_PKC_PUBKEY_HASH, (uint32_t *)fuse_hash,
							HASH_SZ);
	if (ret != TEGRABL_NO_ERROR) {
		pr_error("Failed to read PKC hash from fuse\n");
		goto done;
	}

	/* Check if bct key modulus hash match with that in fuse */
	if (memcmp(key_hash, fuse_hash, HASH_SZ) != 0) {
		pr_error("BCT public key hash does not match with PKC hash fuse\n");
		ret = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);
		goto done;
	}

done:
	return ret;
}

static status_t verify_image(uintptr_t image_addr, size_t image_size,
							 uintptr_t sig_section, boot_state_t *bs,
							 struct rsa_public_key *out_pub_key)
{
	status_t ret = NO_ERROR;
	uint8_t bct_key_mod[VERITY_KEY_SIZE];
	struct rsa_public_key pub_key = {
		.len = VERITY_KEY_SIZE / sizeof(uint32_t),
		.exponent = 65537,
		.n0inv = 0,
		.n = {0},
		.rr = {0},
	};

	/* Start with RED STATE */
	*bs = VERIFIED_BOOT_RED_STATE;

	/* Verify boot image with key embedded in the signature section */
	ret = __verify_signature(&pub_key, image_addr, image_size,
							 (uint8_t *)sig_section);
	if (ret != NO_ERROR) {
		if (ret != ERR_NOT_VALID) { /* Fail on error other than ERR_NOT_VALID */
			pr_error("Critical failure when verifying image\n");
			return ret;
		}
		pr_error("Failed to verify image with embedded key\n");
		*bs = VERIFIED_BOOT_RED_STATE;
		ret = NO_ERROR; /* Boot in red state */
		goto out;
	}

	/* We should at least boot to yellow state from this point.
	 * So do not return an error code from here on
	 */

	/* Get public key from BCT */
	ret = tegrabl_pkc_modulus_get(bct_key_mod);
	if (ret != TEGRABL_NO_ERROR) {
		pr_error("Failed to get oem key from BCT\n");
		*bs = VERIFIED_BOOT_YELLOW_STATE;
		ret = NO_ERROR;
		goto out;
	}

	/* If the keys from boot image and BCT match, boot to green state, else
	 * boot to yellow state
	 */
	if (validate_pkc_modulus_hash(bct_key_mod) != TEGRABL_NO_ERROR ||
		!are_keys_identical((uint8_t *)pub_key.n, bct_key_mod)) {
		pr_info("Public key in signature is different from BCT key\n");
		*bs = VERIFIED_BOOT_YELLOW_STATE;
		ret = NO_ERROR;
		goto out;
	} else {
		pr_info("Public key in signature is identical with BCT key\n");
		*bs = VERIFIED_BOOT_GREEN_STATE; /* Everything went as expected */
		ret = NO_ERROR;
		goto out;
	}

out:
	if (!out_pub_key)
		return ret;

	/* Populate out_pub_key based on the boot state */
	switch (*bs) {
	case VERIFIED_BOOT_GREEN_STATE:
	case VERIFIED_BOOT_YELLOW_STATE:
		memcpy(out_pub_key, &pub_key, sizeof(pub_key));
		break;
	case VERIFIED_BOOT_RED_STATE:
	default:
		memset(out_pub_key, 0x00, sizeof(pub_key));
		break;
	}

	return ret;
}

static bool s_boot_image_verified;
static boot_state_t s_boot_state; /* Possible values: red/yellow/green */

status_t verified_boot_get_boot_state(union tegrabl_bootimg_header *hdr,
									  void *kernel_dtb, boot_state_t *bs,
									  struct rsa_public_key *boot_pub_key,
									  struct rsa_public_key *dtb_pub_key)
{
	status_t ret = NO_ERROR;
	boot_state_t bs_dtb, bs_boot;
	size_t dtb_size;
	size_t boot_size;
	uint8_t *dtb_sig_section;  /* Pointer to signature section of kernel-dtb */
	uint8_t *boot_sig_section; /* Pointer to signature section of boot img */

	if (!(tegrabl_odmdata_get() & (1 << TEGRA_BOOTLOADER_LOCK_BIT))) {
		*bs = VERIFIED_BOOT_ORANGE_STATE; /* Device is unlocked */
		return NO_ERROR;
	} else if (s_boot_image_verified) { /* Return early if already verified */
		*bs = s_boot_state;
		return NO_ERROR;
	} else {
#ifndef IS_T186
		tegrabl_crypto_early_init();
#endif

		/* Get boot.img boot state */
		/* Size of boot image including signature section */
		boot_size = total_boot_pages(hdr) * hdr->pagesize;
		boot_sig_section = (uint8_t *)hdr + boot_size;
		pr_info("Verifying boot: boot.img\n");
		ret = verify_image((uintptr_t)hdr, boot_size,
						   (uintptr_t)boot_sig_section, &bs_boot, boot_pub_key);
		if (ret != NO_ERROR)
			return ret;

		/* Get kernel-dtb boot state */
		dtb_size = fdt_totalsize(kernel_dtb);
		dtb_sig_section = (uint8_t *)(kernel_dtb) + dtb_size;
		pr_info("Verifying boot: kernel-dtb\n");
		ret = verify_image((uintptr_t)kernel_dtb, dtb_size,
						   (uintptr_t)dtb_sig_section, &bs_dtb, dtb_pub_key);
		if (ret != NO_ERROR)
			return ret;

		/* Verified boot states Red, Yellow and Green are arranged in increasing
		 * order of level of trust (Red < Yellow < Green), hence set the overall
		 * state to the minimum of the values */
		*bs = MIN(bs_boot, bs_dtb);

		s_boot_state = *bs;			  /* Cache the boot state */
		s_boot_image_verified = true; /* Mark the boot state cached */
		return NO_ERROR;
	}
}

status_t verified_boot_ui(boot_state_t bs,
						  struct rsa_public_key *boot_pub_key,
						  struct rsa_public_key *dtb_pub_key)
{
	switch (bs) {
	case VERIFIED_BOOT_RED_STATE:
		return verified_boot_red_state_ui();
	case VERIFIED_BOOT_YELLOW_STATE:
		return verified_boot_yellow_state_ui(boot_pub_key, dtb_pub_key);
	case VERIFIED_BOOT_ORANGE_STATE:
		return verified_boot_orange_state_ui();
	default:
		return ERR_INVALID_ARGS;
	}
}

tegrabl_error_t verify_boot(union tegrabl_bootimg_header *hdr,
							void *kernel_dtb, void *kernel_dtbo)
{
	status_t ret = NO_ERROR;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	boot_state_t bs;
	struct rsa_public_key *vb_pub_key_boot = NULL;
	struct rsa_public_key *vb_pub_key_dtb = NULL;
	struct root_of_trust r_o_t_params;

	/* DTBO is not enabled in verified boot 1.0 in Android N */
	TEGRABL_UNUSED(kernel_dtbo);

	vb_pub_key_boot = calloc(1, sizeof(struct rsa_public_key));
	if (!vb_pub_key_boot) {
		pr_error("No memory to hold verified boot public key\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto exit;
	}
	vb_pub_key_dtb = calloc(1, sizeof(struct rsa_public_key));
	if (!vb_pub_key_dtb) {
		pr_error("No memory to hold verified boot public key\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto exit;
	}

	ret = verified_boot_get_boot_state(hdr, kernel_dtb, &bs, vb_pub_key_boot,
									   vb_pub_key_dtb);
	if (ret != NO_ERROR)
		panic("An error occured in verified boot module.\n");

	r_o_t_params.magic_header = MAGIC_HEADER;
	r_o_t_params.version = VERSION;

	memcpy(r_o_t_params.boot_pub_key, vb_pub_key_boot->n,
		   sizeof(r_o_t_params.boot_pub_key));
	memcpy(r_o_t_params.dtb_pub_key, vb_pub_key_dtb->n,
		   sizeof(r_o_t_params.dtb_pub_key));
	r_o_t_params.verified_boot_state = bs;

	/* Flush these bytes so that the monitor can access them */
	tegrabl_arch_clean_dcache_range((addr_t)&r_o_t_params,
									sizeof(struct root_of_trust));

	/* Passing the address directly works because VMEM=PMEM in Cboot.
	 * If this assumption changes, we need to explicitly convert the
	 * virtual adddress to a physical address prior to the smc_call.
	 * Re-attempt the smc if it receives SM_ERR_INTERRUPTED.
	 */
	ret = smc_call(SMC_SET_ROOT_OF_TRUST, (uintptr_t)&r_o_t_params,
				   sizeof(struct root_of_trust));
	while (ret == SM_ERR_INTERRUPTED) {
		pr_info("SMC Call Interrupted, retry with smc # %x\n",
				SMC_TOS_RESTART_LAST);
		ret = smc_call(SMC_TOS_RESTART_LAST, 0, 0);
	}

	if (ret != NO_ERROR) {
		pr_error("Failed to pass verified boot params: %x\n", ret);
		err = TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
		goto exit;
	}

	pr_info("Verified boot state = %s\n",
			bs == VERIFIED_BOOT_RED_STATE ? "Red" :
			bs == VERIFIED_BOOT_YELLOW_STATE ? "Yellow" :
			bs == VERIFIED_BOOT_GREEN_STATE ? "Green" :
			bs == VERIFIED_BOOT_ORANGE_STATE ? "Orange" : "Unknown");

	if (bs != VERIFIED_BOOT_GREEN_STATE)
		verified_boot_ui(bs, vb_pub_key_boot, vb_pub_key_dtb);

exit:
	if (vb_pub_key_dtb)
		free(vb_pub_key_dtb);
	if (vb_pub_key_boot)
		free(vb_pub_key_boot);
	return err;
}
