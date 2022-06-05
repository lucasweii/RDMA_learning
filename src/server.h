#include "rdma.h"
#include "tcp_connection.h"

class Server : public RDMA {
 public:
  Server(std::string ip_port, uint32_t ib_port, uint32_t gid_idx);

  bool Connect();

  bool Sync() { return conn_->Sync(); }

 private:
  TCPConnector *conn_;
};