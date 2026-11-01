#include "cacheforge/distributed.hpp"
namespace cacheforge {
uint64_t ConsistentHashRing::hash(const std::string&s){uint64_t h=14695981039346656037ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
void ConsistentHashRing::add(Node n){remove(n.id);nodes_[n.id]=n;for(size_t i=0;i<virtual_nodes_;++i)ring_[hash(n.id+"#"+std::to_string(i))]=n;}
void ConsistentHashRing::remove(const std::string&id){nodes_.erase(id);for(auto it=ring_.begin();it!=ring_.end();)if(it->second.id==id)it=ring_.erase(it);else++it;}
std::optional<Node> ConsistentHashRing::locate(const std::string&k)const{if(ring_.empty())return{};auto it=ring_.lower_bound(hash(k));if(it==ring_.end())it=ring_.begin();return it->second;}
std::vector<Node> ConsistentHashRing::nodes()const{std::vector<Node> v;for(auto&[_,n]:nodes_)v.push_back(n);return v;}
std::vector<std::pair<uint64_t,std::string>> ConsistentHashRing::points()const{std::vector<std::pair<uint64_t,std::string>>v;for(auto&[p,n]:ring_)v.emplace_back(p,n.id);return v;}
}
