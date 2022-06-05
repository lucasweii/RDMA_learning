#include <netdb.h>
#include <sys/socket.h>
#include <string>
#define TCP_BUF_SIZE 1

class TCPConnector {
 public:
  // Sever
  TCPConnector(std::string ip_port);
  // Client
  TCPConnector();

  ~TCPConnector();

  TCPConnector(const TCPConnector &t) = delete;
  TCPConnector &operator=(const TCPConnector &t) = delete;
  // only for client
  bool Connect(std::string ip_addr, std::string ip_port);
  // only for sever
  bool Connect();

  bool Sync();
  int ExchangeData(const char *send_buf, int send_size, char *recv_buf, int recv_size);
  int SockFD() const { return sock_fd_; }
  char *Buf() { return send_buf_; }

 private:
  int sync_counter_ = 1;
  bool is_sever_ = false;
  int sock_fd_ = -1;
  int client_sock_fd_ = -1;
  char *send_buf_ = nullptr;
  char *recv_buf_ = nullptr;
};