#pragma once
#ifdef XTOUCH_WAVESHARE
#include <SD_MMC.h>
#define SD SD_MMC
#else
#include <SD.h>
#endif
