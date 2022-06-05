#include "server.h"
#include <glog/logging.h>
#include <cassert>

Server::Server(std::string ip_port, uint32_t ib_port, uint32_t gid_idx) : RDMA(ib_port, gid_idx) {
  conn_ = new TCPConnector(ip_port);
}

bool Server::Connect() {
  bool rt = conn_->Connect();
  assert(rt);
  rt = Init();
  assert(rt);
  Connection linfo = LocalInfo();
  Connection rinfo;
  int recv = conn_->ExchangeData((char *)&linfo, sizeof(linfo), (char *)&rinfo, sizeof(rinfo));
  SetRemoteInfo(rinfo);
  rt = ModifyQP(INIT);
  assert(rt);
  LOG(INFO) << "server : modify to INIT ";
  rt = ModifyQP(RTR);
  assert(rt);
  LOG(INFO) << "server : modify to RTR ";
  rt = ModifyQP(RTS);
  assert(rt);
  LOG(INFO) << "server : modify to RTS ";
  return rt;
}
