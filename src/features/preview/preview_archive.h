#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace XTouchPreviewArchive {
constexpr size_t imageLimit=262144,scanLimit=1024*1024;
constexpr unsigned thumbnailEdge=128;
inline uint16_t u16(const uint8_t *p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
inline uint32_t u32(const uint8_t *p){return uint32_t(u16(p))|uint32_t(u16(p+2))<<16;}
}
