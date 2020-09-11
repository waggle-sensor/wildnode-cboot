/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef TEGRABL_CRYPTO_SE_H
#define TEGRABL_CRYPTO_SE_H

#include <stddef.h>
#include <stdint.h>
#include <tegrabl_error.h>
#include <nvboot_boot_component.h>

/**
 * @brief intialize common crypto library
 */
void tegrabl_crypto_early_init(void);

/**
 * @brief Initiate sha2 computation on given payload
 *        This is a non blocking call
 *
 * @param src pointer to payload
 * @param size size of payload
 * @param dst destination to store computed digest
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_compute_sha2_async(uint8_t *src, uint32_t size, uint8_t *dst);

/**
 * @brief Check the status of ongoing sha2 computation
 *        This will copy the result to the destination buffer
 *        This is a non blocking call
 *
 * @param status Address of the Place holder for copying the status
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_compute_sha2_done(bool *status);

/**
 * @brief Waits till ongoing sha2 computation is completed or not
 *        This will copy the result to the destination buffer
 *        This is a blocking call
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_compute_sha2_finish(void);

/**
 * @brief compute sha2 on given payload and store digest in dst
 *
 * @param src pointer to payload
 * @param size size of payload
 * @param dst destination to store computed digest
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_compute_sha2(uint8_t *src, uint32_t size,
				uint8_t *dst);

/**
 * @brief rsa-pss verification with SHA2 digest and preloaded keyslots
 *
 * @param src payload on which rsa-pss needs to be verified
 * @param size size of payload
 * @param signature signature against which rsapss needs to be verified
 * @param keyslot rsa keyslot to be used
 * @param keysize_bits size of the key
 * @param modulus pointer to rsa key modulus
 * @param mprime precomputed montgomery value
 * @param rsquare precomputed montgomery value
 * @param is_little_endian specifies endianness of the provided key
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_rsa_pss_verify(uint8_t *src,
	uint32_t size, uint8_t *signature, uint32_t keyslot, uint32_t keysize_bits,
	uint8_t *modulus, uint8_t *mprime, uint8_t *rsquare, bool is_little_endian);

/**
 * @brief AES decryption with preloaded keyslot and specified salt
 *
 * @param src payload to be decrypted
 * @param size size of payload
 * @param dst destination to store the decrypted payload
 * @param key_slot aes keyslot to be used for decryption
 * @param key_size_bytes size of the key to be used
 * @param salt salt to be used as ivs
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_decrypt_buffer(uint8_t *src, uint32_t size,
				uint8_t *dst, uint32_t key_slot,
				uint32_t key_size_bytes, uint8_t *salt);

/**
 * @brief AES encryption with preloaded keyslot and specified salt
 *
 * @param src payload to be encrypted
 * @param size size of payload
 * @param dst destination to store the encrypted payload
 * @param key_slot aes keyslot to be used for encryption
 * @param key_size_bytes size of the key to be used
 * @param salt salt to be used as ivs
 * @param skey key to be used for encryption
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_encrypt_buffer(uint8_t *src, uint32_t size,
				uint8_t *dst, uint32_t key_slot,
				uint32_t key_size_bytes, uint8_t *salt, uint8_t *skey);

/**
 * @brief AES CMAC with preloaded keyslot and given data
 *
 * @param src payload for CMAC computation
 * @param size size of payload
 * @param dst destination to store the computed CMAC
 * @parsm aes_key aes key that needs to be used to compute CMAC hash
 * @param key_slot aes keyslot to be used for CMAC computation
 * @param key_size_bytes size of the key to be used
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_compute_cmac(uint8_t *src,
											uint32_t size,
											uint8_t *dst,
											uint8_t *aes_key,
											uint32_t key_slot,
											uint32_t key_size_bytes);

/**
 * @brief Compute hash with zero sbk and compare with input cmac hash
 *
 * @param buffer Input buffer
 * @param buffer_size Size of buffer
 * @param cmachash Input cmac hash of buffer
 *
 * @return TEGRABL_NO_ERROR if buffer is valid else appropriate error.
 */
tegrabl_error_t tegrabl_verify_cmachash(void *buffer,
		uint32_t buffer_size, void *cmachash);

/**
 * @brief ecdsa verification with SHA2 digest
 *
 * @param src payload on which rsa-pss needs to be verified
 * @param size size of payload
 * @param signature signature against which ecdsa needs to be verified
 * @param keysize_bits size of the key
 * @param key pointer to ecdsa public key
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_ecdsa_verify(uint8_t *src, uint32_t size,
				uint8_t *signature, uint32_t keysize_bits,
				uint8_t *key);

/**
 * @brief ed25519 verification with SHA2 digest
 *
 * @param src payload on which rsa-pss needs to be verified
 * @param size size of payload
 * @param signature signature against which ed25519 needs to be verified
 * @param keysize_bits size of the key
 * @param key pointer to ed25519 public key
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_ed25519_verify(uint8_t *src, uint32_t size,
        uint8_t *signature, uint32_t keysize_bits, uint8_t *key);

/*
 * @brief Validate public-crypto parameters hash against fuses.
 *
 * @param pcp pointer to pcp in BCH
 *
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_crypto_validate_pcp(NvBootPublicCryptoParameters *pcp);

/*
 * @brief clear aes keyslot for se device
 *
 * @param keyslot to be cleared
 * @return TEGRABL_NO_ERROR if success, specific error if fails
 */
tegrabl_error_t tegrabl_se_clear_device_aes_keyslot(uint32_t keyslot);
#endif /* TEGRABL_CRYPTO_SE_H */

