#include "cloud_service.hpp"
#include "crypto_service.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>

CloudService::CloudService(const std::string& ip, int port)
    : ip_(ip), port_(port), sock_(-1) {}

CloudService::~CloudService() {
    if (sock_ >= 0) close(sock_);
}

bool CloudService::ensureConnection() {
    if (sock_ >= 0) return true;

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) return false;

    struct timeval tv{10, 0};
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    if (connect(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock_); sock_ = -1; return false;
    }
    std::cout << "[OK] Cloud baglantisi kuruldu" << std::endl;
    return true;
}

std::string CloudService::httpPost(const std::string& body) {
    // İlk bağlantı veya yeniden bağlan
    if (!ensureConnection()) return "";

    std::string req =
        "POST /detect HTTP/1.1\r\n"
        "Host: " + ip_ + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n" + body;

    if (send(sock_, req.c_str(), req.size(), 0) < 0) {
        close(sock_); sock_ = -1; return "";
    }

    std::string resp;
    char buf[65536];
    int n;
    while ((n = recv(sock_, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = 0; resp += buf;
    }
    // Connection: close — sunucu kapattı, biz de kapatalım
    close(sock_); sock_ = -1;

    auto pos = resp.find("\r\n\r\n");
    return pos != std::string::npos ? resp.substr(pos+4) : resp;
}

std::string CloudService::jsonGet(const std::string& json,
                                   const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    if (json[pos] == '"') {
        pos++;
        auto end = json.find('"', pos);
        return json.substr(pos, end-pos);
    }
    auto end = json.find_first_of(",}", pos);
    return json.substr(pos, end-pos);
}

FaceResult CloudService::detect(const std::vector<unsigned char>& jpegFrame) {
    FaceResult res;
    auto t0 = std::chrono::steady_clock::now();

    std::vector<unsigned char> cipher, nonce, tag;
    if (!CryptoService::encrypt(jpegFrame, cipher, nonce, tag)) return res;

    std::vector<unsigned char> cipherWithTag = cipher;
    cipherWithTag.insert(cipherWithTag.end(), tag.begin(), tag.end());

    std::string frameB64 = CryptoService::base64Encode(cipherWithTag);
    std::string nonceB64 = CryptoService::base64Encode(nonce);

    std::string body = "{\"frame\":\"" + frameB64 +
                       "\",\"nonce\":\"" + nonceB64 + "\"}";

    std::string resp = httpPost(body);
    if (resp.empty()) { connected_ = false; return res; }
    connected_ = true;

    std::string encB64   = jsonGet(resp, "encrypted");
    std::string rNonce64 = jsonGet(resp, "nonce");

    if (encB64.empty()) return res;

    auto encWithTag = CryptoService::base64Decode(encB64);
    auto rNonce     = CryptoService::base64Decode(rNonce64);

    if (encWithTag.size() < 16) return res;

    std::vector<unsigned char> rCipher(encWithTag.begin(), encWithTag.end()-16);
    std::vector<unsigned char> rTag(encWithTag.end()-16, encWithTag.end());

    std::vector<unsigned char> plain;
    if (!CryptoService::decrypt(rCipher, rNonce, rTag, plain)) return res;

    std::string jsonResp(plain.begin(), plain.end());

    std::string foundStr = jsonGet(jsonResp, "face_found");
    foundStr.erase(0, foundStr.find_first_not_of(" \t\r\n"));
    foundStr.erase(foundStr.find_last_not_of(" \t\r\n")+1);
    res.found = (foundStr == "true");

    if (res.found) {
        try {
            res.cx = std::stof(jsonGet(jsonResp, "cx"));
            res.cy = std::stof(jsonGet(jsonResp, "cy"));
            res.x  = std::stof(jsonGet(jsonResp, "x"));
            res.y  = std::stof(jsonGet(jsonResp, "y"));
            res.w  = std::stof(jsonGet(jsonResp, "w"));
            res.h  = std::stof(jsonGet(jsonResp, "h"));
            res.confidence = std::stof(jsonGet(jsonResp, "confidence"));
        } catch (...) {}
    }

    auto t1 = std::chrono::steady_clock::now();
    res.latency_ms = std::chrono::duration<float,std::milli>(t1-t0).count();
    return res;
}