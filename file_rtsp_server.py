#!/usr/bin/env python3

import os
import sys

import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstRtspServer", "1.0")

from gi.repository import GLib, Gst, GstRtspServer


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <h264-video.mp4>")
        return 1

    filename = os.path.abspath(sys.argv[1])

    if not os.path.isfile(filename):
        print(f"File not found: {filename}", file=sys.stderr)
        return 1

    # Escape the filename for the GStreamer launch description.
    escaped_filename = filename.replace("\\", "\\\\").replace('"', '\\"')

    Gst.init(None)

    server = GstRtspServer.RTSPServer()
    server.set_service("8554")

    factory = GstRtspServer.RTSPMediaFactory()
    factory.set_shared(True)

    factory.set_launch(
        "("
        f' filesrc location="{escaped_filename}"'
        " ! qtdemux name=demux"
        " demux. ! queue"
        " ! h264parse"
        " ! rtph264pay name=pay0 pt=96 config-interval=1"
        ")"
    )

    mounts = server.get_mount_points()
    mounts.add_factory("/test", factory)

    if server.attach(None) == 0:
        print("Failed to start the RTSP server", file=sys.stderr)
        return 1

    print("RTSP server started")
    print(f"Input: {filename}")
    print("URL:   rtsp://127.0.0.1:8554/test")
    print("Press Ctrl+C to stop")

    loop = GLib.MainLoop()

    try:
        loop.run()
    except KeyboardInterrupt:
        print("\nStopping RTSP server")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())