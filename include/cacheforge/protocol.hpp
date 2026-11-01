#pragma once
#include <string>
#include <vector>
namespace cacheforge::protocol {
std::vector<std::string> parse(const std::string& line, std::string& error);
std::string quote(const std::string& value);
std::string upper(std::string value);
}
