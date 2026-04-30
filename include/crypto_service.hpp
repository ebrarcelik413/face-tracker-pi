#pragma once
#include <vector>
#include <string>

class CryptoService {
public:
    static bool encrypt(const std::vector<unsigned char>& plain,
                        std::vector<unsigned char>& cipher,
                        std::vector<unsigned char>& nonce,
                        std::vector<unsigned char>& tag);

    static bool decrypt(const std::vector<unsigned char>& cipher,
                        const std::vector<unsigned char>& nonce,
                        const std::vector<unsigned char>& tag,
                        std::vector<unsigned char>& plain);

    static std::string base64Encode(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> base64Decode(const std::string& b64);
};