# Crypto fixtures (pre-migration vectors)

Test-only RSA-1024 keypair, independent of `lmcCrypto`:

- `test-rsa1024-pub-pkcs1.pem` — PKCS#1 public key (`BEGIN RSA PUBLIC KEY`),
  the exact format `lmcCrypto::generateRSA()` emits. `generateAES` must keep
  accepting it after the EVP_PKEY migration (dual-read).
- `test-rsa1024-priv.pem` — matching private key, used by the stock-OpenSSL
  oracle in `tst_crypto` to prove our OAEP blobs stay standards-compliant.

Regenerate (any equivalent 1024-bit pair works; sizes 128/48 are asserted):

```sh
openssl genrsa -out test-rsa1024-priv.pem 1024
openssl rsa -in test-rsa1024-priv.pem -RSAPublicKey_out \
  -out test-rsa1024-pub-pkcs1.pem
```
