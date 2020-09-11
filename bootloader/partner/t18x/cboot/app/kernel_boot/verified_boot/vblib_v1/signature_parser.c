/*
 * Copyright (C) 2015-2018, NVIDIA CORPORATION. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <trace.h>
#include "debug.h"

#include "asn1_decoder.h"
#include "signature_parser.h"

#define LOCAL_TRACE 0

/* Max permissible size of ASN1 encoding */
static const size_t max_der_length = 20 * 1024;

static const size_t num_skip_get_pubkey = 6;
static const size_t num_skip_get_sig_value = 4;
static const size_t num_skip_get_auth_attr = 3;

/* TODO: Move this function to a tegrabl_utils.c in common repo and avoid code
 * duplication with tegrabl_eeprom_dump
 */
static void print_value_hex(uint8_t *value, size_t len)
{
	size_t i;

	if (!value || !LOCAL_TRACE)
		return;

	for (i = 0; i < len; i++) {
		dprintf(INFO, "%2X ", *(value + i));
		if ((i + 1) % 16 == 0)
			dprintf(INFO, "\n");
	}
	dprintf(INFO, "\n");
}

status_t get_cert(uint8_t *vb_sig_der, uint8_t **cert_der)
{
	struct asn1_context *ctx, *sig_seq;
	status_t ret = NO_ERROR;

	if (!vb_sig_der || !cert_der)
		return ERR_INVALID_ARGS;

	ctx = asn1_context_new(vb_sig_der, max_der_length);
	if (ctx == NULL)
		return ERR_NOT_VALID;

	sig_seq = asn1_sequence_get(ctx);
	if (sig_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto ctx_free;
	}

	if (!asn1_sequence_next(sig_seq)) {
		ret = ERR_NOT_VALID;
		goto sig_seq_free;
	}

	*cert_der = sig_seq->p;

	/* DEBUG -- Dump Certificate*/
	LTRACEF("Certificate:\n");
	print_value_hex(*cert_der, 1024);

sig_seq_free:
	asn1_context_free(sig_seq);
ctx_free:
	asn1_context_free(ctx);

	return ret;
}

status_t get_pubkey(uint8_t *cert_der, uint8_t **pubkey_modulus,
				size_t mod_size)
{
	struct asn1_context *ctx, *cert, *cert_seq;
	struct asn1_context *pubkey_seq, *pubkeyinfo_seq, *pubkey;
	uint8_t *bitstring = NULL;
	uint8_t padding_bit;
	size_t modulus_len, len;
	status_t ret = NO_ERROR;

	if (!cert_der || !pubkey_modulus || mod_size == 0)
		return ERR_INVALID_ARGS;

	ctx = asn1_context_new(cert_der, max_der_length);
	if (ctx == NULL)
		return ERR_NOT_VALID;

	cert = asn1_sequence_get(ctx);
	if (cert == NULL) {
		ret = ERR_NOT_VALID;
		goto ctx_free;
	}

	cert_seq = asn1_sequence_get(cert);
	if (cert_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto cert_free;
	}

	/* skip some sequences before SubjectPublicKeyInfo */
	if (!asn1_sequence_skip(cert_seq, num_skip_get_pubkey)) {
		ret = ERR_NOT_VALID;
		goto cert_seq_free;
	}

	/* Get SubjectPublicKeyInfo */
	pubkeyinfo_seq = asn1_sequence_get(cert_seq);
	if (pubkeyinfo_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto cert_seq_free;
	}

	if (!asn1_sequence_next(pubkeyinfo_seq)) {
		ret = ERR_NOT_VALID;
		goto pubkeyinfo_seq_free;
	}

	if (!asn1_bit_string_get(pubkeyinfo_seq, &bitstring, &padding_bit,
							&len) || padding_bit != 0) {
		ret = ERR_NOT_VALID;
		goto pubkeyinfo_seq_free;
	}

	pubkey_seq = asn1_context_new(bitstring, len);
	if (pubkey_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto pubkeyinfo_seq_free;
	}

	pubkey = asn1_sequence_get(pubkey_seq);
	if (pubkey == NULL) {
		ret = ERR_NOT_VALID;
		goto pubkey_seq_free;
	}

	if (!asn1_integer_get(pubkey, pubkey_modulus, &modulus_len)) {
		ret = ERR_NOT_VALID;
		goto pubkey_free;
	}

	/* Ignore possible padding byte in asn1 integer*/
	if (*(*pubkey_modulus) == 0x00) {
		(*pubkey_modulus)++;
		modulus_len--;
	}

	/* DEBUG -- Dump public key */
	LTRACEF("Dump public key modulus:\n");
	print_value_hex(*pubkey_modulus, modulus_len);

	if (modulus_len != mod_size) {
		dprintf(INFO, "Public key modulus is not valid\n");
		ret = ERR_NOT_VALID;
		goto pubkey_free;
	}

pubkey_free:
	asn1_context_free(pubkey);
pubkey_seq_free:
	asn1_context_free(pubkey_seq);
pubkeyinfo_seq_free:
	asn1_context_free(pubkeyinfo_seq);
cert_seq_free:
	asn1_context_free(cert_seq);
cert_free:
	asn1_context_free(cert);
ctx_free:
	asn1_context_free(ctx);

	return ret;
}

status_t get_auth_attr(uint8_t *vb_sig_der, uint8_t **authattr_der,
		   size_t *authattr_len)
{
	struct asn1_context *ctx, *sig_seq, *attr_seq;
	status_t ret = NO_ERROR;

	if (!vb_sig_der || !authattr_der || !authattr_len)
		return ERR_INVALID_ARGS;

	ctx = asn1_context_new(vb_sig_der, max_der_length);
	if (ctx == NULL)
		return ERR_NOT_VALID;

	sig_seq = asn1_sequence_get(ctx);
	if (sig_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto ctx_free;
	}

	/* Skip some sequence before getting authticated attributes*/
	if (!asn1_sequence_skip(sig_seq, num_skip_get_auth_attr)) {
		ret = ERR_NOT_VALID;
		goto sig_seq_free;
	}

	/* Get Authenticated Attributes */
	*authattr_der = sig_seq->p;

	attr_seq = asn1_sequence_get(sig_seq);
	if (attr_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto sig_seq_free;
	}

	/* +2 here to include asn1 header(type + length)*/
	*authattr_len = attr_seq->length + 2;
	asn1_context_free(attr_seq);

sig_seq_free:
	asn1_context_free(sig_seq);
ctx_free:
	asn1_context_free(ctx);

	return ret;
}

status_t get_vb_signature(uint8_t *vb_sig_der, uint8_t **sig_value,
		   size_t *sig_value_len)
{
	struct asn1_context *ctx, *sig_seq;
	uint8_t *sig_der_ptr;
	status_t ret = NO_ERROR;

	ctx = asn1_context_new(vb_sig_der, max_der_length);
	if (ctx == NULL)
		return ERR_NOT_VALID;

	sig_seq = asn1_sequence_get(ctx);
	if (sig_seq == NULL) {
		ret = ERR_NOT_VALID;
		goto ctx_free;
	}

	/* Skip some sequence before getting signature value*/
	if (!asn1_sequence_skip(sig_seq, num_skip_get_sig_value)) {
		ret = ERR_NOT_VALID;
		goto sig_seq_free;
	}

	/* Get Signature value */
	if (!asn1_octet_string_get(sig_seq, &sig_der_ptr, sig_value_len)) {
		ret = ERR_NOT_VALID;
		goto sig_seq_free;
	}

	*sig_value = sig_der_ptr;
	/* DEBUG -- Dump signature value */
	LTRACEF("Signature value:\n");
	print_value_hex(*sig_value, *sig_value_len);

sig_seq_free:
	asn1_context_free(sig_seq);
ctx_free:
	asn1_context_free(ctx);

	return ret;
}
