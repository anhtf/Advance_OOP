/*
Trong thực tế, không phải lúc nào vùng nhớ cũng thuộc về đối tượng duy nhất.
C++ cung cấp std::shared_ptr giải quyết bài toán này thông qua cơ chế tham chiếu
nguyên tử (Atomic reference counting)

  + Init: std::shared_ptr -> counter = 1
  + Coppy con trỏ này sang luồng hoặc đối tượng khác -> counter += 1
  + Khi con trỏ bị hủy -> counter -= 1
  + Counter == 0 => Heap mới thực sự được giải phóng

== >Chi phí tính toán bộ đếm nhưng màn lại sự linh hoạt trong các kiến trúc phức
tạp.

RAII không chỉ dành cho RAM: Ngoài dùng cho malloc và free thì quản lý tệp tin
miêu tả (file descriptor) hoặc khóa đa luồng (mutex locks). Giả sử 1 hàm mở file
hay mở kết nối mạng, sau đó gặp 1 khối try-catch luồng điều khiển sẽ vang ra
ngoài ngay lập tức, các lệnh close ở cuối hàm sẽ vĩnh viễn k đc thực thi.
*/

#include "fcntl.h"
#include "iostream"
#include "memory"
#include "stdexcept"
#include "unistd.h"
#include "vector"
#include <fcntl.h>

class safeFileDescriptor {
private:
  int fd;

public:
  explicit safeFileDescriptor(const char *filePath, int flags) { // Ngăn cấm việc ép kiểu ngầm định (safeFileDescriptor a = "/dev/null")
    fd = open(filePath, flags);
    if (fd == -1) {
      throw std::runtime_error("Failed to open file descriptor");
    }
    std::cout << "Resource Acquired: FD " << fd << " open.\n";
  }

  ~safeFileDescriptor() { // Tự động gọi khi đối tượng ra khỏi phạm vi khối lệnh
    if (fd != -1) {
      close(fd);
      std::cout << "Resource Released: FD " << fd << " closed automatically.\n";
    }
  }

  // Xóa bỏ hàm coppy
  /*
  Trong C++11 trở đi, từ khóa = delete là cách bạn nói thẳng với trình biên dịch: 
  "Này, tôi cấm tuyệt đối việc sử dụng hàm này. Nếu ai cố tình dùng, hãy báo lỗi ngay lúc biên dịch (compile-time) chứ đừng đợi đến lúc chạy mới crash!"
  */
  safeFileDescriptor(const safeFileDescriptor &) = delete;
  safeFileDescriptor &operator=(const safeFileDescriptor &) = delete;

  int get() const { return fd; }
};

void process_data_from_device(bool trigger_error) {
  safeFileDescriptor sensor_file("/dev/null", O_RDONLY);

  // Dòng này cấm việc tạo ra một đối tượng mới tinh bằng cách copy từ một đối tượng đã có.
  // Hành vi bị chặn: SafeFileDescriptor B = A; hoặc khi bạn truyền đối tượng vào một hàm theo kiểu tham trị: void do_something(SafeFileDescriptor file).
  // safeFileDescriptor sensor_file2 = sensor_file;
  int fd = sensor_file.get();

  std::cout << "Reading from FD: " << fd << "...\n";

  if (trigger_error) {
    std::cout << "Hardware fault detected! Throwing exception...\n";
    throw std::runtime_error("Device disconnected");
  }
  std::cout << "Data processed successfully.\n";
}

int main() {
  std::cout << "--- Test 1: Normal Execution ---\n";
  try {
    process_data_from_device(false);
  } catch (const std::exception &e) {
    std::cout << "Caught: " << e.what() << "\n";
  }

  std::cout << "\n--- Test 2: Exception Handling ---\n";
  try {
    process_data_from_device(true);
  } catch (const std::exception &e) {
    std::cout << "Caught: " << e.what() << "\n";
  }

  return 0;
}