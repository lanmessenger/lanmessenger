# Crypto fixtures (interop vectors)

Test-only keypairs, independent of `lmcCrypto`:

- `test-rsa1024-pub-pkcs1.pem` — PKCS#1 public key (`BEGIN RSA PUBLIC KEY`)
  of a released RSA-1024 peer (v1.3.3); must keep interoperating with
  RSA-2048 builds. `generateAES` must keep accepting it.
- `test-rsa1024-priv.pem` — matching private key (test-only), used by the
  stock-OpenSSL oracle in `tst_crypto` to prove our OAEP blobs stay
  standards-compliant. Sealed session: envelope 128 / keyIv 48.
- `test-rsa2048-pub-pkcs1.pem` — PKCS#1 public key of a current RSA-2048
  peer, the exact format `lmcCrypto::generateRSA()` emits.
- `test-rsa2048-priv.pem` — matching private key (test-only), used the same
  way. Sealed session: envelope 256 / keyIv 48.
- `test-rsa2048-pub-spki.pem` — SubjectPublicKeyInfo public key
  (`BEGIN PUBLIC KEY`) of the same test 2048-bit key; `generateAES`
  accepts it (liberal read) while still emitting PKCS#1 itself.
- `test-rsa512-pub-pkcs1.pem` — PKCS#1 public key of a throwaway 512-bit
  key (private half discarded, never stored): parses fine but cannot fit
  the 48-byte keyIv under OAEP-SHA1 (64-42=22), so `generateAES` must fail
  closed with no session left behind.

Regenerate (any equivalent pair works; sizes above are asserted):

```sh
openssl genrsa -out test-rsa1024-priv.pem 1024
openssl rsa -in test-rsa1024-priv.pem -RSAPublicKey_out \
  -out test-rsa1024-pub-pkcs1.pem
openssl genrsa -out test-rsa2048-priv.pem 2048
openssl rsa -in test-rsa2048-priv.pem -RSAPublicKey_out \
  -out test-rsa2048-pub-pkcs1.pem
openssl rsa -in test-rsa2048-priv.pem -pubout \
  -out test-rsa2048-pub-spki.pem
# 512-bit probe: private half is throwaway, only the public key is stored.
openssl genrsa -out /tmp/throwaway-rsa512-priv.pem 512
openssl rsa -in /tmp/throwaway-rsa512-priv.pem -RSAPublicKey_out \
  -out test-rsa512-pub-pkcs1.pem
```
