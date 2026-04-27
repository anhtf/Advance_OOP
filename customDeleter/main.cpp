/*
1. Khai báo Custom Deleter (struct FileDeleter)
Thay vì để unique_ptr dùng lệnh delete (vốn sẽ gây lỗi crash vì FILE* không được cấp phát bằng new), 
chúng ta định nghĩa một cấu trúc có nạp chồng toán tử gọi hàm operator().
 Lệnh fclose() sẽ được bọc an toàn ở đây.

2. Type Alias (using SmartFile)
Dòng lệnh using SmartFile = std::unique_ptr<FILE, FileDeleter>; 
giúp rút gọn cú pháp. Giờ đây SmartFile đóng vai trò như một kiểu dữ liệu mới:
 Nó là một con trỏ giữ FILE* và biết cách tự hủy bằng FileDeleter.

3. Tương tác với C-API (.get())
Hàm fprintf() của C yêu cầu một con trỏ FILE* thô. Để truyền dữ liệu cho nó, chúng ta sử dụng phương thức .get() của unique_ptr (ví dụ: log_pool[0].get()). 
Hàm này cấp quyền truy cập đọc con trỏ thô bên trong mà không làm mất đi quyền sở hữu (ownership) của unique_ptr.

4. Chuỗi phản ứng tháo dỡ (Unwinding Chain)
Khi lệnh throw nổ ra ở cuối hàm process_multiple_logs, chuỗi dọn dẹp diễn ra cực kỳ chặt chẽ:

Hàm bị ngắt, đối tượng log_pool (kiểu std::vector) nằm trên Stack bị phá hủy.

std::vector kích hoạt hàm hủy của nó, tiến hành duyệt qua mảng và gọi hàm hủy của 2 đối tượng SmartFile.

Hàm hủy của SmartFile nhận ra nó đang chứa một FILE* hợp lệ, bèn kích hoạt FileDeleter.

fclose() được gọi 2 lần trên 2 file log. An toàn tuyệt đối.
*/
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

struct FileDeleter {
  void operator()(FILE *fp) const {
    if (fp) {
      std::cout << "RAII Triggered: Safely closing FILE pointer.\n";
      fclose(fp);
    }
  }
};

using SmartFile = std::unique_ptr<FILE, FileDeleter>;

void process_multiple_logs() {
  std::vector<SmartFile> log_pool;

  log_pool.push_back(SmartFile(fopen("network_traffic.log", "w")));
  log_pool.push_back(SmartFile(fopen("hardware_faults.log", "w")));

  if (!log_pool[0] || !log_pool[1]) {
    throw std::runtime_error("Disk I/O Error: Cannot create log files");
  }

  fprintf(log_pool[0].get(), "Network interface up.\n");
  fprintf(log_pool[1].get(), "Sensors initialized.\n");

  std::cout << "Logs created. Simulating unexpected kernel panic...\n";
  throw std::runtime_error("Simulated crash during file I/O!");

  fprintf(log_pool[0].get(), "This line will never be written.\n");
}

int main() {
  try {
    process_multiple_logs();
  } catch (const std::exception &e) {
    std::cout << "Fatal error caught in main: " << e.what() << "\n";
  }
  return 0;
}
/*
*/