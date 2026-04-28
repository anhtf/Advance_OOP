/*
- Trong lập trình C truyền thống, khi nhiều threads tranh giành đọc ghi vào 1 tài nguyên phần cứng -> pthread_mutex_lock() và pthread_mutex_unlock() được sử dụng để đảm bảo tính nhất quán của dữ liệu. Tuy nhiên, nếu một thread bị lỗi hoặc quên unlock mutex, sẽ dẫn đến deadlock hoặc resource leak
- Nếu trong lúc tài nguyên bị khóa, luồng thực thi bị văng ra ngoài hoặc segmentfault => unlock mãi mãi k dc gọi => Deadlock

===> RAII quản lý đa luồng thay vì gọi các hàm thủ công là tiêu chuẩn bắt buộc.
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <chrono>

// 1. Tài nguyên chia sẻ toàn cục
std::mutex i2c_bus_mutex;
int nfc_read_count = 0;

void read_pn532_nfc_data(int thread_id) {
    try {
        // 2. Áp dụng RAII để khóa Mutex
        std::lock_guard<std::mutex> lock(i2c_bus_mutex);

        // --- Bắt đầu vùng tranh chấp (Critical Section) ---
        std::cout << "[Luồng " << thread_id << "] Đang chiếm quyền I2C bus...\n";
        
        // 3. Giả lập độ trễ khi giao tiếp phần cứng
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        nfc_read_count++;

        // 4. Giả lập lỗi phần cứng ngẫu nhiên ở luồng số 3
        if (thread_id == 3) {
            throw std::runtime_error("Mất kết nối I2C với module PN532!");
        }

        std::cout << "[Luồng " << thread_id << "] Đọc thành công. Số lần đọc: " << nfc_read_count << "\n";
        // --- Kết thúc vùng tranh chấp ---

    } catch (const std::exception& e) {
        std::cout << "[Luồng " << thread_id << "] Lỗi: " << e.what() << " -> Đang thoát luồng.\n";
    }
    // 5. Ngay tại vị trí kết thúc hàm hoặc văng lỗi, 'lock' bị hủy và tự nhả khóa.
}

int main() {
    std::vector<std::thread> workers;

    // 6. Yêu cầu Linux tạo 5 luồng chạy song song
    for (int i = 1; i <= 5; ++i) {
        workers.push_back(std::thread(read_pn532_nfc_data, i));
    }

    // 7. Chờ các luồng thực thi xong
    for (auto& t : workers) {
        t.join();
    }

    std::cout << "Hệ thống kết thúc an toàn. Trạng thái Mutex: Đã nhả khóa hoàn toàn.\n";
    return 0;
}
