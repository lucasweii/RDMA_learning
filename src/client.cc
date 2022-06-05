#include "client.h"
#include <glog/logging.h>
#include <cassert>

Client::Client(uint32_t ib_port, uint32_t gid_idx) : RDMA(ib_port, gid_idx) {
  conn_ = new TCPConnector();
}

bool Client::Connect(std::string ip_addr, std::string ip_port) {
  bool rt = conn_->Connect(ip_addr, ip_port);
  assert(rt);
  rt = Init();
  assert(rt);
  Connection linfo = LocalInfo();
  Connection rinfo;
  int recv = conn_->ExchangeData((char *)&linfo, sizeof(linfo), (char *)&rinfo, sizeof(rinfo));
  SetRemoteInfo(rinfo);
  rt = ModifyQP(INIT);
  assert(rt);
  LOG(INFO) << "client : modify to INIT ";
  rt = ModifyQP(RTR);
  assert(rt);
  LOG(INFO) << "client : modify to RTR ";
  rt = ModifyQP(RTS);
  assert(rt);
  LOG(INFO) << "client : modify to RTS ";
  return rt;
}