#include <iostream>
#include <vector>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <opencv2/opencv.hpp>

#define MAX_SIZE 250000

struct msg_buffer {
    long msg_type;
    int frame_size;
    unsigned char frame_data[MAX_SIZE];
};

int main() {
    key_t key = ftok("/tmp", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);

    struct msg_buffer message;

    cv::namedWindow("Stream", cv::WINDOW_AUTOSIZE);

    while (true) {
        if (msgrcv(msgid, &message, sizeof(message) - sizeof(long), 1, 0) > 0) {
            std::vector<unsigned char> data(message.frame_data, message.frame_data + message.frame_size);
            cv::Mat frame = cv::imdecode(data, cv::IMREAD_COLOR);
            
            if (!frame.empty()) {
                cv::imshow("Stream", frame);
                if (cv::waitKey(1) == 27) break;
            }
        }
    }

    cv::destroyAllWindows();
    return 0;
}