#pragma once

#include <event2/buffer.h>
#include <list>
#include <string>

#include "redis_cmd.h"
#include "worker.h"
#include "status.h"
#include "storage.h"

namespace Redis {

class Connection;

class Request {
 public:
  explicit Request(Worker *svr) : svr_(svr) {}
  // Not copyable
  Request(const Request &) = delete;
  Request &operator=(const Request &) = delete;

  // Parse the redis requests (bulk string array format)
  void Tokenize(evbuffer *input);
  // Exec return true when command finished
  void ExecuteCommands(evbuffer *output, Connection *conn);

 private:
  // internal states related to parsing

  enum ParserState { ArrayLen, BulkLen, BulkData };
  ParserState state_ = ArrayLen;
  size_t multi_bulk_len_ = 0;
  size_t bulk_len_ = 0;
  using CommandTokens = std::vector<std::string>;
  CommandTokens tokens_;
  std::vector<CommandTokens> commands_;

  Worker *svr_;
};

class Connection {
 public:
  explicit Connection(bufferevent *bev, Worker *svr) : bev_(bev), req_(svr), owner_(svr) {}
  ~Connection() {
    if (bev_) bufferevent_free(bev_);
  }

  static void OnRead(struct bufferevent *bev, void *ctx);
  static void OnEvent(bufferevent *bev, short events, void *ctx);

  evbuffer *Input();
  evbuffer *Output();
  bufferevent *DetachBufferEvent() {
    auto tmp = bev_;
    bev_ = nullptr;
    return tmp;
  }

  int GetFD();

 private:
  bufferevent *bev_;
  Request req_;
  Worker *owner_;
};

}  // namespace Redis
