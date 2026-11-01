#include "cacheforge/network.hpp"
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>
static cacheforge::Server* active=nullptr;
static void signal_handler(int){if(active)active->stop();}
int main(int argc,char**argv){uint16_t port=7001;size_t capacity=10000;std::string data="data";bool persist=true;for(int i=1;i<argc;++i){std::string a=argv[i];auto need=[&](){if(++i>=argc)throw std::runtime_error("missing value for "+a);return std::string(argv[i]);};try{if(a=="--port")port=(uint16_t)std::stoul(need());else if(a=="--capacity")capacity=std::stoull(need());else if(a=="--data-dir")data=need();else if(a=="--no-persistence")persist=false;else if(a=="--help"){std::cout<<"avenza-server [--port N] [--capacity N] [--data-dir PATH] [--no-persistence]\n";return 0;}else throw std::runtime_error("unknown option "+a);}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}}cacheforge::Server server(port,capacity,data,persist);active=&server;std::signal(SIGINT,signal_handler);std::signal(SIGTERM,signal_handler);std::string error;if(!server.start(error)){std::cerr<<"startup failed: "<<error<<'\n';return 1;}std::cout<<"Avenza listening on port "<<port<<"\n";server.run();return 0;}
