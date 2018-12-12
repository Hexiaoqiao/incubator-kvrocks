#ifndef KVROCKS_SERVER_H
#define KVROCKS_SERVER_H

#include <list>
#include <string>
#include <vector>

#include "stats.h"
#include "storage.h"
#include "replication.h"
#include "task_runner.h"
#include "t_metadata.h"

namespace Redis {
class Connection;
}

class WorkerThread;

struct DBScanInfo {
  time_t last_scan_time = 0;
  uint64_t n_key = 0;
  bool is_scanning = false;
};

class Server {
 public:
  explicit Server(Engine::Storage *storage, Config *config);
  ~Server();
  Status Start();
  void Stop();
  void Join();

  Status AddMaster(std::string host, uint32_t port);
  Status RemoveMaster();
  bool IsLoading() {return is_loading_;}
  int PublishMessage(std::string &channel, std::string &msg);
  void SubscribeChannel(std::string &channel, Redis::Connection *conn);
  void UnSubscribeChannel(std::string &channel, Redis::Connection *conn);
  Config *GetConfig() { return config_; }
  bool IsSlave() { return !master_host_.empty(); }

  Status IncrClients();
  void DecrClients();
  void GetInfo(std::string ns, std::string section, std::string &info);
  void GetStatsInfo(std::string &info);
  void GetServerInfo(std::string &info);
  void GetRocksDBInfo(std::string &info);
  void GetReplicationInfo(std::string &info);
  void GetClientsInfo(std::string &info);
  void GetMemoryInfo(std::string &info);
  Status AsyncScanDBSize(std::string &ns);
  uint64_t GetLastKeyNum(std::string &ns);
  time_t GetLastScanTime(std::string &ns);

  Stats stats_;
  Engine::Storage *storage_;
 private:
  bool is_loading_ = false;
  time_t start_time_ = 0;
  std::string master_host_;
  uint32_t master_port_ = 0;

  std::atomic<int> connected_clients_{0};
  std::atomic<uint64_t> total_clients_{0};

  Config *config_;
  std::vector<WorkerThread*> worker_threads_;
  std::unique_ptr<ReplicationThread> replication_thread_;
  std::thread cron_thread_;
  TaskRunner *task_runner_;

  // TODO: locked before modify
  std::map<std::string, std::list<Redis::Connection*>> pubsub_channels_;
  std::map<std::string, DBScanInfo> db_scan_infos_;

  void cron();
  void clientsCron();
};


#endif //KVROCKS_SERVER_H
