#pragma once
#include <ctype.h>
#include <string.h>
namespace XTouchClock {
inline bool number(const char *&p,int limit) {
 if(!isdigit((unsigned char)*p))return false;
 unsigned n=0;while(isdigit((unsigned char)*p)){n=n*10+(*p++-'0');if(n>(unsigned)limit)return false;}return true;
}
inline bool name(const char *&p) {
 unsigned n=0;if(*p=='<'){++p;while(isalnum((unsigned char)*p)||*p=='+'||*p=='-'){++p;++n;}if(*p!='>')return false;++p;}
 else while(isalpha((unsigned char)*p)){++p;++n;}
 return n>=3;
}
inline bool offset(const char *&p,int hours=24) {
 if(*p=='+'||*p=='-')++p;if(!number(p,hours))return false;
 if(*p==':'){++p;if(!number(p,59))return false;if(*p==':'){++p;if(!number(p,59))return false;}}
 return true;
}
inline bool rule(const char *&p) {
 if(*p=='M'){++p;const char *month=p;if(!number(p,12)||month[0]=='0'||*p++!='.')return false;
 const char *week=p;if(!number(p,5)||week[0]=='0'||*p++!='.'||!number(p,6))return false;}
 else {bool julian=*p=='J';if(julian)++p;if(!number(p,365))return false;}
 if(*p=='/'){++p;if(!offset(p,167))return false;}return true;
}
inline bool timezone(const char *value) {
 if(!value||strlen(value)>=96)return false;if(!*value)return true;
 const char *p=value;if(!name(p)||!offset(p))return false;if(!*p)return true;
 if(!name(p))return false;if(*p!=','&&*p&&!offset(p))return false;if(!*p)return true;
 if(*p++!=','||!rule(p)||*p++!=','||!rule(p))return false;return !*p;
}
inline bool server(const char *value) {
 size_t n=value?strlen(value):0;if(!n||n>=128)return false;
 if(!isalnum((unsigned char)value[0])||!isalnum((unsigned char)value[n-1]))return false;
 unsigned label=0;for(const char *p=value;*p;++p){if(*p=='.'){if(!label||p[-1]=='-')return false;label=0;}
 else {if(!isalnum((unsigned char)*p)&&*p!='-')return false;if(!label&&*p=='-')return false;if(++label>63)return false;}}
 return label>0;
}
}
