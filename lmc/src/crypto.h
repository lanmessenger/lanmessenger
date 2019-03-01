/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
**
** Contact:  qualiatech@gmail.com
**
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#include <QMap>
#include <QDataStream>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/aes.h>

#ifndef CRYPTO_H
#define CRYPTO_H

class EVP_CIPHER_CTX_wrapper {
public :

	#if OPENSSL_VERSION_NUMBER < 0x10100000L

	EVP_CIPHER_CTX_wrapper() {
		EVP_CIPHER_CTX_init(&ctx);
	}
	EVP_CIPHER_CTX* ptr() { return &ctx; };

	EVP_CIPHER_CTX ctx;

	#else

	EVP_CIPHER_CTX_wrapper() {
		ctx = EVP_CIPHER_CTX_new();
		EVP_CIPHER_CTX_init(ctx);
	}

	~EVP_CIPHER_CTX_wrapper() {
		EVP_CIPHER_CTX_free(ctx);
	}

	EVP_CIPHER_CTX* ptr() { return ctx; };

	EVP_CIPHER_CTX* ctx;

	#endif

};

#define EVP_CIPHER_CTX EVP_CIPHER_CTX_wrapper

class lmcCrypto
{
public:
	lmcCrypto(void);
	~lmcCrypto(void);

	QByteArray generateRSA(void);
	QByteArray generateAES(QString* lpszUserId, QByteArray& pubKey);
	void retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv);
	QByteArray encrypt(QString* lpszUserId, QByteArray& clearData);
	QByteArray decrypt(QString* lpszUserId, QByteArray& cipherData);

	QByteArray publicKey;

private:
	RSA* pRsa;
	QMap<QString, EVP_CIPHER_CTX> encryptMap;
	QMap<QString, EVP_CIPHER_CTX> decryptMap;
	int bits;
	long exponent;
};

#endif // CRYPTO_H
