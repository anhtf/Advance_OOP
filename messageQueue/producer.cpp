#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <linux/videodev2.h>

#define DEVICE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define MAX_SIZE 250000

struct msg_buffer {
    long msg_type;
    int frame_size;
    unsigned char frame_data[MAX_SIZE];
};

struct buffer {
    void *start;
    size_t length;
};

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) return 1;

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    ioctl(fd, VIDIOC_S_FMT, &fmt);

    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 30;
    ioctl(fd, VIDIOC_S_PARM, &parm);

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ioctl(fd, VIDIOC_REQBUFS, &req);

    struct buffer *buffers = (struct buffer*)calloc(req.count, sizeof(*buffers));
    for (int i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl(fd, VIDIOC_QUERYBUF, &buf);
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        ioctl(fd, VIDIOC_QBUF, &buf);
    }

    key_t key = ftok("/tmp", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    struct msg_buffer message;
    message.msg_type = 1;

    while (true) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) break;

        message.frame_size = buf.bytesused;
        memcpy(message.frame_data, buffers[buf.index].start, buf.bytesused);

        msgsnd(msgid, &message, sizeof(message.frame_size) + message.frame_size, 0);

        ioctl(fd, VIDIOC_QBUF, &buf);
    }

    ioctl(fd, VIDIOC_STREAMOFF, &type);
    for (int i = 0; i < req.count; ++i) munmap(buffers[i].start, buffers[i].length);
    free(buffers);
    close(fd);
    msgctl(msgid, IPC_RMID, NULL);
    return 0;
}