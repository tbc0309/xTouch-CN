#include "board.h"
#include <SD_MMC.h>
bool __attribute__((weak)) board_draw_buffers(lv_color_t **,lv_color_t **,uint32_t *){return false;}
bool __attribute__((weak)) board_pixels_async(lv_disp_drv_t *,int,int,int,int,const uint16_t *){return false;}
#ifdef XTOUCH_BOARD_P4
static_assert(BOARD_SDMMC_SLOT==0,"SD card must not share the C6 SDIO slot");
static_assert(BOARD_SDMMC_POWER_PIN==45&&BOARD_SDMMC_POWER_CHANNEL==4,"Review Waveshare SD power wiring before changing the Arduino variant");
#endif
bool board_card_begin(){
#ifdef XTOUCH_BOARD_S3
 pinMode(21,OUTPUT);digitalWrite(21,HIGH);
 if(!SD_MMC.setPins(14,17,16))return false;
#else
 if(!SD_MMC.setPins(43,44,39,40,41,42))return false;
#endif
 return SD_MMC.begin("/sdcard",
#ifdef XTOUCH_BOARD_S3
 true,
#else
 false,
#endif
 false,20000); // Never format an inserted card on mount failure.
}
