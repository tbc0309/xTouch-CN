#pragma once
#include <stddef.h>
#include <stdint.h>
namespace xtouch_idf {
/* Prefetch using a nonblocking read, independent of optional FIONREAD support.
   TLS WANT_READ/WANT_WRITE are not payload and do not mean a broken stream. */
template<class Read>
size_t receive_available(uint8_t *buffer,size_t capacity,size_t &cursor,size_t &count,Read read){
 if(cursor<count)return count-cursor;
 cursor=count=0;
 int received=read(buffer,capacity);
 if(received>0&&size_t(received)<=capacity)count=size_t(received);
 return count;
}
}
