#include "cacheforge/network.hpp"
#include "cacheforge/protocol.hpp"
#include <algorithm>
#include <charconv>
#include <sstream>
#include <unordered_map>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
namespace cacheforge { namespace {
#ifdef _WIN32
using Socket=SOCKET; constexpr Socket bad=INVALID_SOCKET;void close_socket(Socket s){closesocket(s);}struct SocketInit{SocketInit(){WSADATA w;WSAStartup(MAKEWORD(2,2),&w);}~SocketInit(){WSACleanup();}} socket_init;
#else
using Socket=int;constexpr Socket bad=-1;void close_socket(Socket s){::close(s);}
#endif
bool send_all(Socket s,const std::string&v){size_t n=0;while(n<v.size()){int r=send(s,v.data()+n,(int)(v.size()-n),0);if(r<=0)return false;n+=r;}return true;}
bool number(const std::string&s,int64_t&v){auto [p,e]=std::from_chars(s.data(),s.data()+s.size(),v);return e==std::errc{}&&p==s.data()+s.size();}
int64_t unix_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(SystemClock::now().time_since_epoch()).count();}
}
struct Server::Client{Socket socket{bad};std::mutex write_mutex;std::unordered_map<std::string,uint64_t> subscriptions;std::atomic<bool> alive{true};bool write(const std::string&s){std::lock_guard l(write_mutex);return send_all(socket,s+"\n");}};
Server::Server(uint16_t p,size_t c,std::string d,bool persist):port_(p),cache_(c),persistence_(std::move(d),persist){std::string warning;persistence_.recover(cache_,warning);}
Server::~Server(){stop();}
bool Server::start(std::string&error){if(running_)return true;Socket s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s==bad){error="socket creation failed";return false;}int yes=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char*)&yes,sizeof yes);sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons(port_);if(bind(s,(sockaddr*)&a,sizeof a)!=0||listen(s,64)!=0){close_socket(s);error="bind/listen failed (port may be in use)";return false;}listener_=(std::intptr_t)s;running_=true;return true;}
void Server::run(){if(running_)accept_loop();}
void Server::accept_loop(){while(running_){Socket s=accept((Socket)listener_,nullptr,nullptr);if(s==bad){if(!running_)break;continue;}auto c=std::make_shared<Client>();c->socket=s;{std::lock_guard l(clients_mutex_);clients_.push_back(c);}{std::lock_guard l(threads_mutex_);client_threads_.emplace_back(&Server::serve,this,c);}}}
void Server::stop(){if(!running_.exchange(false))return;Socket l=(Socket)listener_;listener_=-1;
#ifdef _WIN32
 shutdown(l,SD_BOTH);
#else
 shutdown(l,SHUT_RDWR);
#endif
 close_socket(l);{std::lock_guard lock(clients_mutex_);for(auto&c:clients_){c->alive=false;
#ifdef _WIN32
 shutdown(c->socket,SD_BOTH);
#else
 shutdown(c->socket,SHUT_RDWR);
#endif
 close_socket(c->socket);}clients_.clear();}{std::lock_guard lock(threads_mutex_);for(auto&t:client_threads_)if(t.joinable())t.join();client_threads_.clear();}}
std::string Server::execute(const std::vector<std::string>&a){if(a.empty())return "ERR empty command";auto cmd=protocol::upper(a[0]);try{
if(cmd=="PING"&&a.size()==1)return "PONG";
if(cmd=="SET"&&(a.size()==3||a.size()==5)){std::optional<std::chrono::milliseconds> ttl;if(a.size()==5){if(protocol::upper(a[3])!="EX")return "ERR expected EX seconds";int64_t sec;if(!number(a[4],sec)||sec<=0)return "ERR TTL must be positive";ttl=std::chrono::seconds(sec);}cache_.set(a[1],a[2],ttl);persistence_.log_set(a[1],a[2],ttl?std::optional<int64_t>(unix_ms()+ttl->count()):std::nullopt);return "OK";}
if(cmd=="GET"&&a.size()==2){auto v=cache_.get(a[1]);return v?"VALUE "+protocol::quote(*v):"NIL";}
if(cmd=="DEL"&&a.size()==2){bool n=cache_.erase(a[1]);if(n)persistence_.log_del(a[1]);return n?"1":"0";}
if(cmd=="EXISTS"&&a.size()==2)return cache_.exists(a[1])?"1":"0";
if(cmd=="EXPIRE"&&a.size()==3){int64_t sec;if(!number(a[2],sec))return "ERR invalid integer";bool n=cache_.expire(a[1],std::chrono::seconds(sec));if(n){if(sec<=0)persistence_.log_del(a[1]);else persistence_.log_expire(a[1],unix_ms()+sec*1000);}return n?"1":"0";}
if(cmd=="TTL"&&a.size()==2)return std::to_string(cache_.ttl_seconds(a[1]));
if(cmd=="FLUSHALL"&&a.size()==1){cache_.flush();persistence_.log_flush();return "OK";}
if(cmd=="SAVE"&&a.size()==1){std::string e;return persistence_.save_snapshot(cache_,e)?"OK":"ERR "+e;}
if((cmd=="STATS"||cmd=="INFO")&&a.size()==1){auto s=cache_.stats();std::ostringstream o;o<<"size="<<cache_.size()<<" capacity="<<cache_.capacity()<<" hits="<<s.hits<<" misses="<<s.misses<<" sets="<<s.sets<<" deletes="<<s.deletes<<" evictions="<<s.evictions<<" expirations="<<s.expirations;return o.str();}
if(cmd=="PUBLISH"&&a.size()==3)return std::to_string(pubsub_.publish(a[1],a[2]));
return "ERR unknown command or wrong argument count";}catch(const std::exception&e){return std::string("ERR ")+e.what();}}
void Server::serve(std::shared_ptr<Client>c){std::string buffer;char bytes[4096];while(running_&&c->alive){int n=recv(c->socket,bytes,sizeof bytes,0);if(n<=0)break;buffer.append(bytes,n);if(buffer.size()>1024*1024){c->write("ERR request too large");break;}size_t pos;while((pos=buffer.find('\n'))!=std::string::npos){std::string line=buffer.substr(0,pos);buffer.erase(0,pos+1);if(!line.empty()&&line.back()=='\r')line.pop_back();std::string err;auto a=protocol::parse(line,err);if(!err.empty()){c->write("ERR "+err);continue;}auto cmd=a.empty()?"":protocol::upper(a[0]);if(cmd=="SUBSCRIBE"&&a.size()==2){if(c->subscriptions.count(a[1])){c->write("OK");continue;}std::weak_ptr<Client>w=c;auto id=pubsub_.subscribe(a[1],[w](const std::string&ch,const std::string&m){if(auto x=w.lock();x&&x->alive)x->write("MESSAGE "+protocol::quote(ch)+" "+protocol::quote(m));});c->subscriptions[a[1]]=id;c->write("OK");}else if(cmd=="UNSUBSCRIBE"&&a.size()==2){auto it=c->subscriptions.find(a[1]);bool ok=it!=c->subscriptions.end();if(ok){pubsub_.unsubscribe(a[1],it->second);c->subscriptions.erase(it);}c->write(ok?"1":"0");}else c->write(execute(a));}}bool owned=c->alive.exchange(false);for(auto&[ch,id]:c->subscriptions)pubsub_.unsubscribe(ch,id);if(owned)close_socket(c->socket);}
TcpClient::~TcpClient(){close();}bool TcpClient::connect(const std::string&h,uint16_t p,std::string&e){close();addrinfo hints{},*res=nullptr;hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;auto ps=std::to_string(p);if(getaddrinfo(h.c_str(),ps.c_str(),&hints,&res)!=0){e="host lookup failed";return false;}for(auto*i=res;i;i=i->ai_next){Socket s=socket(i->ai_family,i->ai_socktype,i->ai_protocol);if(s!=bad&&::connect(s,i->ai_addr,(int)i->ai_addrlen)==0){socket_=(std::intptr_t)s;break;}if(s!=bad)close_socket(s);}freeaddrinfo(res);if(socket_==-1){e="connection failed";return false;}return true;}
bool TcpClient::send_line(const std::string&line,std::string&response,std::string&error){if(socket_==-1){error="not connected";return false;}if(!send_all((Socket)socket_,line+"\n")){error="send failed";return false;}response.clear();char c;while(response.size()<1024*1024){int n=recv((Socket)socket_,&c,1,0);if(n<=0){error="connection closed";return false;}if(c=='\n')return true;if(c!='\r')response+=c;}error="response too large";return false;}
void TcpClient::close(){if(socket_!=-1){close_socket((Socket)socket_);socket_=-1;}}
}
