/*
 * Copyright (C) 2015-2018, NVIDIA CORPORATION. All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 * */

#ifndef __SIG_PARSER_H_
#define __SIG_PARSER_H_

#include <stdint.h>

/*
 * Boot Image Signature Format:
 *
 * AndroidVerifiedBootSignature DEFINITIONS ::=
 *     BEGIN
 *	     FormatVersion ::= INTEGER
 *	     Certificate ::= Certificate OPTIONAL
 *	     AlgorithmIdentifier  ::=  SEQUENCE {
 *	          algorithm OBJECT IDENTIFIER,
 *	          parameters ANY DEFINED BY algorithm OPTIONAL
 *	     }
 *        AuthenticatedAttributes ::= SEQUENCE {
 *	          target CHARACTER STRING,
 *	          length INTEGER
 *	     }
 *        Signature ::= OCTET STRING
 *     END
 */

/*
 * X.509 Certificate Format:
 *
 * X509_Certificate DEFINITIONS ::=
 *    BEGIN
 *	     Certificate  ::=  SEQUENCE  {
 *          tbsCertificate       TBSCertificate,
 *          signatureAlgorithm   AlgorithmIdentifier,
 *          signatureValue       BIT STRING
 *       }
 *       TBSCertificate  ::=  SEQUENCE  {
 *          version         [0]  EXPLICIT Version DEFAULT v1,
 *          serialNumber         CertificateSerialNumber,
 *          signature            AlgorithmIdentifier,
 *          issuer               Name,
 *          validity             Validity,
 *          subject              Name,
 *          subjectPublicKeyInfo SubjectPublicKeyInfo,
 *          issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
 *                                    -- If present, version MUST be v2 or v3
 *          subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
 *                                    -- If present, version MUST be v2 or v3
 *          extensions      [3]  EXPLICIT Extensions OPTIONAL
 *                                    -- If present, version MUST be v3
 *       }
 *       Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
 *       CertificateSerialNumber  ::=  INTEGER
 *       Validity ::= SEQUENCE {
 *          notBefore      Time,
 *          notAfter       Time
 *       }
 *       Time ::= CHOICE {
 *          utcTime        UTCTime,
 *          generalTime    GeneralizedTime
 *       }
 *       UniqueIdentifier  ::=  BIT STRING
 *       SubjectPublicKeyInfo  ::=  SEQUENCE  {
 *          algorithm            AlgorithmIdentifier,
 *          subjectPublicKey     BIT STRING
 *       }
 *       Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension
 *       Extension  ::=  SEQUENCE  {
 *          extnID      OBJECT IDENTIFIER,
 *          critical    BOOLEAN DEFAULT FALSE,
 *          extnValue   OCTET STRING
 *                   -- contains the DER encoding of an ASN.1 value
 *                   -- corresponding to the extension type identified
 *                   -- by extnID
 *       }
 *    END
 */

/**
 * @brief Get certificate from boot image signature section
 *
 * @param vb_sig_der boot.img signature address
 * @param cert_der memory address of the certificate returned
 */
status_t get_cert(uint8_t *vb_sig_der, uint8_t **cert_der);

/**
 * @brief Extract public key modulus from certificate
 *
 * @param cert_der certificate address
 * @param pubkey_modulus public key address returned
 * @param mod_size modulus size defined by x509 cert
 */
status_t get_pubkey(uint8_t *cert_der, uint8_t **pubkey_modulus,
			size_t mod_size);

/**
 * @brief Extract authticated attibutes from boot image signature section
 *
 * @param vb_sig_der boot.img signature address
 * @param auth_attr_value auth_attr address returned
 * @param auth_attr_len auth_attr length returned
 */
status_t get_auth_attr(uint8_t *vb_sig_der, uint8_t **auth_attr_value,
			size_t *auth_attr_len);

/**
 * @brief Extract signature value from boot image signature section
 *
 * @param vb_sig_der boot.img signature address
 * @param sig_value signature value address returned
 * @param sig_value_len signature value length returned
 */
status_t get_vb_signature(uint8_t *vb_sig_der, uint8_t **sig_value,
			size_t *sig_value_len);

#endif
