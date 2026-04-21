#ifndef CRYPTO_SERVICE_HPP
#define CRYPTO_SERVICE_HPP

#include <vector>

class CryptoService {
public:
    static std::vector<unsigned char> encrypt(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> decrypt(const std::vector<unsigned char>& data);
};

#endif
