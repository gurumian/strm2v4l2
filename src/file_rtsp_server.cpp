#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <iostream>

int main(int argc, char* argv[])
{
    gst_init(&argc, &argv);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <video-file>\n";
        return 1;
    }

    // Convert the supplied path to an absolute path.
    gchar* absolute_path = g_canonicalize_filename(argv[1], nullptr);

    if (!g_file_test(absolute_path, G_FILE_TEST_IS_REGULAR)) {
        std::cerr << "File not found: " << absolute_path << '\n';
        g_free(absolute_path);
        return 1;
    }

    // Convert the local filename to a properly escaped file:// URI.
    GError* error = nullptr;
    gchar* file_uri = gst_filename_to_uri(absolute_path, &error);

    if (file_uri == nullptr) {
        std::cerr << "Failed to create file URI: "
                  << (error ? error->message : "unknown error")
                  << '\n';

        g_clear_error(&error);
        g_free(absolute_path);
        return 1;
    }

    /*
     * Decode the input file regardless of its original codec, then:
     *
     * decoded video
     *   -> raw I420 at 30 FPS
     *   -> H.264
     *   -> RTP
     *
     * The RTSP server requires the RTP payloader to be named pay0.
     */
    gchar* launch_description = g_strdup_printf(
        "( "
        "uridecodebin uri=\"%s\" name=decoder "
        "decoder. ! queue "
        "! video/x-raw "
        "! videoconvert "
        "! videoscale "
        "! videorate "
        "! video/x-raw,format=I420,framerate=30/1 "
        "! x264enc "
        "    tune=zerolatency "
        "    speed-preset=ultrafast "
        "    bitrate=2500 "
        "    key-int-max=30 "
        "! h264parse "
        "! rtph264pay "
        "    name=pay0 "
        "    pt=96 "
        "    config-interval=1 "
        ")",
        file_uri
    );

    GstRTSPServer* server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", nullptr);

    GstRTSPMountPoints* mounts =
        gst_rtsp_server_get_mount_points(server);

    GstRTSPMediaFactory* factory =
        gst_rtsp_media_factory_new();

    gst_rtsp_media_factory_set_launch(
        factory,
        launch_description
    );

    /*
     * FALSE means each connection receives its own pipeline and starts
     * reading the file from the beginning.
     */
    gst_rtsp_media_factory_set_shared(factory, FALSE);

    gst_rtsp_mount_points_add_factory(
        mounts,
        "/test",
        factory
    );

    g_object_unref(mounts);

    const guint server_id = gst_rtsp_server_attach(server, nullptr);

    if (server_id == 0) {
        std::cerr << "Failed to attach the RTSP server.\n";

        g_object_unref(server);
        g_free(launch_description);
        g_free(file_uri);
        g_free(absolute_path);

        return 1;
    }

    std::cout
        << "Input file: " << absolute_path << '\n'
        << "RTSP URL:  rtsp://127.0.0.1:8554/test\n"
        << "Press Ctrl+C to stop.\n";

    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(server);

    g_free(launch_description);
    g_free(file_uri);
    g_free(absolute_path);

    return 0;
}
