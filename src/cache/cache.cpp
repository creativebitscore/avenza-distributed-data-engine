#include "cacheforge/cache.hpp"
#include <algorithm>
#include <stdexcept>
namespace cacheforge {
Cache::Cache(size_t capacity): capacity_(capacity) { if (!capacity) throw std::invalid_argument("capacity must be positive"); }
bool Cache::expired(const Entry& e) const { return e.expires && Clock::now() >= *e.expires; }
void Cache::remove_locked(decltype(entries_)::iterator it, bool expiry) { lru_.erase(it->second.lru); entries_.erase(it); if(expiry) ++stats_.expirations; }
void Cache::purge_expired_locked(){ for(auto it=entries_.begin(); it!=entries_.end();) { if(expired(it->second)){ auto doomed=it++; remove_locked(doomed,true); } else ++it; } }
void Cache::touch_locked(decltype(entries_)::iterator it){ lru_.splice(lru_.begin(),lru_,it->second.lru); it->second.lru=lru_.begin(); }
void Cache::set(std::string key,std::string value,std::optional<std::chrono::milliseconds> ttl){ if(key.empty()) throw std::invalid_argument("key must not be empty"); if(ttl && ttl->count()<=0) throw std::invalid_argument("TTL must be positive"); std::lock_guard lock(mutex_); auto it=entries_.find(key); auto expiry=ttl?std::optional(Clock::now()+*ttl):std::nullopt; if(it!=entries_.end()){ it->second.value=std::move(value); it->second.expires=expiry; touch_locked(it); }else{ lru_.push_front(key); entries_.emplace(std::move(key),Entry{std::move(value),lru_.begin(),expiry}); } ++stats_.sets; while(entries_.size()>capacity_){ auto victim=entries_.find(lru_.back()); remove_locked(victim,false); ++stats_.evictions; } }
std::optional<std::string> Cache::get(const std::string& key){ std::lock_guard lock(mutex_); auto it=entries_.find(key); if(it==entries_.end()){++stats_.misses; return{};} if(expired(it->second)){remove_locked(it,true);++stats_.misses;return{};} touch_locked(it);++stats_.hits;return it->second.value; }
bool Cache::erase(const std::string& key){std::lock_guard lock(mutex_);auto it=entries_.find(key);if(it==entries_.end())return false;remove_locked(it,false);++stats_.deletes;return true;}
bool Cache::exists(const std::string& key){std::lock_guard lock(mutex_);auto it=entries_.find(key);if(it==entries_.end())return false;if(expired(it->second)){remove_locked(it,true);return false;}return true;}
bool Cache::expire(const std::string& key,std::chrono::seconds ttl){if(ttl.count()<=0)return erase(key);std::lock_guard lock(mutex_);auto it=entries_.find(key);if(it==entries_.end()||expired(it->second)){if(it!=entries_.end())remove_locked(it,true);return false;}it->second.expires=Clock::now()+ttl;return true;}
int64_t Cache::ttl_seconds(const std::string& key){std::lock_guard lock(mutex_);auto it=entries_.find(key);if(it==entries_.end())return -2;if(expired(it->second)){remove_locked(it,true);return -2;}if(!it->second.expires)return -1;auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(*it->second.expires-Clock::now()).count();return std::max<int64_t>(0,(ms+999)/1000);}
void Cache::flush(){std::lock_guard lock(mutex_);entries_.clear();lru_.clear();}
size_t Cache::size(){std::lock_guard lock(mutex_);purge_expired_locked();return entries_.size();}
CacheStats Cache::stats()const{std::lock_guard lock(mutex_);return stats_;}
std::vector<Record> Cache::records(){std::lock_guard lock(mutex_);purge_expired_locked();std::vector<Record> out; auto nowS=SystemClock::now();auto now=Clock::now();for(const auto& k:lru_){const auto&e=entries_.at(k);Record r{k,e.value,{}};if(e.expires)r.expires_unix_ms=std::chrono::duration_cast<std::chrono::milliseconds>(nowS.time_since_epoch()+(*e.expires-now)).count();out.push_back(std::move(r));}return out;}
void Cache::restore(Record r){std::optional<std::chrono::milliseconds> ttl;if(r.expires_unix_ms){auto left=std::chrono::milliseconds(*r.expires_unix_ms)-std::chrono::duration_cast<std::chrono::milliseconds>(SystemClock::now().time_since_epoch());if(left.count()<=0)return;ttl=left;}set(std::move(r.key),std::move(r.value),ttl);}
uint64_t PubSub::subscribe(const std::string& c,Callback cb){std::lock_guard l(mutex_);auto id=next_id_++;channels_[c][id]=std::move(cb);return id;}
bool PubSub::unsubscribe(const std::string& c,uint64_t id){std::lock_guard l(mutex_);auto it=channels_.find(c);if(it==channels_.end())return false;bool n=it->second.erase(id)>0;if(it->second.empty())channels_.erase(it);return n;}
void PubSub::unsubscribe_all(uint64_t id){std::lock_guard l(mutex_);for(auto it=channels_.begin();it!=channels_.end();){it->second.erase(id);if(it->second.empty())it=channels_.erase(it);else++it;}}
size_t PubSub::publish(const std::string& c,const std::string&m){std::vector<Callback> callbacks;{std::lock_guard l(mutex_);auto it=channels_.find(c);if(it!=channels_.end())for(auto&[_,cb]:it->second)callbacks.push_back(cb);}for(auto&cb:callbacks)cb(c,m);return callbacks.size();}
}
