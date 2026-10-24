#pragma once
#include <stdint.h>
#if defined(XTOUCH_WAVESHARE) && defined(XTOUCH_BOARD_S3)
static constexpr uint32_t xtouchHttpUploadLimit=6U*1024U*1024U+4096U;
#elif defined(XTOUCH_WAVESHARE)
static constexpr uint32_t xtouchHttpUploadLimit=12U*1024U*1024U+4096U;
#else
static constexpr uint32_t xtouchHttpUploadLimit=1966080U+4096U;
#endif
static constexpr uint32_t xtouchHttpBodyLimit=32768U;
static constexpr uint32_t xtouchHttpConfigBodyLimit=196608U;
