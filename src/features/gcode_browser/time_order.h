#pragma once
#include <string>
#include <cstdint>
#include <cstdio>
#include <cctype>
namespace xtouch_files {
struct FileTimeContext { int year=0; int64_t now=0; int localOffset=0; };
inline int64_t calendarSeconds(int y,int m,int d,int h=0,int n=0,int s=0) {
    if(y<1970 || y>9999 || m<1 || m>12 || d<1 || h<0 || h>23 || n<0 || n>59 || s<0 || s>59)return -1;
    const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    bool leap=y%4==0 && (y%100!=0 || y%400==0);
    if(d>days[m-1]+(m==2 && leap))return -1;
    y-=m<=2;int era=y/400,yoe=y-era*400;
    int doy=(153*(m+(m>2?-3:9))+2)/5+d-1;
    int64_t day=int64_t(era)*146097+yoe*365+yoe/4-yoe/100+doy-719468;
    return day*86400+h*3600+n*60+s;
}
// MLSD modify is UTC. UNIX LIST is local time and may omit the current year.
// Unknown timestamps stay last; ties retain the original listing order.
inline int64_t fileTime(const std::string &stamp,const FileTimeContext &ctx) {
    if(stamp.size()>=14 && std::isdigit((unsigned char)stamp[0])) {
        for(unsigned i=0;i<14;++i)if(!std::isdigit((unsigned char)stamp[i]))return 0;
        if(stamp.size()>14) {
            if(stamp[14]!='.' || stamp.size()==15)return 0;
            for(unsigned i=15;i<stamp.size();++i)if(!std::isdigit((unsigned char)stamp[i]))return 0;
        }
        auto num=[&](unsigned at,unsigned length){int v=0;while(length--)v=v*10+stamp[at++]-'0';return v;};
        int64_t value=calendarSeconds(num(0,4),num(4,2),num(6,2),num(8,2),num(10,2),num(12,2));
        return value<0?0:value;
    }
    char month[4]{},token[16]{},extra=0;unsigned day=0;
    if(std::sscanf(stamp.c_str(),"%3s %u %15s %c",month,&day,token,&extra)!=3 || day>31)return 0;
    for(char &c:month)c=std::tolower((unsigned char)c);
    const char *months[]={"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};
    int m=0;for(int i=0;i<12;++i)if(std::string(month)==months[i])m=i+1;
    if(!m)return 0;
    std::string tail=token;int year=0,hour=0,minute=0;bool inferred=tail.size()==5 && tail[2]==':';
    if(inferred) {
        if(!ctx.year)return 0;
        for(unsigned i:{0u,1u,3u,4u})if(!std::isdigit((unsigned char)tail[i]))return 0;
        year=ctx.year;hour=(tail[0]-'0')*10+tail[1]-'0';minute=(tail[3]-'0')*10+tail[4]-'0';
    } else {
        if(tail.size()!=4)return 0;
        for(char c:tail){if(!std::isdigit((unsigned char)c))return 0;year=year*10+c-'0';}
    }
    auto localEpoch=calendarSeconds(year,m,int(day),hour,minute);if(localEpoch<0)return 0;
    int64_t value=localEpoch-ctx.localOffset;
    if(inferred && ctx.now && value>ctx.now+86400) {
        localEpoch=calendarSeconds(year-1,m,int(day),hour,minute);if(localEpoch<0)return 0;
        value=localEpoch-ctx.localOffset;
    }
    return value<0?0:value;
}
} // namespace xtouch_files
