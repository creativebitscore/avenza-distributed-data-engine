#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cacheforge {
using Clock = std::chrono::steady_clock;
using SystemClock = std::chrono::system_clock;

struct CacheStats { uint64_t hits{}, misses{}, sets{}, deletes{}, evictions{}, expirations{}; };
struct Record { std::string key, value; std::optional<int64_t> expires_unix_ms; };

class Cache {
 public:
  explicit Cache(size_t capacity);
  void set(std::string key, std::string value, std::optional<std::chrono::milliseconds> ttl = {});
  std::optional<std::string> get(const std::string& key);
  bool erase(const std::string& key); bool exists(const std::string& key);
  bool expire(const std::string& key, std::chrono::seconds ttl);
  int64_t ttl_seconds(const std::string& key);
  void flush(); size_t size(); size_t capacity() const { return capacity_; }
  CacheStats stats() const; std::vector<Record> records();
  void restore(Record record);
 private:
  struct Entry { std::string value; std::list<std::string>::iterator lru; std::optional<Clock::time_point> expires; };
  bool expired(const Entry&) const; void remove_locked(std::unordered_map<std::string, Entry>::iterator, bool expiration);
  void purge_expired_locked(); void touch_locked(std::unordered_map<std::string, Entry>::iterator);
  size_t capacity_; mutable std::mutex mutex_; std::unordered_map<std::string, Entry> entries_;
  std::list<std::string> lru_; CacheStats stats_;
};

class PubSub {
 public:
  using Callback = std::function<void(const std::string&, const std::string&)>;
  uint64_t subscribe(const std::string& channel, Callback callback);
  bool unsubscribe(const std::string& channel, uint64_t id);
  size_t publish(const std::string& channel, const std::string& message);
  void unsubscribe_all(uint64_t id);
 private:
  std::mutex mutex_; uint64_t next_id_{1};
  std::unordered_map<std::string, std::unordered_map<uint64_t, Callback>> channels_;
};
}
