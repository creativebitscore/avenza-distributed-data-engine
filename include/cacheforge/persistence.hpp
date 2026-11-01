#pragma once
#include "cacheforge/cache.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>

namespace cacheforge {
class Persistence {
 public:
  Persistence(std::filesystem::path directory, bool enabled = true);
  bool recover(Cache& cache, std::string& error);
  bool save_snapshot(Cache& cache, std::string& error);
  void log_set(const std::string& key, const std::string& value, std::optional<int64_t> expires_unix_ms = {});
  void log_del(const std::string& key); void log_expire(const std::string& key, int64_t expires_unix_ms); void log_flush();
  bool enabled() const { return enabled_; }
 private:
  void append(char type, const std::string& key, const std::string& value, int64_t expiry);
  bool replay_file(const std::filesystem::path&, Cache&, bool snapshot, std::string&);
  std::filesystem::path dir_, aof_, snapshot_; bool enabled_; std::mutex mutex_;
};
}
