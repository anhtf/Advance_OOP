#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>

int main(int argc, char *argv[]) {
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    // 1. Khởi tạo GStreamer
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    // 2. Tạo một RTSP Server instance (mặc định chạy ở port 8554)
    server = gst_rtsp_server_new();

    // 3. Lấy đối tượng quản lý đường dẫn (Mount Points)
    mounts = gst_rtsp_server_get_mount_points(server);

    // 4. Tạo một Media Factory để sản xuất luồng video
    factory = gst_rtsp_media_factory_new();

    // 5. CẤU HÌNH ĐƯỜNG ỐNG (PIPELINE) SIÊU QUAN TRỌNG
    // - v4l2src: Đọc từ camera /dev/video0
    // - videoconvert: Chuyển đổi không gian màu phù hợp
    // - video/x-raw...: Ép độ phân giải 640x480 và 30 fps
    // - x264enc: Nén phần mềm H.264 (cấu hình siêu nhanh, không độ trễ)
    // - rtph264pay: Đóng gói H.264 thành các gói tin RTP để gửi qua mạng
    const char *pipeline = 
        "( v4l2src device=/dev/video0 ! "
        "videoconvert ! "
        "video/x-raw,width=640,height=480,framerate=30/1 ! "
        "x264enc speed-preset=ultrafast tune=zerolatency ! "
        "rtph264pay name=pay0 pt=96 )";

    gst_rtsp_media_factory_set_launch(factory, pipeline);

    // Cho phép nhiều người vào xem cùng lúc luồng này (Share stream)
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    // 6. Gắn luồng video vào đường dẫn "/camera" (rtsp://IP:8554/camera)
    gst_rtsp_mount_points_add_factory(mounts, "/camera", factory);
    g_object_unref(mounts);

    // 7. Chạy server và lắng nghe kết nối
    gst_rtsp_server_attach(server, NULL);
    std::cout << "RTSP Server is running at rtsp://127.0.0.1:8554/camera\n";
    
    // Giữ cho chương trình chạy liên tục (Event Loop)
    g_main_loop_run(loop);

    return 0;
}