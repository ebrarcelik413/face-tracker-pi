#include "crypto_service.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

// Pi ile AWS aynı key kullanır
static const unsigned char AES_KEY[32] = {
    0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
    0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
    0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
    0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef
};

bool CryptoService::encrypt(const std::vector<unsigned char>& plain,
                             std::vector<unsigned char>& cipher,
                             std::vector<unsigned char>& nonce,
                             std::vector<unsigned char>& tag) {
    nonce.resize(12);
    if (!RAND_bytes(nonce.data(), 12)) return false;

    cipher.resize(plain.size());
    tag.resize(16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr, AES_KEY, nonce.data());
    EVP_EncryptUpdate(ctx, cipher.data(), &len, plain.data(), plain.size());
    int clen = len;
    EVP_EncryptFinal_ex(ctx, cipher.data() + len, &len);
    clen += len;
    cipher.resize(clen);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data());
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool CryptoService::decrypt(const std::vector<unsigned char>& cipher,
                             const std::vector<unsigned char>& nonce,
                             const std::vector<unsigned char>& tag,
                             std::vector<unsigned char>& plain) {
    plain.resize(cipher.size());
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, AES_KEY, nonce.data());
    EVP_DecryptUpdate(ctx, plain.data(), &len, cipher.data(), cipher.size());
    int plen = len;
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                         (void*)tag.data());
    int ret = EVP_DecryptFinal_ex(ctx, plain.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);
    if (ret <= 0) return false;
    plen += len;
    plain.resize(plen);
    return true;
}

std::string CryptoService::base64Encode(const std::vector<unsigned char>& data) {
    BIO* b64  = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.data(), data.size());
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

std::vector<unsigned char> CryptoService::base64Decode(const std::string& b64) {
    BIO* bio  = BIO_new_mem_buf(b64.data(), b64.size());
    BIO* b64f = BIO_new(BIO_f_base64());
    bio = BIO_push(b64f, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    std::vector<unsigned char> result(b64.size());
    int len = BIO_read(bio, result.data(), b64.size());
    BIO_free_all(bio);
    result.resize(len > 0 ? len : 0);
    return result;
}