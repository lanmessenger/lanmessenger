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


#include "trace.h"
#include "crypto.h"

lmcCrypto::lmcCrypto(void) {
	pRsa = NULL;
	encryptMap.clear();
	decryptMap.clear();
	bits = 1024;
	exponent = 65537;
}

lmcCrypto::~lmcCrypto(void) {
	RSA_free(pRsa);
}

//	creates an RSA key pair and returns the string representation of the public key
QByteArray lmcCrypto::generateRSA(void) {
	unsigned char* buf = (unsigned char*)malloc(bits);
	RAND_seed(buf, bits);
	pRsa = RSA_generate_key(bits, exponent, NULL, NULL);

	BIO* bio = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPublicKey(bio, pRsa);
	int keylen = BIO_pending(bio);
	char* pem_key = (char*)calloc(keylen + 1, 1);
	BIO_read(bio, pem_key, keylen);
	publicKey = QByteArray(pem_key, keylen);
	BIO_free_all(bio);
	free(pem_key);
	free(buf);

	return publicKey;
}

//	generates a random aes key and iv, and encrypts it with the public key
QByteArray lmcCrypto::generateAES(QString* lpszUserId, QByteArray& pubKey) {
	char* pemKey = pubKey.data();
	RSA* rsa = RSA_new();
	BIO* bio = BIO_new_mem_buf(pemKey, pubKey.length());
	PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL);
	
	int keyDataLen = 32;
	unsigned char* keyData = (unsigned char*)malloc(keyDataLen);
	RAND_bytes(keyData, keyDataLen);
	int keyLen = 32;
	int ivLen = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
	int keyIvLen = keyLen + ivLen;
	unsigned char* keyIv = (unsigned char*)malloc(keyIvLen);
	int rounds = 5;
	keyLen = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), NULL, keyData, keyDataLen, rounds, keyIv, keyIv + keyLen);

	EVP_CIPHER_CTX ectx, dctx;
	EVP_CIPHER_CTX_init(&ectx);
	EVP_EncryptInit_ex(&ectx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
	encryptMap.insert(*lpszUserId, ectx);
	EVP_CIPHER_CTX_init(&dctx);
	EVP_DecryptInit_ex(&dctx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
	decryptMap.insert(*lpszUserId, dctx);

	unsigned char* eKeyIv = (unsigned char*)malloc(RSA_size(rsa));
	int eKeyIvLen = RSA_public_encrypt(keyIvLen, keyIv, eKeyIv, rsa, RSA_PKCS1_OAEP_PADDING);
	QByteArray baKeyIv((char*)eKeyIv, eKeyIvLen);

	BIO_free_all(bio);
	RSA_free(rsa);
	free(keyIv);
	free(eKeyIv);
	free(keyData);

	return baKeyIv;
}

//	decrypts the aes key and iv with the private key
void lmcCrypto::retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv) {
	unsigned char* keyIv = (unsigned char*)malloc(RSA_size(pRsa));
    RSA_private_decrypt(aesKeyIv.length(), (unsigned char*)aesKeyIv.data(), keyIv, pRsa, RSA_PKCS1_OAEP_PADDING);

	int keyLen = 32;
	EVP_CIPHER_CTX ectx, dctx;
	EVP_CIPHER_CTX_init(&ectx);
	EVP_EncryptInit_ex(&ectx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
	encryptMap.insert(*lpszUserId, ectx);
	EVP_CIPHER_CTX_init(&dctx);
	EVP_DecryptInit_ex(&dctx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
	decryptMap.insert(*lpszUserId, dctx);

	free(keyIv);
}

QByteArray lmcCrypto::encrypt(QString* lpszUserId, QByteArray& clearData) {
	int outLen = clearData.length() + AES_BLOCK_SIZE;
	unsigned char* outBuffer = (unsigned char*)malloc(outLen);
	if(outBuffer == NULL) {
		lmcTrace::write("Error: Buffer not allocated");
		return QByteArray();
	}
	int foutLen = 0;

	EVP_CIPHER_CTX ctx = encryptMap.value(*lpszUserId);
	if(EVP_EncryptInit_ex(&ctx, NULL, NULL, NULL, NULL)) {
		if(EVP_EncryptUpdate(&ctx, outBuffer, &outLen, (unsigned char*)clearData.data(), clearData.length())) {
			if(EVP_EncryptFinal_ex(&ctx, outBuffer + outLen, &foutLen)) {
				outLen += foutLen;
				QByteArray byteArray((char*)outBuffer, outLen);
				free(outBuffer);
				return byteArray;
			}
		}
	}
	lmcTrace::write("Error: Message encryption failed");
	return QByteArray();
}

QByteArray lmcCrypto::decrypt(QString* lpszUserId, QByteArray& cipherData) {
	int outLen = cipherData.length();
	unsigned char* outBuffer = (unsigned char*)malloc(outLen);
	if(outBuffer == NULL) {
		lmcTrace::write("Error: Buffer not allocated");
		return QByteArray();
	}
	int foutLen = 0;

	EVP_CIPHER_CTX ctx = decryptMap.value(*lpszUserId);
	if(EVP_DecryptInit_ex(&ctx, NULL, NULL, NULL, NULL)) {
		if(EVP_DecryptUpdate(&ctx, outBuffer, &outLen, (unsigned char*)cipherData.data(), cipherData.length())) {
			if(EVP_DecryptFinal_ex(&ctx, outBuffer + outLen, &foutLen)) {
				outLen += foutLen;
				QByteArray byteArray((char*)outBuffer, outLen);
				free(outBuffer);
				return byteArray;
			}
		}
	}
	lmcTrace::write("Error: Message decryption failed");
	return QByteArray();
}
