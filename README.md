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
| `gstreamer1.0-plugins-good` | `v4l2sink` and `v4l2src` elements |
| `v4l-utils` (optional) | `v4l2-ctl` to inspect loopback devices and formats |
| `ffmpeg` (optional) | `ffplay` to preview the virtual camera |
| `vlc` (optional) | GUI preview of the virtual camera |

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

This section turns the RTSP stream into a virtual webcam at `/dev/video10`. You need **three terminals**: one for the RTSP server, one to feed the loopback device, and one to preview it.

### Step 1 — Load the loopback module

See [Optional: RTSP to V4L2 testing](#optional-rtsp-to-v4l2-testing) for `modprobe` options. Quick load:

```bash
sudo modprobe v4l2loopback \
    devices=1 \
    video_nr=10 \
    card_label="RTSP Test Camera" \
    exclusive_caps=1
```

Confirm the device exists:

```bash
v4l2-ctl --list-devices
```

(`v4l-utils` package provides `v4l2-ctl`.)

### Step 2 — Start the RTSP server

Terminal 1:

```bash
./build/file_rtsp_server "/path/to/your/video.mp4"
```

### Step 3 — Feed RTSP into the loopback device

Terminal 2 — keep this running while you preview or use the virtual camera:

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

### Step 4 — Play the virtual loopback device

Terminal 3 — preview `/dev/video10` like any V4L2 camera.

**GStreamer** (uses packages already installed above):

```bash
gst-launch-1.0 -v \
    v4l2src device=/dev/video10 \
    ! videoconvert \
    ! autovideosink
```

**FFmpeg** (`ffmpeg` package):

```bash
ffplay -f v4l2 -input_format yuyv422 /dev/video10
```

If playback fails, list supported formats and pick one that matches what `v4l2sink` writes (YUY2 / `yuyv422`):

```bash
v4l2-ctl -d /dev/video10 --list-formats-ext
```

**VLC** (`vlc` package):

```bash
vlc v4l2:///dev/video10
```

Or open VLC → **Media** → **Open Capture Device** → capture mode **Video camera**, device `/dev/video10`.

Any application that accepts a V4L2 camera (OBS, browsers via PipeWire, etc.) can select **RTSP Test Camera** or `/dev/video10` while the RTSP-to-loopback pipeline in terminal 2 is running.

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
