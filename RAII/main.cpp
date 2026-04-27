/*
- Trong C, con trỏ có thể nhận giá trị NULL, thực hiện phép toán số học và linh hoạt thay đổi địa chỉ vùng nhớ
- Trong C++, "tham chiếu" như 1 biến an toàn hơn khi: bắt buộc khởi tạo, Không bao giờ mang giá trị NULL và không thể rebind sang địa chỉ khác sau khi đã khởi tạo.

==> Resource Acquisition IS Initialization : tài nguyên được cấp phát trong hàm tạo và bắt buộc phải được giải phóng trong hàm hủy.

- Hiện thân là smart_pointers thay thế hoàn toàn delete và new. std::unique_ptr và std::shared_ptr
*/

#include <vector>
#include <memory>
#include <cstdlib>
#include <string>
#include "stdint.h"

#pragma pack(1)
struct DeviceContext {
    std::string endpoint_ip;
    uint8_t firmware_uid[4];
    bool link_established;

    DeviceContext(std::string ip) : endpoint_ip(ip), firmware_uid{0x00, 0x00, 0x00, 0x00}, link_established(false) {}
    ~DeviceContext() {}
};
#pragma pop

void legacy_c_allocation() {
    DeviceContext* ctx = (DeviceContext*)malloc(sizeof(DeviceContext));
    if (ctx) {
        ctx->firmware_uid[0] = 0x09;
        ctx->firmware_uid[1] = 0x55;
        ctx->firmware_uid[2] = 0xBD;
        ctx->firmware_uid[3] = 0x02;
        //free(ctx);
    }
}

void legacy_cpp_allocation() {
    DeviceContext* ctx = new DeviceContext("192.168.7.2");
    ctx->link_established = true;
    delete ctx;
}

void modern_cpp_allocation() {
    std::vector<std::unique_ptr<DeviceContext>> node_pool;
    node_pool.reserve(100);

    for (int i = 0; i < 100; ++i) {
        node_pool.push_back(std::make_unique<DeviceContext>("192.168.7.2"));
    }

    node_pool[0]->firmware_uid[0] = 0x75;
    node_pool[0]->firmware_uid[1] = 0xEE;
    node_pool[0]->firmware_uid[2] = 0xC9;
    node_pool[0]->firmware_uid[3] = 0x01;
    node_pool[0]->link_established = true;
}

int main() {
    legacy_c_allocation();
    legacy_cpp_allocation();
    modern_cpp_allocation();
    return 0;
}
