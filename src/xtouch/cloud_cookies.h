#pragma once
#include <algorithm>
#include <cstring>
#include <string>
namespace XTouchCloudCookies {
/* Keep named response cookies independently: later tracking cookies must not
   overwrite authentication. Limits match the existing login/CSRF boundaries. */
struct Headers {
 std::string token,csrf;
 bool add(const char *header){
  if(!header)return false;
  while(*header==' '||*header=='\t')++header;
  const char *equal=std::strchr(header,'=');if(!equal)return false;
  std::string *target=nullptr;size_t limit=0,key=size_t(equal-header);
  if(key==5&&!std::strncmp(header,"token",5)){target=&token;limit=2048;}
  else if(key==14&&!std::strncmp(header,"bbl_csrf_token",14)){target=&csrf;limit=256;}
  else return false;
  std::fill(target->begin(),target->end(),0);target->clear();
  const char *value=equal+1,*end=std::strchr(value,';');if(!end)end=value+std::strlen(value);
  if(end-value>=2&&value[0]=='"'&&end[-1]=='"'){++value;--end;}
  if(size_t(end-value)>limit)return false;
  for(const char *p=value;p<end;++p)if(static_cast<unsigned char>(*p)<=32||static_cast<unsigned char>(*p)>=127)return false;
  target->assign(value,end);return true;
 }
 ~Headers(){std::fill(token.begin(),token.end(),0);std::fill(csrf.begin(),csrf.end(),0);}
};
}
