#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <chrono>

using namespace cv;
using namespace std::chrono;

int main() {
    // Haar Cascade yükle
    CascadeClassifier face_cascade;
    std::string cascade_path =
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml";

    if (!face_cascade.load(cascade_path)) {
        std::cerr << "HATA: Cascade dosyasi bulunamadi: " << cascade_path << "\n";
        std::cerr << "Alternatif: find /usr -name 'haarcascade_frontalface*' 2>/dev/null\n";
        return -1;
    }

    // Kamerayı aç
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "HATA: Kamera acilamadi.\n";
        return -1;
    }

    cap.set(CAP_PROP_FRAME_WIDTH,  640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS, 30);

    std::cout << "Kamera acildi. 'q' ile cik.\n";

    std::vector<double> latencies;
    Mat frame, gray;

    while (true) {
        auto t0 = high_resolution_clock::now();

        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Kare alinamadi.\n";
            break;
        }

        // Gri tonlamaya çevir — cascade için gerekli
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        equalizeHist(gray, gray);  // kontrast iyileştir

        // Yüz tespiti
        std::vector<Rect> faces;
        face_cascade.detectMultiScale(
            gray, faces,
            1.1,   // scaleFactor
            5,     // minNeighbors — düşürürsen daha hassas ama daha fazla yanlış pozitif
            0,
            Size(60, 60)   // minSize
        );

        int img_cx = frame.cols / 2;
        int img_cy = frame.rows / 2;

        // Görüntü merkezi — servo'nun hedefi bu nokta
        drawMarker(frame, Point(img_cx, img_cy),
                   Scalar(200, 200, 200), MARKER_CROSS, 20, 1);

        for (const auto& face : faces) {
            int fx = face.x + face.width  / 2;  // yüz merkezi x
            int fy = face.y + face.height / 2;  // yüz merkezi y
            int ex = fx - img_cx;               // yatay hata (servo bunu sıfırlayacak)
            int ey = fy - img_cy;               // dikey hata

            // Bounding box
            rectangle(frame, face, Scalar(0, 220, 100), 2);

            // Yüz merkezi
            circle(frame, Point(fx, fy), 5, Scalar(0, 220, 100), -1);

            // Hata vektörü — merkezden yüze çizgi
            line(frame, Point(img_cx, img_cy), Point(fx, fy),
                 Scalar(50, 180, 255), 1);

            // Hata değerleri
            std::string err_text = "ex=" + std::to_string(ex) +
                                   "px  ey=" + std::to_string(ey) + "px";
            putText(frame, err_text,
                    Point(face.x, face.y - 10),
                    FONT_HERSHEY_SIMPLEX, 0.55, Scalar(0, 220, 100), 1);
        }

        // Gecikme hesapla
        auto t1  = high_resolution_clock::now();
        double dt_ms = duration<double, std::milli>(t1 - t0).count();
        latencies.push_back(dt_ms);

        double fps = 1000.0 / dt_ms;

        // Ekran bilgisi
        putText(frame,
                "FPS: " + std::to_string((int)fps) +
                "  delay: " + std::to_string((int)dt_ms) + "ms",
                Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.6,
                Scalar(255, 255, 255), 1);
        putText(frame,
                "Yuz: " + std::to_string(faces.size()),
                Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.6,
                Scalar(255, 255, 255), 1);

        imshow("Face Tracker Demo", frame);
        if ((waitKey(1) & 0xFF) == 'q') break;
    }

    cap.release();
    destroyAllWindows();

    // Latency istatistikleri — tez için veri
    if (!latencies.empty()) {
        double sum  = std::accumulate(latencies.begin(), latencies.end(), 0.0);
        double mean = sum / latencies.size();
        double mn   = *std::min_element(latencies.begin(), latencies.end());
        double mx   = *std::max_element(latencies.begin(), latencies.end());

        std::vector<double> sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        double p95 = sorted[(int)(sorted.size() * 0.95)];

        // Std sapma
        double sq_sum = 0;
        for (double v : latencies) sq_sum += (v - mean) * (v - mean);
        double stddev = std::sqrt(sq_sum / latencies.size());

        std::cout << "\n--- Gecikme Ozeti ---\n";
        std::cout << "Olcum sayisi : " << latencies.size() << "\n";
        std::cout << "Ortalama     : " << mean   << " ms\n";
        std::cout << "Min          : " << mn     << " ms\n";
        std::cout << "Max          : " << mx     << " ms\n";
        std::cout << "Std sapma    : " << stddev << " ms\n";
        std::cout << "P95          : " << p95    << " ms\n";
    }

    return 0;
}