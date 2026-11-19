#pragma once
#include <stdint.h>
inline bool xtouch_cloud_has_body(int status,int length=-1){return length!=0&&status>=200&&status!=204&&status!=205&&status!=304;}
// Keep transport failures, ordinary denials and browser challenges distinct.
inline bool xtouch_cloud_contains(const char *text,const char *needle){
 if(!text||!needle)return false;
 for(;*text;++text){const char *a=text,*b=needle;while(*a&&*b){char c=*a,d=*b;if(c>='A'&&c<='Z')c+='a'-'A';if(d>='A'&&d<='Z')d+='a'-'A';if(c!=d)break;++a;++b;}if(!*b)return true;}return false;
}
inline bool xtouch_cloud_challenge(int status,const char *body,const char *mitigated,const char *ray){
 if(status<=0)return false;
 if(xtouch_cloud_contains(body,"Just a moment...")||xtouch_cloud_contains(body,"challenges.cloudflare.com"))return true;
 if(status==403&&mitigated&&*mitigated)return true;
 if((status==403||status==429)&&xtouch_cloud_contains(body,"cloudflare"))return true;
 if(status==503&&ray&&*ray)return true;
 return status==418&&(xtouch_cloud_contains(body,"captcha")||xtouch_cloud_contains(body,"robot"));
}
