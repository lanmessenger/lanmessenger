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

Regenerate (any equivalent pair works; sizes above are asserted):

```sh
openssl genrsa -out test-rsa1024-priv.pem 1024
openssl rsa -in test-rsa1024-priv.pem -RSAPublicKey_out \
  -out test-rsa1024-pub-pkcs1.pem
openssl genrsa -out test-rsa2048-priv.pem 2048
openssl rsa -in test-rsa2048-priv.pem -RSAPublicKey_out \
  -out test-rsa2048-pub-pkcs1.pem
```
