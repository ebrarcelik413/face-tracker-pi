#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include "cloud_service.hpp"
#include "control_service.hpp"
#include "kalman_tracker.hpp"

using namespace cv;
using namespace std;

// ─── Sabitler ───────────────────────────────────────────────────────
const string CLOUD_IP = "35.157.209.216";
const int    CLOUD_PORT   = 5001;
const string SNAPSHOT_TMP = "/tmp/snapshot_tmp.jpg";
const string SNAPSHOT_PATH= "/tmp/snapshot.jpg";
const string STATE_PATH   = "/tmp/tracker_state.json";

// ─── Paylaşılan durum ────────────────────────────────────────────────
Mat          g_frame;
mutex        g_frameMtx;
bool         g_newFrame = false;
FaceResult   g_result;
mutex        g_resultMtx;
atomic<bool> g_running(true);
atomic<bool> g_cloudOk(false);

// ─── State JSON yaz ─────────────────────────────────────────────────
void writeState(const FaceResult& r, float panPW, float tiltPW) {
    FILE* f = fopen(STATE_PATH.c_str(), "w");
    if (!f) return;
    fprintf(f,
        "{\"cx\":%.2f,\"cy\":%.2f,\"raw_cx\":%.0f,\"raw_cy\":%.0f,"
        "\"face_found\":%s,\"confidence\":%.3f,\"lat_total\":%.1f,"
        "\"pan_pw\":%.0f,\"tilt_pw\":%.0f,\"cloud_connected\":%s}",
        r.cx, r.cy, r.cx, r.cy,
        r.found?"true":"false",
        r.confidence, r.latency_ms,
        panPW, tiltPW,
        g_cloudOk.load()?"true":"false"
    );
    fclose(f);
}

// ─── Kamera Thread ───────────────────────────────────────────────────
void cameraThread() {
    VideoCapture cap(0, CAP_V4L2);
    cap.set(CAP_PROP_FRAME_WIDTH,  640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS,          20);
    cap.set(CAP_PROP_BUFFERSIZE,   1);

    if (!cap.isOpened()) {
        cerr << "[HATA] Kamera açılamadı!" << endl;
        g_running = false; return;
    }
    cout << "[OK] Kamera açıldı" << endl;

    Mat raw;
    int cnt = 0;
    while (g_running) {
        cap >> raw;
        if (raw.empty()) continue;

        Mat bgr;
        cvtColor(raw, bgr, COLOR_RGB2BGR);

        { lock_guard<mutex> lk(g_frameMtx); bgr.copyTo(g_frame); g_newFrame=true; }

        if (cnt++ % 2 == 0) {
            Mat disp = bgr.clone();
            FaceResult r; float panPW=1500, tiltPW=1500;
            {
                lock_guard<mutex> lk(g_resultMtx);
                r = g_result;
            }
            if (r.found) {
                rectangle(disp, Rect((int)r.x,(int)r.y,(int)r.w,(int)r.h),
                          Scalar(0,255,0), 2);
                circle(disp, Point((int)r.cx,(int)r.cy), 5, Scalar(0,0,255), -1);
                putText(disp, "cx:"+to_string((int)r.cx)+" cy:"+to_string((int)r.cy),
                        Point((int)r.x,(int)r.y-8),
                        FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,255,0), 1);
            }
            string status = g_cloudOk ? "CLOUD:OK" : "CLOUD:OFF";
            Scalar color  = g_cloudOk ? Scalar(0,255,0) : Scalar(0,0,255);
            putText(disp, status, Point(5,25),
                    FONT_HERSHEY_SIMPLEX, 0.7, color, 2);

            imwrite(SNAPSHOT_TMP, disp, {IMWRITE_JPEG_QUALITY, 80});
            rename(SNAPSHOT_TMP.c_str(), SNAPSHOT_PATH.c_str());
        }
    }
    cap.release();
}

// ─── Cloud + Kontrol Thread ──────────────────────────────────────────
void cloudControlThread() {
    CloudService   cloud(CLOUD_IP, CLOUD_PORT);
    ControlService ctrl;
    KalmanTracker  kalman;

    Ptr<FaceDetectorYN> localDetector;
    try {
        localDetector = FaceDetectorYN::create(
            "/home/pi/face_tracker/face_detection_yunet_2023mar.onnx",
            "", Size(320, 240), 0.6f, 0.3f, 5000);
        cout << "[OK] Local YuNet yüklendi (fallback)" << endl;
    } catch (...) {
        cerr << "[WARN] Local model yüklenemedi" << endl;
    }

    cout << "[INFO] Cloud: " << CLOUD_IP << ":" << CLOUD_PORT << endl;

    Mat frame;
    while (g_running) {
        bool hasNew = false;
        {
            lock_guard<mutex> lk(g_frameMtx);
            if (g_newFrame && !g_frame.empty()) {
                g_frame.copyTo(frame); g_newFrame=false; hasNew=true;
            }
        }
        if (!hasNew) { this_thread::sleep_for(chrono::milliseconds(5)); continue; }

        vector<unsigned char> jpegBuf;
        imencode(".jpg", frame, jpegBuf, {IMWRITE_JPEG_QUALITY, 70});

        FaceResult res = cloud.detect(jpegBuf);
        g_cloudOk = cloud.isConnected();

        if (!g_cloudOk && localDetector) {
            Mat blob;
            resize(frame, blob, Size(320, 240));
            Mat faces;
            localDetector->setInputSize(blob.size());
            localDetector->detect(blob, faces);
            if (!faces.empty()) {
                float sx = 640.0f/320.0f, sy = 480.0f/240.0f;
                res.x = faces.at<float>(0,0)*sx;
                res.y = faces.at<float>(0,1)*sy;
                res.w = faces.at<float>(0,2)*sx;
                res.h = faces.at<float>(0,3)*sy;
                res.cx = res.x + res.w/2.0f;
                res.cy = res.y + res.h/2.0f;
                res.confidence = faces.at<float>(0,14);
                res.found = true;
            }
        }

        // Kalman güncelle veya tahmin et
        if (res.found) {
            kalman.update(res);
        } else if (kalman.isActive()) {
            FaceResult predicted = kalman.predict();
            if (predicted.found) {
                res = predicted;
                res.confidence = 0.5f; // tahmin olduğunu belirt
            }
        }

        ctrl.update(res);
        { lock_guard<mutex> lk(g_resultMtx); g_result = res; }
        writeState(res, ctrl.getPanPW(), ctrl.getTiltPW());

        this_thread::sleep_for(chrono::milliseconds(20));
    }
}

// ─── Ana Fonksiyon ───────────────────────────────────────────────────
int main() {
    cout << "[INFO] Face Tracker başlıyor (Cloud Mod)..." << endl;

    thread tCam(cameraThread);
    thread tCloud(cloudControlThread);

    while (g_running)
        this_thread::sleep_for(chrono::seconds(1));

    tCam.join();
    tCloud.join();
    return 0;
}