/*
 * Copyright (c) 2018-2019, NVIDIA Corporation.  All rights reserved.
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

#if defined(CONFIG_ENABLE_ENCRYPTION)
	uint32_t val;
	uint32_t encryption_scheme;

	/* Decrypt the binary using SBK key if encryption is enabled in
	 * BOOT_SECURITY_INFO fuse
	 */
	val = REG_READ(FUSE, FUSE_BOOT_SECURITY_INFO);
	encryption_scheme = val & FUSE_ENCRYPTION_SCHEME_MASK;

	if (encryption_scheme == FUSE_ENCRYPTION_SCHEME_MASK) {
		/* Decrypt the binary */
		err = tegrabl_crypto_decrypt_buffer((uint8_t *)header + sizeof(*header),
				header->Stage2Components[0].BinaryLen,
				(uint8_t *)header + sizeof(*header),
				AES_KEYSLOT_SBK, SBK_KEY_SIZE_BYTES,
				header->Salt2);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Binary decryption failed\n");
			goto fail;
		}
	}
#endif /* defined(CONFIG_ENABLE_ENCRYPTION) */

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
#endif /* defined(CONFIG_ENABLE_SECURE_BOOT) */
