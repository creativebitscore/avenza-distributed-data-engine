#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace cacheforge {
struct Node { std::string id, host; uint16_t port{}; bool operator==(const Node&) const = default; };
class ConsistentHashRing {
 public:
  explicit ConsistentHashRing(size_t virtual_nodes = 128): virtual_nodes_(virtual_nodes) {}
  void add(Node node); void remove(const std::string& id); std::optional<Node> locate(const std::string& key) const;
  std::vector<Node> nodes() const; std::vector<std::pair<uint64_t,std::string>> points() const;
  static uint64_t hash(const std::string& text);
 private: size_t virtual_nodes_; std::map<uint64_t, Node> ring_; std::map<std::string, Node> nodes_;
};
}
