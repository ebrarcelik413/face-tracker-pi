#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

using namespace cv;
using namespace std;

// --- 1. ORTAK VERİ ALANI (Shared State) ---
// Thread'lerin çarpışmasını engellemek için mutex (kilit) kullanıyoruz.
Mat sharedFrame; // Bir kez yer ayıracağız
mutex frameMutex;
Mat currentFrame;       // Kameradan gelen en son görüntü
bool newFrame = false;  // Yeni görüntü geldi mi bayrağı

mutex dataMutex;
float targetX = 0, targetY = 0; // AI'ın bulduğu yüz koordinatları
bool faceFound = false;         // Yüz var mı yok mu?

atomic<bool> isRunning(true);   // Sistemi durdurmak için genel anahtar


// --- 2. KAMERA THREAD'İ ---
// Görevi: Sadece kameradan 30 FPS hızında sürekli görüntü almak.
void cameraThreadFunc() {
    VideoCapture cap(0, CAP_V4L2);
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS, 20);
    cap.set(CAP_PROP_BUFFERSIZE, 1);

    if (!cap.isOpened()) {
        cerr << "[HATA] Kamera acilamadi!" << endl;
        isRunning = false;
        return;
    }

    Mat tempFrame;
    while (isRunning) {
        cap >> tempFrame;
        if (tempFrame.empty()) continue;

        // Görüntüyü hem AI için paylaşıyoruz hem de dosyaya yazıyoruz
        {
            lock_guard<mutex> lock(frameMutex);
            tempFrame.copyTo(sharedFrame);
            newFrame = true;
        }

        // --- KRİTİK EKSİK BURASIYDI: Dashboard için dosyayı güncelle ---
        // Her 2 karede bir yazarak işlemciyi yormayalım
        static int frameCount = 0;
        if (frameCount++ % 2 == 0) {
            // DOSYA YOLUNU GARANTİYE ALALIM
            cv::imwrite("/tmp/snapshot.jpg", tempFrame);
        }
    }
    cap.release();
}


// --- 3. YAPAY ZEKA (AI) THREAD'İ ---
// Görevi: En taze görüntüyü alıp yüz bulmak (YuNet veya Haar).
void aiThreadFunc() {
    // YuNet modelini yükle (Klasöründe face_detection_yunet_2023mar.onnx olmalı)
    string modelPath = "face_detection_yunet_2023mar.onnx";
    Ptr<FaceDetectorYN> detector = FaceDetectorYN::create(modelPath, "", Size(320, 240));

    Mat frameToProcess;
    while (isRunning) {
        bool hasNewFrame = false;
        {
            lock_guard<mutex> lock(frameMutex);
            if (newFrame && !sharedFrame.empty()) {
                // clone() yerine copyTo() kullanarak bellek yönetimini optimize edelim
                sharedFrame.copyTo(frameToProcess);
                newFrame = false;
                hasNewFrame = true;
            }
        }

        if (hasNewFrame) {
            // Görüntüyü AI'nın anlayacağı boyuta (320x240) düşür
            Mat inputBlob;
            resize(frameToProcess, inputBlob, Size(320, 240));
            
            Mat faces;
            detector->setInputSize(inputBlob.size());
            detector->detect(inputBlob, faces);

            if (!faces.empty()) {
                lock_guard<mutex> lock(dataMutex);
                // Yüzün merkez koordinatlarını (x, y) güncelle
                targetX = faces.at<float>(0, 0) + faces.at<float>(0, 2) / 2;
                targetY = faces.at<float>(0, 1) + faces.at<float>(0, 3) / 2;
                faceFound = true;
                // Terminale koordinat bas ki çalıştığını görelim
                cout << "Yuz Tespit Edildi: X=" << targetX << " Y=" << targetY << endl;
            } else {
                lock_guard<mutex> lock(dataMutex);
                faceFound = false;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(5));
    }
}


// --- 4. MOTOR (KONTROL) THREAD'İ ---
// Görevi: Koordinatı alıp PD kontrolü yapmak ve arayüze veri basmak.
void controlThreadFunc() {
    while (isRunning) {
        float cx, cy;
        bool found;
        {
            lock_guard<mutex> lock(dataMutex);
            cx = targetX; cy = targetY; found = faceFound;
        }

        // Dashboard için JSON yaz
        FILE* f = fopen("/tmp/tracker_state.json", "w");
        if (f) {
            fprintf(f, "{\"cx\": %.2f, \"cy\": %.2f, \"face_found\": %s}", 
                    cx, cy, found ? "true" : "false");
            fclose(f);
        }
        this_thread::sleep_for(chrono::milliseconds(20));
    }
}


// --- ANA FONKSİYON ---
int main() {
    cout << "[INFO] Tracker Başlıyor..." << endl;

    thread t_cam(cameraThreadFunc);
    thread t_ai(aiThreadFunc);    
    thread t_ctrl(controlThreadFunc);

    while(isRunning) {
        this_thread::sleep_for(chrono::seconds(1));
    }
    return 0;
}