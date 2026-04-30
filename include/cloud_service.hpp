#pragma once
#include <string>
#include <vector>
#include "cloud_service.hpp"

struct FaceResult {
    float cx=0, cy=0, x=0, y=0, w=0, h=0;
    bool  found=false;
    float confidence=0;
    float latency_ms=0;
};

class CloudService {
public:
    CloudService(const std::string& ip, int port);
    ~CloudService();
    FaceResult detect(const std::vector<unsigned char>& jpegFrame);
    bool isConnected() const { return connected_; }

private:
    std::string ip_;
    int port_;
    bool connected_ = false;
    int sock_ = -1;  // kalıcı socket
    
    bool ensureConnection();
    std::string httpPost(const std::string& body);
    std::string jsonGet(const std::string& json, const std::string& key);
};