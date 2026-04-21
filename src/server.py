import json
import time
import os
import io
import threading
from flask import Flask, Response, jsonify, request, send_file

app = Flask(__name__)

STATE_FILE  = "/tmp/tracker_state.json"
FRAME_FILE  = "/tmp/snapshot.jpg"  
SNAP_FILE   = "/tmp/snapshot.jpg"
target_path = "/tmp/snapshot.jpg"

# AES toggle Flask tarafında tutulur,
# C++ her döngüde state.json'dan okur
aes_override = {"active": True}

# Latency geçmişi (son 30 ölçüm)
lat_history = {"values": [], "lock": threading.Lock()}

# ── Picamera2 canlı akış ──
from picamera2 import Picamera2

live_cam = {"instance": None, "index": -1, "lock": threading.Lock()}

def start_live_cam(cam_idx):
    """Belirtilen kamera indeksini aç (0 veya 1)"""
    with live_cam["lock"]:
        if live_cam["instance"] is not None:
            try:
                live_cam["instance"].stop()
                live_cam["instance"].close()
            except:
                pass
            live_cam["instance"] = None
        cam = Picamera2(cam_idx)
        cam.configure(cam.create_video_configuration(main={"size": (640, 480)}))
        cam.start()
        live_cam["instance"] = cam
        live_cam["index"] = cam_idx

def gen_live():
    """Picamera2'den doğrudan MJPEG akışı (diske yazmadan)"""
    while True:
        with live_cam["lock"]:
            cam = live_cam["instance"]
        if cam is None:
            time.sleep(0.1)
            continue
        try:
            buf = io.BytesIO()
            cam.capture_file(buf, format="jpeg")
            data = buf.getvalue()
            yield (b"--frame\r\n"
                   b"Content-Type: image/jpeg\r\n\r\n"
                   + data + b"\r\n")
        except:
            time.sleep(0.05)

def read_state():
    try:
        with open(STATE_FILE) as f:
            s = json.load(f)
        s["aes_active"] = aes_override["active"]
        # Latency geçmişine ekle
        if "lat_total" in s:
            with lat_history["lock"]:
                lat_history["values"].append(round(s["lat_total"], 1))
                lat_history["values"] = lat_history["values"][-30:]
        # Min / max / avg / jitter hesapla
        with lat_history["lock"]:
            vals = list(lat_history["values"])
        if vals:
            s["lat_min"]    = round(min(vals), 1)
            s["lat_max"]    = round(max(vals), 1)
            s["lat_avg"]    = round(sum(vals) / len(vals), 1)
            s["lat_jitter"] = round(max(vals) - min(vals), 1)
        else:
            s["lat_min"] = s["lat_max"] = s["lat_avg"] = s["lat_jitter"] = "—"
        return s
    except:
        return {"error": "C++ sureci henuz baslamadi"}

def gen_frames():
    """snapshot.jpg'yi sürekli oku, MJPEG olarak aktar - INFINITE"""
    while True:
        try:
            if os.path.exists(FRAME_FILE):
                with open(FRAME_FILE, "rb") as f:
                    frame_bytes = f.read()
                if frame_bytes and len(frame_bytes) > 100:
                    yield (b"--frame\r\n"
                           b"Content-Type: image/jpeg\r\n"
                           b"Content-Length: " + str(len(frame_bytes)).encode() + b"\r\n\r\n"
                           + frame_bytes + b"\r\n")
            time.sleep(0.03)
        except Exception as e:
            print(f"[ERROR] {e}")
            time.sleep(0.1)

@app.route("/")
def index():
    with open(os.path.join(os.path.dirname(__file__), "index.html")) as f:
        return f.read()

@app.route("/video")
def video():
    return Response(gen_frames(),
                    mimetype="multipart/x-mixed-replace; boundary=frame")

@app.route("/live")
def live():
    """Picamera2 doğrudan canlı akış (diske kaydetmeden)"""
    cam_idx = request.args.get("cam", None)
    if cam_idx is not None:
        cam_idx = int(cam_idx)
        if cam_idx != live_cam["index"]:
            start_live_cam(cam_idx)
    elif live_cam["instance"] is None:
        start_live_cam(0)
    return Response(gen_live(),
                    mimetype="multipart/x-mixed-replace; boundary=frame")

@app.route("/switch_cam", methods=["POST"])
def switch_cam():
    """Kamera değiştir (0 veya 1)"""
    cam_idx = int(request.json.get("cam", 0))
    try:
        start_live_cam(cam_idx)
        return jsonify({"ok": True, "cam": cam_idx})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)}), 400

@app.route("/cam_info")
def cam_info():
    return jsonify({"active_cam": live_cam["index"]})

@app.route("/state")
def state():
    return jsonify(read_state())

@app.route("/toggle_aes", methods=["POST"])
def toggle_aes():
    aes_override["active"] = not aes_override["active"]
    return jsonify({"aes_active": aes_override["active"]})

@app.route("/set_model", methods=["POST"])
def set_model():
    model = request.json.get("model", "yunet")
    if model not in ("yunet", "haar", "lbp"):
        model = "yunet"
    with open("/tmp/tracker_model.txt", "w") as f:
        f.write(model)
    return jsonify({"model": model})

@app.route("/get_model")
def get_model():
    try:
        with open("/tmp/tracker_model.txt") as f:
            model = f.read().strip()
    except:
        model = "yunet"
    return jsonify({"model": model})

@app.route("/set_fps", methods=["POST"])
def set_fps():
    fps = int(request.json.get("fps", 15))
    fps = max(1, min(30, fps))
    with open("/tmp/tracker_fps.txt", "w") as f:
        f.write(str(fps))
    return jsonify({"fps": fps})

@app.route("/get_fps")
def get_fps():
    try:
        with open("/tmp/tracker_fps.txt") as f:
            fps = int(f.read().strip())
    except:
        fps = 15
    return jsonify({"fps": fps})

@app.route("/snapshot", methods=["POST"])
def snapshot():
    if os.path.exists(FRAME_FILE):
        import shutil
        shutil.copy(FRAME_FILE, SNAP_FILE)
        return jsonify({"ok": True, "time": time.strftime("%H:%M:%S"), "path": SNAP_FILE})
    return jsonify({"ok": False})

@app.route("/download_snapshot")
def download_snapshot():
    if os.path.exists(SNAP_FILE):
        return send_file(SNAP_FILE, as_attachment=True,
                         download_name="snapshot.jpg")
    return "Snapshot yok", 404

@app.route("/latencies")
def latencies():
    with lat_history["lock"]:
        return jsonify(list(lat_history["values"]))

if __name__ == "__main__":
    print("Dashboard: http://0.0.0.0:5000")
    app.run(host="0.0.0.0", port=5000, threaded=True)

    