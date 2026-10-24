#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
static inline bool xtouch_application_image(const uint8_t *header,size_t size,uint16_t chip){
 return size>=36&&header[0]==0xe9&&((uint16_t)header[12]|((uint16_t)header[13]<<8))==chip&&header[32]==0x32&&header[33]==0x54&&header[34]==0xcd&&header[35]==0xab;
}
