// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for lmcCrypto (message-level crypto over plain TCP).

#include <QtTest>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "core/crypto.h"

namespace {

// Freshly generated (post-change) RSA-2048 OAEP envelope; 32-byte AES-256 key + 16-byte IV.
constexpr int kSealedSessionSize2048 = 256;
// Pre-change RSA-1024 envelope, still spoken by released peers.
constexpr int kSealedSessionSize1024 = 128;
constexpr int kSessionKeyIvSize = 48; // unchanged

QByteArray readFixture(const char* name) {
  QFile file(QString::fromLocal8Bit(LMC_CRYPTO_FIXTURES_DIR) +
             QLatin1Char('/') + QString::fromLatin1(name));
  if(!file.open(QIODevice::ReadOnly))
    return QByteArray();
  return file.readAll();
}

// --- Stock-OpenSSL oracle (independent of lmcCrypto) ---

EVP_PKEY* loadPrivateKey(const QByteArray& pem) {
  BIO* bio = BIO_new_mem_buf(pem.constData(), pem.size());
  if(!bio)
    return nullptr;
  EVP_PKEY* key = nullptr;
  if(PEM_read_bio_PrivateKey(bio, &key, nullptr, nullptr) == nullptr)
    key = nullptr;
  BIO_free(bio);
  return key;
}

QByteArray stockDecryptOaepSha1(EVP_PKEY* key, const QByteArray& blob) {
  QByteArray result;
  EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(key, nullptr);
  if(!ctx)
    return result;

  const auto* in = reinterpret_cast<const unsigned char*>(blob.constData());
  size_t outLen = 0;
  bool ok = EVP_PKEY_decrypt_init(ctx) > 0 &&
            EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) > 0 &&
            EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha1()) > 0 &&
            EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha1()) > 0 &&
            EVP_PKEY_decrypt(ctx, nullptr, &outLen, in, size_t(blob.size())) > 0;
  if(ok) {
    result.resize(int(outLen));
    auto* out = reinterpret_cast<unsigned char*>(result.data());
    if(EVP_PKEY_decrypt(ctx, out, &outLen, in, size_t(blob.size())) <= 0)
      result.clear();
    else
      result.resize(int(outLen));
  }
  EVP_PKEY_CTX_free(ctx);
  return result;
}

// --- Protocol helpers, mirroring tcpnetwork.cpp ---
// The legacy API takes QString*/QByteArray& (never modified); hidden here.

QByteArray encryptFor(lmcCrypto& crypto, const QString& peerId, const QByteArray& plain) {
  QString id = peerId;
  QByteArray copy = plain;
  return crypto.encrypt(&id, copy);
}

QByteArray decryptFrom(lmcCrypto& crypto, const QString& peerId, const QByteArray& cipher) {
  QString id = peerId;
  QByteArray copy = cipher;
  return crypto.decrypt(&id, copy);
}

QByteArray wrapSession(lmcCrypto& crypto, const QString& peerId, const QByteArray& pubKey) {
  QString id = peerId;
  QByteArray copy = pubKey;
  return crypto.generateAES(&id, copy);
}

struct PeerPair {
  lmcCrypto server;
  lmcCrypto client;
  QString serverId = QStringLiteral("alice");
  QString clientId = QStringLiteral("bob");

  bool shakeHands(void) {
    const QByteArray pub = server.generateRSA();
    if(pub.isEmpty())
      return false;
    QByteArray session = wrapSession(client, serverId, pub);
    if(session.isEmpty())
      return false;
    server.retreiveAES(&clientId, session);
    return true;
  }

  QByteArray clientToServer(const QByteArray& plain) {
    return decryptFrom(server, clientId, encryptFor(client, serverId, plain));
  }

  QByteArray serverToClient(const QByteArray& plain) {
    return decryptFrom(client, serverId, encryptFor(server, clientId, plain));
  }
};

} // namespace

class TstCrypto : public QObject {
  Q_OBJECT

private slots:
  void publicKeyIsPkcs1(void);
  void handshakeBothDirections(void);
  void payloadMatrix_data(void);
  void payloadMatrix(void);
  void sequentialMessagesKeepChain(void);
  void unknownPeerFails(void);
  void tamperedCiphertextFails(void);
  void legacyPkcs1PublicKeyHandshake(void);
  void oldClientToNewServerHandshake(void);
  void generatedKeyIs2048(void);
};

void TstCrypto::publicKeyIsPkcs1(void) {
  lmcCrypto crypto;
  const QByteArray pub = crypto.generateRSA();
  QVERIFY2(!pub.isEmpty(), "generateRSA returned empty key");
  QVERIFY2(pub.startsWith("-----BEGIN RSA PUBLIC KEY-----"), "public key is not PKCS#1");
}

void TstCrypto::handshakeBothDirections(void) {
  PeerPair pair;
  QVERIFY2(pair.shakeHands(), "handshake failed");

  const QByteArray message = QByteArrayLiteral("hello, lan");
  QCOMPARE(pair.clientToServer(message), message);
  QCOMPARE(pair.serverToClient(message), message);
}

void TstCrypto::payloadMatrix_data(void) {
  QTest::addColumn<QByteArray>("plain");
  QTest::newRow("empty") << QByteArray(); // padding block only
  QTest::newRow("one-byte") << QByteArrayLiteral("x");
  QTest::newRow("block-minus-1") << QByteArray(15, 'a');
  QTest::newRow("one-block") << QByteArray(16, 'b');
  QTest::newRow("block-plus-1") << QByteArray(17, 'c');
  QTest::newRow("1kib") << QByteArray(1024, 'd');
  QTest::newRow("64kib") << QByteArray(65536, 'e');
  QTest::newRow("embedded-nulls") << QByteArray("a\0b\0c\0", 6);
}

void TstCrypto::payloadMatrix(void) {
  QFETCH(QByteArray, plain);

  PeerPair pair; // fresh session per payload: isolates framing from chaining
  QVERIFY2(pair.shakeHands(), "handshake failed");
  QCOMPARE(pair.clientToServer(plain), plain);
  QCOMPARE(pair.serverToClient(plain), plain);
}

void TstCrypto::sequentialMessagesKeepChain(void) {
  // Contexts are stateful CBC chains (Init_ex(NULL) over the stored ctx):
  // a fresh IV per message would look equivalent but break old peers.
  PeerPair pair;
  QVERIFY2(pair.shakeHands(), "handshake failed");

  for(int i = 0; i < 5; i++) {
    const QByteArray message = QByteArrayLiteral("message #") + QByteArray::number(i);
    QCOMPARE(pair.clientToServer(message), message);
    QCOMPARE(pair.serverToClient(message), message);
  }
}

void TstCrypto::unknownPeerFails(void) {
  PeerPair pair;
  QVERIFY2(pair.shakeHands(), "handshake failed");

  const QString stranger = QStringLiteral("mallory");
  QVERIFY2(encryptFor(pair.client, stranger, "should not encrypt").isEmpty(),
           "encrypt with unknown peer must fail");
  QVERIFY2(decryptFrom(pair.server, stranger, "not a ciphertext").isEmpty(),
           "decrypt with unknown peer must fail");
}

void TstCrypto::tamperedCiphertextFails(void) {
  PeerPair pair;
  QVERIFY2(pair.shakeHands(), "handshake failed");

  const QByteArray message = QByteArrayLiteral("integrity probe 1234567890");
  QByteArray cipher = encryptFor(pair.client, pair.serverId, message);
  QVERIFY2(cipher.size() > 1, "ciphertext too short to tamper");

  cipher[cipher.size() - 1] = char(cipher[cipher.size() - 1] ^ 0xFF);
  const QByteArray recovered = decryptFrom(pair.server, pair.clientId, cipher);
  // CBC without MAC: garbled plaintext, padding may pass by chance (~1/256).
  QVERIFY2(recovered.isEmpty() || recovered != message,
           "tampered ciphertext must not recover the original");
}

void TstCrypto::legacyPkcs1PublicKeyHandshake(void) {
  // Released-version pin: the fixture is a PKCS#1 key of an RSA-1024 peer
  // (v1.3.3). generateAES must keep accepting it, and the sealed session
  // must stay stock-OpenSSL readable.
  const QByteArray legacyPub = readFixture("test-rsa1024-pub-pkcs1.pem");
  QVERIFY2(legacyPub.startsWith("-----BEGIN RSA PUBLIC KEY-----"), "fixture is not PKCS#1");

  lmcCrypto client;
  const QByteArray session = wrapSession(client, QStringLiteral("legacy-peer"), legacyPub);
  QCOMPARE(session.size(), kSealedSessionSize1024);

  EVP_PKEY* key = loadPrivateKey(readFixture("test-rsa1024-priv.pem"));
  QVERIFY2(key != nullptr, "fixture private key unreadable");
  QCOMPARE(stockDecryptOaepSha1(key, session).size(), kSessionKeyIvSize);
  EVP_PKEY_free(key);
}

void TstCrypto::oldClientToNewServerHandshake(void) {
  // Old (released 1024) writes to new (2048): wrapping depends only on the
  // server's public key, so any lmcCrypto emulates the old client. The new
  // server opens the 256-byte envelope with its own private key for real.
  lmcCrypto server;
  const QByteArray serverPub = server.generateRSA();
  QVERIFY2(serverPub.startsWith("-----BEGIN RSA PUBLIC KEY-----"), "public key is not PKCS#1");

  lmcCrypto client;
  QString serverId = QStringLiteral("new-server");
  QString clientId = QStringLiteral("old-client");
  QByteArray session = wrapSession(client, serverId, serverPub);
  QCOMPARE(session.size(), kSealedSessionSize2048);

  // Pin the mixed-pair asymmetry explicitly: same client, old vs new key.
  const QByteArray oldPub = readFixture("test-rsa1024-pub-pkcs1.pem");
  QVERIFY2(!oldPub.isEmpty(), "fixture missing");
  QCOMPARE(wrapSession(client, QStringLiteral("old-server"), oldPub).size(), kSealedSessionSize1024);

  server.retreiveAES(&clientId, session);

  const QList<QByteArray> messages = {
    QByteArrayLiteral("hello, new peer"),
    QStringLiteral("привет, новый пир! ✓").toUtf8(),
  };
  for(const QByteArray& message : messages) {
    QCOMPARE(decryptFrom(server, clientId, encryptFor(client, serverId, message)), message); // old -> new
    QCOMPARE(decryptFrom(client, serverId, encryptFor(server, clientId, message)), message); // new -> old
  }
}

void TstCrypto::generatedKeyIs2048(void) {
  // Pin the new default: sessions wrapped for a freshly generated key are
  // 256-byte envelopes (only a >=2048-bit modulus fits 48-byte keyIv under
  // OAEP-SHA1), and our 2048 envelopes stay stock-OpenSSL readable.
  lmcCrypto crypto;
  const QByteArray pub = crypto.generateRSA();
  QVERIFY2(pub.startsWith("-----BEGIN RSA PUBLIC KEY-----"), "public key is not PKCS#1");

  lmcCrypto client;
  QCOMPARE(wrapSession(client, QStringLiteral("peer"), pub).size(), kSealedSessionSize2048);

  // Fixture pair oracle: same wrap path against a known 2048-bit key.
  const QByteArray fixturePub = readFixture("test-rsa2048-pub-pkcs1.pem");
  QVERIFY2(fixturePub.startsWith("-----BEGIN RSA PUBLIC KEY-----"), "fixture is not PKCS#1");
  const QByteArray session = wrapSession(client, QStringLiteral("fixture-peer"), fixturePub);
  QCOMPARE(session.size(), kSealedSessionSize2048);
  EVP_PKEY* fixturePriv = loadPrivateKey(readFixture("test-rsa2048-priv.pem"));
  QVERIFY2(fixturePriv != nullptr, "fixture private key unreadable");
  QCOMPARE(stockDecryptOaepSha1(fixturePriv, session).size(), kSessionKeyIvSize);
  EVP_PKEY_free(fixturePriv);
}

QTEST_MAIN(TstCrypto)

#include "tst_crypto.moc"
