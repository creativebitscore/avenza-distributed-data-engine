#pragma once
#include "cacheforge/cache.hpp"
#include "cacheforge/persistence.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace cacheforge {
class Server {
 public:
  Server(uint16_t port, size_t capacity, std::string data_dir, bool persistence = true);
  ~Server(); bool start(std::string& error); void run(); void stop(); uint16_t port() const { return port_; }
 private:
  struct Client;
  void accept_loop(); void serve(std::shared_ptr<Client> client); std::string execute(const std::vector<std::string>& args);
  uint16_t port_; Cache cache_; PubSub pubsub_; Persistence persistence_; std::atomic<bool> running_{false};
  std::intptr_t listener_{-1}; std::mutex clients_mutex_; std::vector<std::shared_ptr<Client>> clients_;
  std::mutex threads_mutex_; std::vector<std::thread> client_threads_;
};
class TcpClient {
 public:
  TcpClient()=default; ~TcpClient(); bool connect(const std::string& host, uint16_t port, std::string& error);
  bool send_line(const std::string& line, std::string& response, std::string& error); void close();
 private: std::intptr_t socket_{-1};
};
}
