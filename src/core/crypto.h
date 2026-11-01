// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QMap>
#include <QDataStream>
#include <openssl/decoder.h>
#include <openssl/encoder.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
// rsa.h only for the RSA_PKCS1_OAEP_PADDING constant (not a deprecated
// function); the low-level RSA_* API itself is gone, key handling is EVP_PKEY.
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/aes.h>

#pragma once

class EVP_CIPHER_CTX_wrapper {
public:
#if OPENSSL_VERSION_NUMBER < 0x10100000L

  EVP_CIPHER_CTX_wrapper() {
    EVP_CIPHER_CTX_init(&ctx);
  }
  EVP_CIPHER_CTX* ptr() {
    return &ctx;
  };

  EVP_CIPHER_CTX ctx;

#else

  EVP_CIPHER_CTX_wrapper() {
    ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
  }

  EVP_CIPHER_CTX_wrapper(const EVP_CIPHER_CTX_wrapper& other) {
    ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_copy(ctx, other.ctx);
  }

  EVP_CIPHER_CTX_wrapper& operator=(const EVP_CIPHER_CTX_wrapper& other) {
    if(this != &other)
      EVP_CIPHER_CTX_copy(ctx, other.ctx);
    return *this;
  }

  ~EVP_CIPHER_CTX_wrapper() {
    EVP_CIPHER_CTX_free(ctx);
  }

  EVP_CIPHER_CTX* ptr() {
    return ctx;
  };

  EVP_CIPHER_CTX* ctx;

#endif
};

#define EVP_CIPHER_CTX EVP_CIPHER_CTX_wrapper

class lmcCrypto {
public:
  lmcCrypto(void);
  ~lmcCrypto(void);

  QByteArray generateRSA(void);
  QByteArray generateAES(QString* lpszUserId, QByteArray& pubKey);
  void retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv);
  bool hasSession(QString* lpszUserId);
  QByteArray encrypt(QString* lpszUserId, QByteArray& clearData);
  QByteArray decrypt(QString* lpszUserId, QByteArray& cipherData);

  QByteArray publicKey;

private:
  EVP_PKEY* pKey;
  QMap<QString, EVP_CIPHER_CTX> encryptMap;
  QMap<QString, EVP_CIPHER_CTX> decryptMap;
  int bits;
  long exponent;
};
