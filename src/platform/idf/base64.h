#ifndef XTOUCH_NATIVE_IDF_BASE64_H
#define XTOUCH_NATIVE_IDF_BASE64_H
#pragma once
#include <mbedtls/base64.h>
inline size_t base64_encode_expected_len(size_t n){return 4*((n+2)/3);}
inline int base64_encode_chars(const char *data,size_t length,char *out){size_t written=0;int rc=mbedtls_base64_encode((unsigned char*)out,base64_encode_expected_len(length)+1,&written,(const unsigned char*)data,length);return rc?0:int(written);}

#endif
