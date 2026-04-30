#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Opened %s\n", DEVICE);

    // -----------------------------
    // Set format: MJPEG 640x480
    // -----------------------------
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT");
        close(fd);
        return 1;
    }

    printf("Format set: %dx%d MJPEG\n", WIDTH, HEIGHT);

    // -----------------------------
    // Request buffer
    // -----------------------------
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS");
        close(fd);
        return 1;
    }

    // -----------------------------
    // Query buffer
    // -----------------------------
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        perror("VIDIOC_QUERYBUF");
        close(fd);
        return 1;
    }

    printf("Buffer length: %u bytes\n", buf.length);

    // -----------------------------
    // mmap buffer
    // -----------------------------
    void *buffer = mmap(NULL,
                        buf.length,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        buf.m.offset);

    if (buffer == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // -----------------------------
    // Queue buffer
    // -----------------------------
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        perror("VIDIOC_QBUF");
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    // -----------------------------
    // Start streaming
    // -----------------------------
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    printf("Streaming started...\n");

    // -----------------------------
    // Dequeue filled buffer
    // -----------------------------
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("VIDIOC_DQBUF");
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    printf("Captured frame: %u bytes\n", buf.bytesused);

    // -----------------------------
    // Save JPEG
    // -----------------------------
    FILE *fp = fopen("image.jpg", "wb");
    if (!fp) {
        perror("fopen");
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    fwrite(buffer, buf.bytesused, 1, fp);
    fclose(fp);

    printf("Saved image.jpg\n");

    // -----------------------------
    // Stop streaming
    // -----------------------------
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("VIDIOC_STREAMOFF");
    }

    munmap(buffer, buf.length);
    close(fd);

    printf("Done.\n");
    return 0;
}