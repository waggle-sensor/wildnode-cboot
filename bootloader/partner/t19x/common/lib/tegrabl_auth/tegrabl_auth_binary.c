/*
 * Copyright (c) 2018-2020, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_AUTH

#if defined(CONFIG_ENABLE_SECURE_BOOT)

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <tegrabl_utils.h>
#include <tegrabl_debug.h>
#include <tegrabl_addressmap.h>
#include <tegrabl_drf.h>
#include <tegrabl_io.h>
#include <tegrabl_fuse.h>
#include <arfuse.h>
#include <tegrabl_fuse_bitmap.h>
#include <nvboot_boot_component.h>
#include <tegrabl_crypto_se.h>
#include <tegrabl_partition_loader.h>
#include <tegrabl_malloc.h>

#define CRYPTO_HEADER_SIZE sizeof(NvBootComponentHeader)
#define MIN_BINARY_SIZE 1024U
#define SBK_KEY_SIZE_BYTES 16U
#define RSA_KEY_2048_BITS 2048U
#define RSA_KEY_3072_BITS 3072U

/**
 * @brief keyslots
 */
#define RSA_KEYSLOT_0 0U
#define RSA_KEYSLOT_1 1U
#define AES_KEYSLOT_SBK 14U

#define STAGE2_SIGN_OFFSET offsetof(NvBootComponentHeader, Salt2)
#define STAGE2_SIGN_SIZE (offsetof(NvBootComponentHeader, Stage1Components) - \
	STAGE2_SIGN_OFFSET + (sizeof(NvStage1Component) * MAX_COMPONENT_COUNT))
#define STAGE1_SIGN_OFFSET offsetof(NvBootComponentHeader, Salt1)
#define STAGE1_SIGN_SIZE (offsetof(NvBootComponentHeader, Stage1Components) - \
	STAGE1_SIGN_OFFSET + (sizeof(NvStage1Component) * MAX_COMPONENT_COUNT))

#define SHA2_DIGEST_BYTES 32U
#define SALT_SIZE 16U

#define FUSE_AUTHENTICATION_SCHEME_MASK 0x83U
#define FUSE_ENCRYPTION_SCHEME_MASK 0x4U
#define AUTHENTICATION_SCHEME_SHA2 0x0U
#define AUTHENTICATION_SCHEME_RSA2K 0x1U
#define AUTHENTICATION_SCHEME_RSA3K 0x2U
#define AUTHENTICATION_SCHEME_ECC 0x3U
#define AUTHENTICATION_SCHEME_ED25519 0x83U
#define ECC_SCHEME_P256 0U
#define ECC_SCHEME_ED25519 1U
#define SE_AES_BLOCK_LENGTH 16U

static bool check_if_keyslot_is_zero(uint8_t keyslot)
{
	static uint8_t sample_text[SE_AES_BLOCK_LENGTH] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};
	static uint8_t cipher_test[SE_AES_BLOCK_LENGTH] = {
		0x7A, 0xCA, 0x0F, 0xD9, 0xBC, 0xD6, 0xEC, 0x7C,
		0x9F, 0x97, 0x46, 0x66, 0x16, 0xE6, 0xA2, 0x82
	};
	bool iszero = false;
	tegrabl_error_t error = TEGRABL_NO_ERROR;
	uint8_t input_data[SE_AES_BLOCK_LENGTH];
	uint8_t input_iv[SE_AES_BLOCK_LENGTH];
	uint32_t i;

	if (keyslot > 15U) {
		pr_error("Invalid Key slot\n");
		error = TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0U);
		goto fail;
	}

	memcpy(input_data , cipher_test, SE_AES_BLOCK_LENGTH);
	memset(input_iv , 0, SE_AES_BLOCK_LENGTH);
	error = tegrabl_crypto_decrypt_buffer(input_data, SE_AES_BLOCK_LENGTH,
						input_data, keyslot, SE_AES_BLOCK_LENGTH, input_iv);
	if (error != TEGRABL_NO_ERROR) {
		pr_error("Failed to decrypt input data\n");
		TEGRABL_SET_HIGHEST_MODULE(error);
		goto fail;
	}

	pr_trace("decrypted data is \n");
	for (i = 0; i <  SE_AES_BLOCK_LENGTH; i++) {
		pr_trace("%0x ", input_data[i]);
	}
	pr_trace("\n");

	/* compare if the decrypted data is same as that of sample text;
	 * if they are the same, keyslot is zero.
	 */
	if (!memcmp(input_data, sample_text, SE_AES_BLOCK_LENGTH)) {
		iszero = true;
	}

fail:
	return iszero;
}

static tegrabl_error_t authenticate_oem_header(void *payload)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t val;
	uint32_t authentication_scheme;
	uint8_t *hash = NULL;
	uint8_t temp_mem[sizeof(NvBootComponentHeader)];
	uint8_t temp_pcp[sizeof(NvBootPublicCryptoParameters)];
	NvBootPublicCryptoParameters *pcp;

	NvBootComponentHeader *header = (NvBootComponentHeader *)payload;

	hash = tegrabl_alloc(TEGRABL_HEAP_DMA, SHA2_DIGEST_BYTES);
	if (hash == NULL) {
		pr_error("Unable to allocate memory\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}

	memcpy(temp_mem, (uint8_t *)((uint8_t *)header + STAGE2_SIGN_OFFSET),
					STAGE2_SIGN_SIZE);

	memcpy(temp_pcp, (uint8_t *)&header->Pcp,
			sizeof(NvBootPublicCryptoParameters));
	pcp = (NvBootPublicCryptoParameters *)temp_pcp;

	val = REG_READ(FUSE, FUSE_BOOT_SECURITY_INFO);
	authentication_scheme = val & FUSE_AUTHENTICATION_SCHEME_MASK;

	if (authentication_scheme != AUTHENTICATION_SCHEME_SHA2) {
		err = tegrabl_crypto_validate_pcp(pcp);
		if (err != TEGRABL_NO_ERROR) {
			goto fail;
		}
	}

	switch (authentication_scheme) {
	case AUTHENTICATION_SCHEME_SHA2:
		err = tegrabl_crypto_compute_sha2(temp_mem,
			STAGE2_SIGN_SIZE, hash);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("SHA2 failed!! err = %d\n", err);
			goto fail;
		}

		if (memcmp(hash, header->Stage2Signature.Digest,
				SHA2_DIGEST_BYTES) != 0) {
			pr_error("Stage2Signature validation failed with SHA2!!\n");
			err = TEGRABL_ERROR(TEGRABL_ERR_VERIFY_FAILED, 0);
			goto fail;
		}
		pr_debug("Stage2Signature validation success with SHA2!!\n");
		break;

	case AUTHENTICATION_SCHEME_RSA2K:
		/* 2048 bit RSA */
		err = tegrabl_crypto_rsa_pss_verify(
			temp_mem,
			STAGE2_SIGN_SIZE,
			(uint8_t *)&header->Stage2Signature.RsaSsaPssSig,
			RSA_KEYSLOT_0, RSA_KEY_2048_BITS,
			(uint8_t *)&pcp->RsaPublicParams, NULL, NULL, true);
		break;

	case AUTHENTICATION_SCHEME_RSA3K:
		/* 3072 bit RSA */
		err = tegrabl_crypto_rsa_pss_verify(
			temp_mem,
			STAGE2_SIGN_SIZE,
			(uint8_t *)&header->Stage2Signature.RsaSsaPssSig,
			RSA_KEYSLOT_0, RSA_KEY_3072_BITS,
			(uint8_t *)&pcp->RsaPublicParams.Modulus,
			(uint8_t *)&pcp->RsaPublicParams.Mprime,
			(uint8_t *)&pcp->RsaPublicParams.Rsquare,
			true);
		break;

	case AUTHENTICATION_SCHEME_ECC:
		err = tegrabl_crypto_ecdsa_verify(
			temp_mem,
			STAGE2_SIGN_SIZE,
			(uint8_t *)&header->Stage2Signature.EccSig.EcdsaSig,
			EccPrimeFieldKeySizeBytes256,
			(uint8_t *)&pcp->EccPublicParams.EccPublicKey);
		break;
	case AUTHENTICATION_SCHEME_ED25519:
		err = tegrabl_crypto_ed25519_verify(
			temp_mem,
			STAGE2_SIGN_SIZE,
			(uint8_t *)&header->Stage2Signature.EccSig.EdDsaSig,
			ED25519_POINT_SIZE_BYTES * 2,
			(uint8_t *)&pcp->EccPublicParams);
		break;
	default:
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
		break;
	}
fail:
	if (hash != NULL) {
		tegrabl_dealloc(TEGRABL_HEAP_DMA, hash);
	}

	return err;
}

static tegrabl_error_t authenticate_oem_payload(void *payload)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint8_t *hash = NULL;
	uint32_t val;
	uint32_t encryption_scheme;
	uint8_t *src;
	uint32_t total_len, len;
	uint32_t chunk = 0x800000;

	NvBootComponentHeader *header = (NvBootComponentHeader *)payload;

	/* Compute SHA256 on binary */
	hash = tegrabl_alloc(TEGRABL_HEAP_DMA, SHA2_DIGEST_BYTES);
	if (hash == NULL) {
		pr_error("failed to allocate memory!!\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}

	err = tegrabl_crypto_compute_sha2((uint8_t *)header + sizeof(*header),
			header->Stage2Components[0].BinaryLen, hash);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("SHA2 failed!! err = %d\n", err);
		goto fail;
	}

	/* Compare computed SHA256 against the digest */
	if (memcmp(hash, header->Stage2Components[0].Digest,
			SHA2_DIGEST_BYTES) != 0) {
		pr_error("digest on binary did not match!!\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_VERIFY_FAILED, 0);
		goto fail;
	}

	/*
	 * Decrypt the binary if encryption is enabled in BOOT_SECURITY_INFO fuse, and
	 * keyslot SBK is not zero.
	 */
	val = REG_READ(FUSE, FUSE_BOOT_SECURITY_INFO);
	encryption_scheme = val & FUSE_ENCRYPTION_SCHEME_MASK;
	if (encryption_scheme != FUSE_ENCRYPTION_SCHEME_MASK) {
		pr_info("Encryption fuse is not ON\n");
		goto fail;
	}

	if (check_if_keyslot_is_zero(AES_KEYSLOT_SBK)) {
		pr_warn("keyslot %d is zero\n", AES_KEYSLOT_SBK);
		goto fail;
	}

	/* Decrypt the binary */
	pr_info("%s: Decrypt the binary\n", __func__);
	src = (uint8_t *)header + sizeof(*header);
	total_len = header->Stage2Components[0].BinaryLen;
	while (total_len) {
		len = (total_len > chunk) ? chunk : total_len;
		err = tegrabl_crypto_decrypt_buffer(src, len, src,
							AES_KEYSLOT_SBK, SBK_KEY_SIZE_BYTES,
							header->Salt2);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Binary decryption failed\n");
			goto fail;
		}
		src += len;
		total_len -= len;
	}

fail:
	if (hash != NULL) {
		tegrabl_dealloc(TEGRABL_HEAP_DMA, hash);
	}

	return err;
}

tegrabl_error_t tegrabl_auth_payload(tegrabl_binary_type_t bin_type,
		char *name, void *payload, uint32_t max_size)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	pr_info("T19x: Authenticate %s (bin_type: %u), max size 0x%x\n", name,
			bin_type, max_size);

    tegrabl_crypto_early_init();

	/* Authenticate OEM signed portion of header */
	err = authenticate_oem_header(payload);
	if (err != TEGRABL_NO_ERROR) {
		pr_critical("OEM authentication of %s header failed!\n", name);

		goto fail;
	}

	/* Authenticate OEM signed payload */
	err = authenticate_oem_payload(payload);
	if (err != TEGRABL_NO_ERROR) {
		pr_critical("OEM authentication of %s payload failed!\n", name);
		goto fail;
	}

	/*
	 * Make sure "load_address" pointing to real payload
	 *
	 * !!! test show it does not work if simply setting
	 * binary->load_address += sizeof(NvBootComponentHeader);
	 */
	memmove((uint8_t *)payload,
			(uint8_t *)payload + sizeof(NvBootComponentHeader),
			(uint32_t)max_size - sizeof(NvBootComponentHeader));
fail:
	return err;
}

uint32_t tegrabl_sigheader_size(void)
{
	return CRYPTO_HEADER_SIZE;
}

uint32_t tegrabl_auth_get_binary_len(void *bin_load_addr)
{
	uint32_t bin_len;
	NvBootComponentHeader *header = (NvBootComponentHeader *)bin_load_addr;

	if (strncmp(bin_load_addr, "NVDA", 4) == 0) {
		bin_len = header->Stage2Components[0].BinaryLen;
		pr_trace("Binary len: %u (0x%08x)\n", bin_len, bin_len);
	} else {
		pr_trace("Header magic mismatch (0x%02x 0x%02x 0x%02x 0x%02x) or header not present\n",
				 header->HeaderMagic[0],
				 header->HeaderMagic[1],
				 header->HeaderMagic[2],
				 header->HeaderMagic[3]);
		bin_len = 0;
	}

	return bin_len;
}

/* Last step: clear the keyslot */
tegrabl_error_t tegrabl_auth_complete(void)
{
	tegrabl_error_t err;

	err = tegrabl_se_clear_device_aes_keyslot(AES_KEYSLOT_SBK);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("clear keyslot %d returns error=0x%x\n", AES_KEYSLOT_SBK, err);
	}

	return err;
}

#endif /* defined(CONFIG_ENABLE_SECURE_BOOT) */
