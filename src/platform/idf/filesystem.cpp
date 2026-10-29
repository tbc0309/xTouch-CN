#include "filesystem.h"
#include <esp_vfs_fat.h>
#include <driver/sdmmc_host.h>
#if defined(XTOUCH_BOARD_CYD)
#include <driver/sdspi_host.h>
#include <driver/spi_master.h>
#endif
#if CONFIG_IDF_TARGET_ESP32P4
#include <sd_pwr_ctrl_by_on_chip_ldo.h>
static sd_pwr_ctrl_handle_t cardPower;
#endif
bool NativeCard::begin(){
#if defined(XTOUCH_PORT_SD_DISABLED) && XTOUCH_PORT_SD_DISABLED
 return false; // Unmapped sockets must not inherit another board's GPIOs.
#else
 if(card)return true;
#if defined(XTOUCH_BOARD_CYD)
 static bool initialized=false;
 if(!initialized){spi_bus_config_t bus{};bus.mosi_io_num=23;bus.miso_io_num=19;bus.sclk_io_num=18;bus.quadwp_io_num=bus.quadhd_io_num=-1;bus.max_transfer_sz=4096;auto error=spi_bus_initialize(SPI3_HOST,&bus,SPI_DMA_CH_AUTO);if(error!=ESP_OK)return false;initialized=true;}
 sdmmc_host_t host=SDSPI_HOST_DEFAULT();host.slot=SPI3_HOST;host.max_freq_khz=10000;
 sdspi_device_config_t slot=SDSPI_DEVICE_CONFIG_DEFAULT();slot.host_id=SPI3_HOST;slot.gpio_cs=GPIO_NUM_5;
 esp_vfs_fat_sdmmc_mount_config_t config{};config.format_if_mount_failed=false;config.max_files=8;config.allocation_unit_size=16*1024;
 if(esp_vfs_fat_sdspi_mount("/sdcard",&host,&slot,&config,&card)!=ESP_OK)card=nullptr;
 return card!=nullptr;
#else
 sdmmc_host_t host=SDMMC_HOST_DEFAULT();
 host.flags|=SDMMC_HOST_FLAG_DEINIT_ARG;
 host.max_freq_khz=20000;
 host.do_transaction=[](int slot,sdmmc_command_t *cmd)->esp_err_t{if(cmd->opcode==0||cmd->opcode==1||cmd->opcode==5||cmd->opcode==8||cmd->opcode==41||cmd->opcode==55)cmd->timeout_ms=100;return sdmmc_host_do_transaction(slot,cmd);};
 sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();
#ifdef XTOUCH_BOARD_ES3C35P
 host.slot=1;host.flags=SDMMC_HOST_FLAG_4BIT|SDMMC_HOST_FLAG_DEINIT_ARG;
 slot.clk=GPIO_NUM_5;slot.cmd=GPIO_NUM_4;slot.d0=GPIO_NUM_6;slot.d1=GPIO_NUM_7;slot.d2=GPIO_NUM_2;slot.d3=GPIO_NUM_3;slot.width=4;
#elif defined(XTOUCH_BOARD_S3)
 pinMode(21,OUTPUT);digitalWrite(21,HIGH);
 host.slot=1;host.flags=SDMMC_HOST_FLAG_1BIT|SDMMC_HOST_FLAG_DEINIT_ARG;
 slot.clk=GPIO_NUM_14;slot.cmd=GPIO_NUM_17;slot.d0=GPIO_NUM_16;slot.width=1;
#else
 host.slot=0;host.flags=SDMMC_HOST_FLAG_4BIT|SDMMC_HOST_FLAG_DEINIT_ARG;
 // P4 slot 0 uses fixed IO_MUX; zero is the IDF 'unspecified' sentinel.
 slot.clk=slot.cmd=slot.d0=slot.d1=slot.d2=slot.d3=slot.d4=slot.d5=slot.d6=slot.d7=GPIO_NUM_0;slot.width=4;
 if(!cardPower){sd_pwr_ctrl_ldo_config_t power{};power.ldo_chan_id=4;if(sd_pwr_ctrl_new_on_chip_ldo(&power,&cardPower)!=ESP_OK)return false;}
 host.pwr_ctrl_handle=cardPower;
 pinMode(45,OUTPUT);digitalWrite(45,HIGH);delay(10);digitalWrite(45,LOW);delay(10);
#endif
 esp_vfs_fat_sdmmc_mount_config_t config{};config.format_if_mount_failed=false;config.max_files=8;config.allocation_unit_size=16*1024;
 auto rc=esp_vfs_fat_sdmmc_mount("/sdcard",&host,&slot,&config,&card);
 if(rc!=ESP_OK){card=nullptr;
#if CONFIG_IDF_TARGET_ESP32P4
  if(cardPower){sd_pwr_ctrl_del_on_chip_ldo(cardPower);cardPower=nullptr;}
#endif
 }
 return card!=nullptr;
#endif
#endif
}
void NativeCard::end(){if(card){esp_vfs_fat_sdcard_unmount("/sdcard",card);card=nullptr;}
#if CONFIG_IDF_TARGET_ESP32P4
 if(cardPower){sd_pwr_ctrl_del_on_chip_ldo(cardPower);cardPower=nullptr;}
#endif
}
