#include "cacheforge/cache.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>
using namespace std::chrono;
static void run(const std::string&name,size_t ops,size_t cap,int get_pct){cacheforge::Cache cache(cap);for(size_t i=0;i<std::min(cap,ops/2);++i)cache.set("k"+std::to_string(i),"value");std::mt19937_64 rng(42);std::vector<double> us;us.reserve(ops);auto all=steady_clock::now();for(size_t i=0;i<ops;++i){auto t=steady_clock::now();if((int)(rng()%100)<get_pct)cache.get("k"+std::to_string(rng()%std::max<size_t>(1,cap)));else cache.set("k"+std::to_string(rng()%std::max<size_t>(1,cap*2)),"value");us.push_back(duration<double,std::micro>(steady_clock::now()-t).count());}double seconds=duration<double>(steady_clock::now()-all).count();std::sort(us.begin(),us.end());auto p=[&](double q){return us[std::min(us.size()-1,(size_t)(q*us.size()))];};double avg=std::accumulate(us.begin(),us.end(),0.0)/us.size();std::cout<<name<<" capacity="<<cap<<" ops="<<ops<<" throughput_ops_s="<<(uint64_t)(ops/seconds)<<" avg_us="<<avg<<" p50_us="<<p(.50)<<" p95_us="<<p(.95)<<" p99_us="<<p(.99)<<'\n';}
int main(int argc,char**argv){size_t ops=argc>1?std::stoull(argv[1]):100000,cap=argc>2?std::stoull(argv[2]):10000;std::cout<<"hardware_threads="<<std::thread::hardware_concurrency()<<" (record OS/CPU/RAM separately for published results)\n";run("set",ops,cap,0);run("get",ops,cap,100);run("mixed80get20set",ops,cap,80);}
