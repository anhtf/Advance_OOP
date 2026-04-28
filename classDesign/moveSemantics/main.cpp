/*
- Thiết kế 1 class hoàn chỉnh và an toàn trong C++: Hệ thống quản lý kết nối TCP
có khả năng cấm sao chép nhưng cho phép di chuyển quyền sở hữu socket giữa các
luồng thực thi
- std::move: bản chất không hề di chuyển bất cữ dữ liệu nào ở tầng vật lý, đây
là 1 kiểu static cast dạng tham chiếu(&&)
==> move constructor để chuyển con trỏ tài nguyên sang 1 đối tượng mới
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

class TcpConnection {
private:
  int socket_fd;
  std::string remote_ip;
  uint16_t remote_port;

  void set_non_blocking() {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
      throw std::runtime_error("F_GETFL failed");
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      throw std::runtime_error("F_SETFL failed");
    }
  }

public:
  explicit TcpConnection(const std::string &ip, uint16_t port)
      : socket_fd(-1), remote_ip(ip), remote_port(port) {

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
      throw std::runtime_error("Socket creation failed");
    }

    set_non_blocking();

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(remote_port);

    if (inet_pton(AF_INET, remote_ip.c_str(), &server_addr.sin_addr) <= 0) {
      close(socket_fd);
      throw std::runtime_error("Invalid IPv4 address");
    }

    int ret =
        connect(socket_fd, reinterpret_cast<struct sockaddr *>(&server_addr),
                sizeof(server_addr));
    if (ret == -1 && errno != EINPROGRESS) {
      close(socket_fd);
      throw std::runtime_error("Connection initiation failed");
    }
  }

  ~TcpConnection() {
    if (socket_fd != -1) {
      close(socket_fd);
    }
  }

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  TcpConnection(TcpConnection &&other) noexcept
      : socket_fd(other.socket_fd), remote_ip(std::move(other.remote_ip)),
        remote_port(other.remote_port) {
    other.socket_fd = -1;
    other.remote_port = 0;
  }

  TcpConnection &operator=(TcpConnection &&other) noexcept {
    if (this != &other) {
      if (socket_fd != -1) {
        close(socket_fd);
      }

      socket_fd = other.socket_fd;
      remote_ip = std::move(other.remote_ip);
      remote_port = other.remote_port;

      other.socket_fd = -1;
      other.remote_port = 0;
    }
    return *this;
  }

  int get_descriptor() const { return socket_fd; }
};

void dispatch_connection(TcpConnection conn) {
  std::cout << "Dispatching FD: " << conn.get_descriptor() << "\n";
}

int main() {
  try {
    TcpConnection client("192.168.7.2", 8080);
    std::cout << "Root FD: " << client.get_descriptor() << "\n";

    std::vector<TcpConnection> connection_pool;

    connection_pool.push_back(std::move(client));

    dispatch_connection(std::move(connection_pool.back()));
    connection_pool.pop_back();

  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << "\n";
  }
  return 0;
}