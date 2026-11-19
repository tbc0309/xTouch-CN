#pragma once
#include <string.h>
/* Exact get_version accessory names; never interpret a printer OTA as AMS. */
static inline unsigned xtouch_ams_module_kind(const char *name,int *id){
    static const char *prefix[]={"ams/","ams_f1/","n3f/","n3s/"};
    if(!name||!id)return 0;
    for(unsigned i=0;i<4;i++){
        size_t n=strlen(prefix[i]);if(strncmp(name,prefix[i],n))continue;
        const char *p=name+n;unsigned value=0;if(!*p)return 0;
        for(;*p;p++){if(*p<'0'||*p>'9')return 0;value=value*10+(*p-'0');if(value>255)return 0;}
        if((i==0||i==2)&&value>=4)return 0;
        if(i==1&&value>=4&&(value<16||value>31))return 0;
        if(i==3&&(value<128||value>135))return 0;
        *id=(int)value;return i==1&&value>=16?5:i+1;
    }
    return 0;
}
