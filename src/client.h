#include "rdma.h"
#include "tcp_connection.h"

class Client : public RDMA {
 public:
  Client(uint32_t ib_port, uint32_t gid_idx);

  bool Connect(std::string ip_addr, std::string ip_port);

  bool Sync() { return conn_->Sync(); }

 private:
  TCPConnector *conn_;
};