// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "core/lmccategories.h"
#include "crypto.h"

// The RSA envelope on the wire is PKCS#1 PEM ("RSA PUBLIC KEY") by design:
// released peers parse it with a PKCS#1-only reader, so generateRSA keeps
// emitting those exact bytes. All key handling underneath is OpenSSL 3.0
// native (EVP_PKEY + OSSL_ENCODER/OSSL_DECODER); no deprecated RSA_* calls.

lmcCrypto::lmcCrypto(void) {
  pKey = NULL;
  encryptMap.clear();
  decryptMap.clear();
  bits = 2048;
  exponent = 65537;
}

lmcCrypto::~lmcCrypto(void) {
  EVP_PKEY_free(pKey);
}

//	creates an RSA key pair and returns the string representation of the public key.
//	The public key stays PKCS#1 PEM ("RSA PUBLIC KEY"): released peers parse
//	the wire format with a PKCS#1-only reader, so the bytes must not change
//	even though key generation is EVP_PKEY now.
QByteArray lmcCrypto::generateRSA(void) {
  EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
  BIGNUM* bn = BN_new();
  EVP_PKEY* key = NULL;
  if(kctx == NULL || bn == NULL || BN_set_word(bn, exponent) != 1 ||
     EVP_PKEY_keygen_init(kctx) != 1 || EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, bits) != 1 ||
     EVP_PKEY_CTX_set1_rsa_keygen_pubexp(kctx, bn) != 1 || EVP_PKEY_generate(kctx, &key) != 1) {
    qCCritical(lmcCoreLog(), "RSA key generation failed");
    BN_free(bn);
    EVP_PKEY_CTX_free(kctx);
    return QByteArray();
  }
  BN_free(bn);
  EVP_PKEY_CTX_free(kctx);

  BIO* bio = BIO_new(BIO_s_mem());
  OSSL_ENCODER_CTX* ectx =
    OSSL_ENCODER_CTX_new_for_pkey(key, OSSL_KEYMGMT_SELECT_PUBLIC_KEY, "PEM", "pkcs1", NULL);
  if(bio == NULL || ectx == NULL || OSSL_ENCODER_to_bio(ectx, bio) != 1) {
    qCCritical(lmcCoreLog(), "Public key export failed");
    OSSL_ENCODER_CTX_free(ectx);
    BIO_free_all(bio);
    EVP_PKEY_free(key);
    return QByteArray();
  }
  OSSL_ENCODER_CTX_free(ectx);
  char* pemKey = NULL;
  const long keylen = BIO_get_mem_data(bio, &pemKey);
  publicKey = QByteArray(pemKey, keylen);
  BIO_free_all(bio);

  EVP_PKEY_free(pKey);
  pKey = key;
  return publicKey;
}

//	generates a random aes key and iv, and encrypts it with the public key.
//	Accepts PKCS#1 ("RSA PUBLIC KEY", what generateRSA emits and released
//	peers send) as well as SubjectPublicKeyInfo ("PUBLIC KEY", liberal read).
//	The sealed envelope stays RSA-OAEP-SHA1 so released peers can open it.
QByteArray lmcCrypto::generateAES(QString* lpszUserId, QByteArray& pubKey) {
  EVP_PKEY* peerKey = NULL;
  OSSL_DECODER_CTX* dctx = OSSL_DECODER_CTX_new_for_pkey(
    &peerKey, "PEM", NULL, "RSA", OSSL_KEYMGMT_SELECT_PUBLIC_KEY, NULL, NULL);
  BIO* bio = BIO_new_mem_buf(pubKey.data(), pubKey.length());
  if(dctx == NULL || bio == NULL || OSSL_DECODER_from_bio(dctx, bio) != 1 || peerKey == NULL) {
    qCCritical(lmcCoreLog(), "Public key parse failed");
    OSSL_DECODER_CTX_free(dctx);
    BIO_free_all(bio);
    return QByteArray();
  }
  OSSL_DECODER_CTX_free(dctx);
  BIO_free_all(bio);

  int keyDataLen = 32;
  unsigned char* keyData = (unsigned char*)malloc(keyDataLen);
  if(keyData == NULL || RAND_bytes(keyData, keyDataLen) != 1) {
    qCCritical(lmcCoreLog(), "Session nonce failed");
    free(keyData);
    EVP_PKEY_free(peerKey);
    return QByteArray();
  }
  int keyLen = 32;
  int ivLen = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
  int keyIvLen = keyLen + ivLen;
  unsigned char* keyIv = (unsigned char*)malloc(keyIvLen);
  if(keyIv == NULL) {
    qCCritical(lmcCoreLog(), "Session buffer failed");
    free(keyData);
    EVP_PKEY_free(peerKey);
    return QByteArray();
  }
  int rounds = 5;
  keyLen = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), NULL, keyData, keyDataLen, rounds, keyIv, keyIv + keyLen);

  EVP_PKEY_CTX* wctx = EVP_PKEY_CTX_new(peerKey, NULL);
  size_t eKeyIvLen = 0;
  if(wctx == NULL || EVP_PKEY_encrypt_init(wctx) != 1 ||
     EVP_PKEY_CTX_set_rsa_padding(wctx, RSA_PKCS1_OAEP_PADDING) != 1 ||
     EVP_PKEY_CTX_set_rsa_oaep_md(wctx, EVP_sha1()) != 1 ||
     EVP_PKEY_CTX_set_rsa_mgf1_md(wctx, EVP_sha1()) != 1 ||
     EVP_PKEY_encrypt(wctx, NULL, &eKeyIvLen, keyIv, (size_t)keyIvLen) != 1) {
    qCCritical(lmcCoreLog(), "Session wrap failed");
    EVP_PKEY_CTX_free(wctx);
    EVP_PKEY_free(peerKey);
    free(keyIv);
    free(keyData);
    return QByteArray();
  }
  unsigned char* eKeyIv = (unsigned char*)malloc(eKeyIvLen);
  if(eKeyIv == NULL ||
     EVP_PKEY_encrypt(wctx, eKeyIv, &eKeyIvLen, keyIv, (size_t)keyIvLen) != 1) {
    qCCritical(lmcCoreLog(), "Session wrap failed");
    free(eKeyIv);
    EVP_PKEY_CTX_free(wctx);
    EVP_PKEY_free(peerKey);
    free(keyIv);
    free(keyData);
    return QByteArray();
  }
  EVP_PKEY_CTX_free(wctx);
  EVP_PKEY_free(peerKey);

  // Commit the session only once the sealed envelope exists: a failed wrap
  // must leave no phantom session behind (and must not clobber a previous
  // valid session for the same peer).
  EVP_CIPHER_CTX ectx, dctx2;
  EVP_EncryptInit_ex(ectx.ptr(), EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
  encryptMap.insert(*lpszUserId, ectx);
  EVP_CIPHER_CTX_init(dctx2.ptr());
  EVP_DecryptInit_ex(dctx2.ptr(), EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
  decryptMap.insert(*lpszUserId, dctx2);

  QByteArray baKeyIv((char*)eKeyIv, eKeyIvLen);

  free(keyIv);
  free(eKeyIv);
  free(keyData);

  return baKeyIv;
}

//	decrypts the aes key and iv with the private key. On failure the session
//	is left absent (fail closed): encrypt/decrypt for the peer then fail
//	through the existing unknown-peer path instead of chaining garbage.
void lmcCrypto::retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv) {
  if(pKey == NULL) {
    qCCritical(lmcCoreLog(), "Session unwrap failed: no private key");
    return;
  }
  EVP_PKEY_CTX* rctx = EVP_PKEY_CTX_new(pKey, NULL);
  size_t keyIvLen = 0;
  if(rctx == NULL || EVP_PKEY_decrypt_init(rctx) != 1 ||
     EVP_PKEY_CTX_set_rsa_padding(rctx, RSA_PKCS1_OAEP_PADDING) != 1 ||
     EVP_PKEY_CTX_set_rsa_oaep_md(rctx, EVP_sha1()) != 1 ||
     EVP_PKEY_CTX_set_rsa_mgf1_md(rctx, EVP_sha1()) != 1 ||
     EVP_PKEY_decrypt(rctx, NULL, &keyIvLen, (unsigned char*)aesKeyIv.data(),
                      (size_t)aesKeyIv.length()) != 1) {
    qCCritical(lmcCoreLog(), "Session unwrap failed");
    EVP_PKEY_CTX_free(rctx);
    return;
  }
  unsigned char* keyIv = (unsigned char*)malloc(keyIvLen);
  if(keyIv == NULL ||
     EVP_PKEY_decrypt(rctx, keyIv, &keyIvLen, (unsigned char*)aesKeyIv.data(),
                      (size_t)aesKeyIv.length()) != 1) {
    qCCritical(lmcCoreLog(), "Session unwrap failed");
    free(keyIv);
    EVP_PKEY_CTX_free(rctx);
    return;
  }
  EVP_PKEY_CTX_free(rctx);

  int keyLen = 32;
  const int ivLen = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
  if(keyIvLen != (size_t)(keyLen + ivLen)) {
    qCCritical(lmcCoreLog(), "Session unwrap failed: unexpected key material size");
    free(keyIv);
    return;
  }
  EVP_CIPHER_CTX ectx, dctx;
  EVP_EncryptInit_ex(ectx.ptr(), EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
  encryptMap.insert(*lpszUserId, ectx);
  EVP_DecryptInit_ex(dctx.ptr(), EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
  decryptMap.insert(*lpszUserId, dctx);

  free(keyIv);
}

bool lmcCrypto::hasSession(QString* lpszUserId) {
  return encryptMap.contains(*lpszUserId);
}

QByteArray lmcCrypto::encrypt(QString* lpszUserId, QByteArray& clearData) {
  int outLen = clearData.length() + AES_BLOCK_SIZE;
  unsigned char* outBuffer = (unsigned char*)malloc(outLen);
  if(outBuffer == NULL) {
    qCCritical(lmcCoreLog(), "Buffer not allocated");
    return QByteArray();
  }
  int foutLen = 0;

  EVP_CIPHER_CTX ctx = encryptMap.value(*lpszUserId);
  if(EVP_EncryptInit_ex(ctx.ptr(), NULL, NULL, NULL, NULL)) {
    if(EVP_EncryptUpdate(ctx.ptr(), outBuffer, &outLen, (unsigned char*)clearData.data(), clearData.length())) {
      if(EVP_EncryptFinal_ex(ctx.ptr(), outBuffer + outLen, &foutLen)) {
        outLen += foutLen;
        QByteArray byteArray((char*)outBuffer, outLen);
        free(outBuffer);
        return byteArray;
      }
    }
  }
  qCCritical(lmcCoreLog(), "Message encryption failed");
  free(outBuffer);
  return QByteArray();
}

QByteArray lmcCrypto::decrypt(QString* lpszUserId, QByteArray& cipherData) {
  int outLen = cipherData.length();
  unsigned char* outBuffer = (unsigned char*)malloc(outLen);
  if(outBuffer == NULL) {
    qCCritical(lmcCoreLog(), "Buffer not allocated");
    return QByteArray();
  }
  int foutLen = 0;

  EVP_CIPHER_CTX ctx = decryptMap.value(*lpszUserId);
  if(EVP_DecryptInit_ex(ctx.ptr(), NULL, NULL, NULL, NULL)) {
    if(EVP_DecryptUpdate(ctx.ptr(), outBuffer, &outLen, (unsigned char*)cipherData.data(), cipherData.length())) {
      if(EVP_DecryptFinal_ex(ctx.ptr(), outBuffer + outLen, &foutLen)) {
        outLen += foutLen;
        QByteArray byteArray((char*)outBuffer, outLen);
        free(outBuffer);
        return byteArray;
      }
    }
  }
  qCCritical(lmcCoreLog(), "Message decryption failed");
  free(outBuffer);
  return QByteArray();
}
