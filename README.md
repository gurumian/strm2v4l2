# strm2v4l2

Stream a local video file over RTSP using GStreamer's native RTSP server. The C++ server decodes almost any input format, re-encodes the video to H.264, and publishes it at:

```text
rtsp://127.0.0.1:8554/test
```

This is useful for testing RTSP clients and RTSP-to-V4L2 pipelines (for example, feeding a virtual webcam via `v4l2loopback`).

## Prerequisites

### Build tools

| Package | Purpose |
|---------|---------|
| `build-essential` | C++ compiler and linker |
| `cmake` | Build system (3.16 or newer) |
| `pkg-config` | Discover GStreamer libraries at configure time |

### GStreamer development libraries

| Package | pkg-config module | Purpose |
|---------|-------------------|---------|
| `libgstreamer1.0-dev` | `gstreamer-1.0` | Core GStreamer API |
| `libgstrtspserver-1.0-dev` | `gstreamer-rtsp-server-1.0` | RTSP server library |

### GStreamer runtime plugins

These are required at **run time** to decode input files, encode H.264, and packetize RTP:

| Package | Purpose |
|---------|---------|
| `gstreamer1.0-tools` | `gst-launch-1.0` for testing |
| `gstreamer1.0-plugins-base` | Base elements (`videoconvert`, `videoscale`, etc.) |
| `gstreamer1.0-plugins-good` | Good-quality plugins |
| `gstreamer1.0-plugins-bad` | Additional codecs and elements |
| `gstreamer1.0-plugins-ugly` | `x264enc` H.264 encoder |
| `gstreamer1.0-libav` | FFmpeg-based decoders (`uridecodebin`) |

### Optional: RTSP to V4L2 testing

To expose the RTSP stream as a virtual webcam:

| Requirement | Purpose |
|-------------|---------|
| `v4l2loopback` kernel module | Creates `/dev/videoN` loopback devices |
| `gstreamer1.0-plugins-good` | `v4l2sink` element |

Load the module (adjust `video_nr` as needed):

```bash
sudo modprobe v4l2loopback \
    devices=1 \
    video_nr=10 \
    card_label="RTSP Test Camera" \
    exclusive_caps=1
```

## Install dependencies (Ubuntu / Debian)

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libgstreamer1.0-dev \
    libgstrtspserver-1.0-dev \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav
```

Verify that pkg-config can find the RTSP server library:

```bash
pkg-config --modversion gstreamer-rtsp-server-1.0
```

If this command fails, install `libgstrtspserver-1.0-dev`.

## Build

```bash
cmake -S . -B build
cmake --build build
```

The executable is written to `build/file_rtsp_server`.

## Run

Pass the path to any supported video file. Quote paths that contain spaces:

```bash
./build/file_rtsp_server "/path/to/your/video.mp4"
```

Expected output:

```text
Input file: /path/to/your/video.mp4
RTSP URL:  rtsp://127.0.0.1:8554/test
Press Ctrl+C to stop.
```

Each RTSP client connection gets its own pipeline and starts reading the file from the beginning. The stream does **not** loop after the file ends.

## Verify the RTSP stream

In another terminal, play the stream with GStreamer:

```bash
gst-launch-1.0 -v \
    rtspsrc location=rtsp://127.0.0.1:8554/test \
        protocols=tcp latency=100 \
    ! rtph264depay \
    ! h264parse \
    ! avdec_h264 \
    ! videoconvert \
    ! autovideosink
```

## RTSP to V4L2

After loading `v4l2loopback` (see [Prerequisites](#optional-rtsp-to-v4l2-testing)), pipe the RTSP stream into the virtual device:

```bash
gst-launch-1.0 -e \
    rtspsrc location=rtsp://127.0.0.1:8554/test \
        protocols=tcp latency=100 \
    ! rtph264depay \
    ! h264parse \
    ! avdec_h264 \
    ! videoconvert \
    ! video/x-raw,format=YUY2 \
    ! v4l2sink device=/dev/video10 sync=false
```

Other applications can then open `/dev/video10` as a camera input.

## Python alternative

The repository also includes `file_rtsp_server.py`, a Python prototype that requires GObject introspection bindings (`python3-gi`, `gir1.2-gst-rtsp-server-1.0`). The C++ version is the recommended approach because it links directly against `libgstrtspserver` without Python dependencies.

## Project layout

```text
strm2v4l2/
├── CMakeLists.txt
├── README.md
├── file_rtsp_server.py
└── src/
    └── file_rtsp_server.cpp
```
