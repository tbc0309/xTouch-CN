#pragma once
#include "policy.h"
namespace xtouch_files {
// Only descend into supported folders explicitly present in the root listing.
inline bool listedDirectory(const std::string &line,const char *wanted) {
    auto separator=line.find(' ');
    if(separator==std::string::npos)return false;
    std::string facts=line.substr(0,separator);
    for(char &c:facts)c=tolower((unsigned char)c);
    if(facts.rfind("type=",0)==0 || facts.find(";type=")!=std::string::npos) {
        bool directory=false;size_t at=0;
        while(at<facts.size()) {auto end=facts.find(';',at);if(end==std::string::npos)return false;
            directory|=facts.substr(at,end-at)=="type=dir";at=end+1;}
        return directory && line.substr(separator+1)==wanted;
    }
    if(line.size()<10 || line[0]!='d')return false;
    size_t at=0;
    for(unsigned field=0;field<8;++field) {
        while(at<line.size() && line[at]==' ')++at;
        auto end=line.find(' ',at);if(end==std::string::npos)return false;at=end+1;
    }
    while(at<line.size() && line[at]==' ')++at;
    return line.substr(at)==wanted;
}
}
