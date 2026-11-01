#include <gtest/gtest.h>
#include "cacheforge/cache.hpp"
#include "cacheforge/distributed.hpp"
#include <thread>
using namespace cacheforge;
TEST(Cache, SetGetDelete){Cache c(3);c.set("a","value");EXPECT_EQ(c.get("a"),"value");EXPECT_TRUE(c.erase("a"));EXPECT_FALSE(c.exists("a"));}
TEST(Cache, EvictsLeastRecentlyUsed){Cache c(2);c.set("a","1");c.set("b","2");c.get("a");c.set("c","3");EXPECT_FALSE(c.exists("b"));EXPECT_TRUE(c.exists("a"));}
TEST(Cache, Expires){Cache c(2);c.set("a","1",std::chrono::milliseconds(5));std::this_thread::sleep_for(std::chrono::milliseconds(10));EXPECT_FALSE(c.get("a"));}
TEST(Cache, RejectsEmptyKey){Cache c(2);EXPECT_THROW(c.set("","x"),std::invalid_argument);}
TEST(HashRing, StableAndRestores){ConsistentHashRing r(32);r.add({"a","h",1});r.add({"b","h",2});auto n=r.locate("key");r.add({"c","h",3});r.remove("c");EXPECT_EQ(r.locate("key"),n);}
