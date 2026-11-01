#include "cacheforge/cache.hpp"
#include "cacheforge/distributed.hpp"
#include "cacheforge/network.hpp"
#include "cacheforge/persistence.hpp"
#include "cacheforge/protocol.hpp"
#include <atomic>
#include <filesystem>
#include <iostream>
#include <thread>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__FILE__<<':'<<__LINE__<<" "#x"\n";return 1;}}while(0)
int main(){using namespace cacheforge;
 Cache c(2);c.set("a","1");c.set("b","2");CHECK(c.get("a")=="1");c.set("c","3");CHECK(!c.get("b"));CHECK(c.exists("a"));CHECK(c.erase("a"));CHECK(!c.exists("a"));
 c.set("ttl","x",std::chrono::milliseconds(30));std::this_thread::sleep_for(std::chrono::milliseconds(40));CHECK(!c.get("ttl"));CHECK(c.ttl_seconds("missing")==-2);
 std::atomic<int> count{};std::vector<std::thread> ts;for(int t=0;t<8;++t)ts.emplace_back([&,t]{for(int i=0;i<1000;++i){c.set(std::to_string(t)+":"+std::to_string(i),"v");c.get(std::to_string(t)+":"+std::to_string(i));++count;}});for(auto&t:ts)t.join();CHECK(count==8000);CHECK(c.size()<=2);
 std::string err;auto parsed=protocol::parse("SET key \"hello world\\nline\"",err);CHECK(err.empty()&&parsed.size()==3&&parsed[2]=="hello world\nline");protocol::parse("SET x \"bad",err);CHECK(!err.empty());
 auto dir=std::filesystem::temp_directory_path()/"cacheforge-selftest";std::filesystem::remove_all(dir);{Cache p(10);Persistence disk(dir);p.set("saved","yes");disk.log_set("saved","yes");p.set("gone","x");disk.log_set("gone","x");p.erase("gone");disk.log_del("gone");if(!disk.save_snapshot(p,err)){std::cerr<<"snapshot error: "<<err<<'\n';return 1;}p.set("after","a");disk.log_set("after","a");}{Cache p(10);Persistence disk(dir);CHECK(disk.recover(p,err));CHECK(p.get("saved")=="yes");CHECK(p.get("after")=="a");CHECK(!p.get("gone"));}std::filesystem::remove_all(dir);
 PubSub ps;std::string delivered;auto id=ps.subscribe("news",[&](auto&,auto&m){delivered=m;});CHECK(ps.publish("other","x")==0);CHECK(ps.publish("news","hello")==1&&delivered=="hello");CHECK(ps.unsubscribe("news",id));
 ConsistentHashRing ring(64);ring.add({"a","localhost",1});ring.add({"b","localhost",2});ring.add({"c","localhost",3});std::vector<std::string> before;for(int i=0;i<1000;++i)before.push_back(ring.locate("key"+std::to_string(i))->id);ring.add({"d","localhost",4});int moved=0;for(int i=0;i<1000;++i)moved+=before[i]!=ring.locate("key"+std::to_string(i))->id;CHECK(moved>100&&moved<500);ring.remove("d");for(int i=0;i<1000;++i)CHECK(before[i]==ring.locate("key"+std::to_string(i))->id);
 auto netdir=std::filesystem::temp_directory_path()/"cacheforge-nettest";Server server(17391,2,netdir.string(),false);CHECK(server.start(err));std::thread st([&]{server.run();});std::this_thread::sleep_for(std::chrono::milliseconds(30));TcpClient one,two;CHECK(one.connect("127.0.0.1",17391,err));CHECK(two.connect("127.0.0.1",17391,err));std::string r;CHECK(one.send_line("PING",r,err)&&r=="PONG");CHECK(one.send_line("SET name \"Ada Lovelace\"",r,err)&&r=="OK");CHECK(two.send_line("GET name",r,err)&&r=="VALUE \"Ada Lovelace\"");CHECK(one.send_line("SET short x EX 1",r,err));CHECK(one.send_line("TTL short",r,err)&&r=="1");server.stop();st.join();std::filesystem::remove_all(netdir);
 std::cout<<"All CacheForge self-tests passed\n";return 0;}
