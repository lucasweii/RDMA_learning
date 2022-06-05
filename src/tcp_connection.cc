#include "tcp_connection.h"
#include <glog/logging.h>
#include <cassert>
#include <cstring>

TCPConnector::TCPConnector(std::string ip_port) : TCPConnector() {
  is_sever_ = true;
  addrinfo hint = {.ai_flags = AI_PASSIVE, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  addrinfo *addrs;
  addrinfo *it;
  int rc = getaddrinfo(nullptr, ip_port.c_str(), &hint, &addrs);
  assert(rc == 0);
  for (it = addrs; it != nullptr; it = it->ai_next) {
    sock_fd_ = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
    if (sock_fd_ == -1) {
      continue;
    }
    break;
  }

  if (sock_fd_ == -1) {
    LOG(ERROR) << "server : create socket failed.";
    return;
  }

  rc = bind(sock_fd_, it->ai_addr, it->ai_addrlen);
  assert(rc == 0);
  rc = listen(sock_fd_, 1);
  assert(rc == 0);
  free(addrs);
}

TCPConnector::TCPConnector() {
  send_buf_ = new char[TCP_BUF_SIZE];
  recv_buf_ = new char[TCP_BUF_SIZE];
}

TCPConnector::~TCPConnector() {
  if (send_buf_ != nullptr) {
    delete[] send_buf_;
    send_buf_ = nullptr;
  }
  if (recv_buf_ != nullptr) {
    delete[] recv_buf_;
    recv_buf_ = nullptr;
  }
  if (sock_fd_ != -1) {
    close(sock_fd_);
    sock_fd_ = -1;
  }
}

bool TCPConnector::Connect(std::string ip_addr, std::string ip_port) {
  addrinfo hint = {.ai_flags = AI_PASSIVE, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  addrinfo *addrs;
  addrinfo *it;
  int rc = getaddrinfo(ip_addr.c_str(), ip_port.c_str(), &hint, &addrs);
  assert(rc == 0);
  for (it = addrs; it != nullptr; it = it->ai_next) {
    sock_fd_ = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
    if (sock_fd_ == -1) {
      continue;
    }
    break;
  }

  if (sock_fd_ == -1) {
    LOG(ERROR) << "server : create socket failed.";
    return false;
  }

  rc = connect(sock_fd_, it->ai_addr, it->ai_addrlen);
  return rc == 0;
}

bool TCPConnector::Connect() {
  int listen_fd = sock_fd_;
  sock_fd_ = accept(listen_fd, nullptr, 0);
  close(listen_fd);
  return sock_fd_ != -1;
}

bool TCPConnector::Sync() {
  memset(send_buf_, 0, TCP_BUF_SIZE);
  send_buf_[0] = sync_counter_++;

  sync_counter_ = sync_counter_ % 256;
  sync_counter_ = sync_counter_ == 0 ? 1 : sync_counter_;

  write(sock_fd_, send_buf_, 1);
  int recv_num = read(sock_fd_, recv_buf_, 1);
  assert(recv_num == 1);

  return recv_buf_[0] == send_buf_[0];
}

int TCPConnector::ExchangeData(const char *send_buf, int send_size, char *recv_buf, int recv_size) {
  int rc = write(sock_fd_, send_buf, send_size);
  assert(rc == send_size);
  rc = 0;
  int recv = 0;
  while (rc == 0 && recv < recv_size) {
    int read_bytes = read(sock_fd_, recv_buf, recv_size);
    if (read_bytes > 0) {
      recv += read_bytes;
    } else {
      rc = read_bytes;
    }
  }
  return recv;
}