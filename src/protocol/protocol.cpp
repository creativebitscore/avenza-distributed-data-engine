#include "cacheforge/protocol.hpp"
#include <cctype>
#include <iomanip>
#include <sstream>
namespace cacheforge::protocol {
std::vector<std::string> parse(const std::string& line,std::string& error){std::vector<std::string> out;size_t i=0;while(i<line.size()){while(i<line.size()&&std::isspace((unsigned char)line[i]))++i;if(i==line.size())break;std::string s;if(line[i]=='"'){++i;bool closed=false;while(i<line.size()){char c=line[i++];if(c=='"'){closed=true;break;}if(c=='\\'){if(i==line.size()){error="trailing escape";return{};}char e=line[i++];if(e=='n')s+='\n';else if(e=='r')s+='\r';else if(e=='t')s+='\t';else if(e=='"'||e=='\\')s+=e;else{error="invalid escape";return{};}}else s+=c;}if(!closed){error="unterminated quote";return{};}if(i<line.size()&&!std::isspace((unsigned char)line[i])){error="expected space after quote";return{};}}else{while(i<line.size()&&!std::isspace((unsigned char)line[i]))s+=line[i++];}out.push_back(std::move(s));}return out;}
std::string quote(const std::string& v){std::string o="\"";for(char c:v){if(c=='\\'||c=='"'){o+='\\';o+=c;}else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else if(c=='\t')o+="\\t";else o+=c;}return o+'"';}
std::string upper(std::string v){for(char&c:v)c=char(std::toupper((unsigned char)c));return v;}
}
