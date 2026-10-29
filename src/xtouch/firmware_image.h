#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
static inline bool xtouch_application_image(const uint8_t *header,size_t size,uint16_t chip){
 return size>=36&&header[0]==0xe9&&((uint16_t)header[12]|((uint16_t)header[13]<<8))==chip&&header[32]==0x32&&header[33]==0x54&&header[34]==0xcd&&header[35]==0xab;
}
/* IDF's descriptor has a fixed 32-byte project field. Chip identity alone is
 * insufficient when multiple display boards use the same ESP32-S3. */
static inline bool xtouch_application_project(const char project[32],const char *expected){
 if(!project||!expected)return false;
 size_t length=0;while(length<32&&expected[length])++length;
 return length>0&&length<32&&project[length]=='\0'&&!memcmp(project,expected,length);
}
