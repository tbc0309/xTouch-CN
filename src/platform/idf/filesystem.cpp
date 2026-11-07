#include "filesystem.h"
#include "storage_profile.h"
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <driver/spi_master.h>
bool NativeCard::begin(){
#if defined(XTOUCH_PORT_SD_DISABLED) && XTOUCH_PORT_SD_DISABLED
 return false;
#else
 if(card){if(xtouch_idf::cardIoGate().retiring())end();if(card)return cardType()!=CARD_NONE;}
 const auto &profile=xtouch_board_storage_profile();
 if(profile.bus==BoardStorageProfile::Bus::None)return false;
 esp_vfs_fat_sdmmc_mount_config_t config{};config.format_if_mount_failed=false;config.max_files=8;config.allocation_unit_size=16*1024;
 esp_err_t error=ESP_FAIL;
 if(profile.bus==BoardStorageProfile::Bus::Spi){
  static bool initialized=false;
  const auto bus=static_cast<spi_host_device_t>(profile.spiHost);
  if(!initialized){spi_bus_config_t pins{};pins.mosi_io_num=profile.mosi;pins.miso_io_num=profile.miso;pins.sclk_io_num=profile.clock;pins.quadwp_io_num=-1;pins.quadhd_io_num=-1;pins.max_transfer_sz=4096;
   if(spi_bus_initialize(bus,&pins,SPI_DMA_CH_AUTO)!=ESP_OK)return false;initialized=true;}
  sdmmc_host_t host=SDSPI_HOST_DEFAULT();host.slot=profile.spiHost;host.max_freq_khz=profile.maxFrequencyKhz;
  sdspi_device_config_t slot=SDSPI_DEVICE_CONFIG_DEFAULT();slot.host_id=bus;slot.gpio_cs=static_cast<gpio_num_t>(profile.cs);
  error=esp_vfs_fat_sdspi_mount("/sdcard",&host,&slot,&config,&card);
 }else{
#if SOC_SDMMC_HOST_SUPPORTED
  sdmmc_host_t host=SDMMC_HOST_DEFAULT();host.flags|=SDMMC_HOST_FLAG_DEINIT_ARG;host.max_freq_khz=profile.maxFrequencyKhz;
  host.do_transaction=[](int slot,sdmmc_command_t *cmd)->esp_err_t{if(cmd->opcode==0||cmd->opcode==1||cmd->opcode==5||cmd->opcode==8||cmd->opcode==41||cmd->opcode==55)cmd->timeout_ms=100;return sdmmc_host_do_transaction(slot,cmd);};
  sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();
  if(!profile.configure||!profile.configure(host,slot)){if(profile.release)profile.release();return false;}
  error=esp_vfs_fat_sdmmc_mount("/sdcard",&host,&slot,&config,&card);
#else
  return false;
#endif
 }
 if(error!=ESP_OK||!card){card=nullptr;if(profile.release)profile.release();return false;}
 if(card)xtouch_idf::cardIoGate().mount(card->ocr&SD_OCR_SDHC_CAP?CARD_SDHC:CARD_SD,uint64_t(card->csd.capacity)*card->csd.sector_size);
 return card!=nullptr;
#endif
}
CardProbeResult NativeCard::probeRaw(uint8_t *p,uint32_t sector){
 if(!card)return CardProbeResult::Failed;
 if(!xtouch_idf::cardIoGate().beginProbe())return CardProbeResult::Busy;
 bool ok=sdmmc_read_sectors(card,p,sector,1)==ESP_OK;xtouch_idf::cardIoGate().endProbe(ok);
 return ok?CardProbeResult::Ready:CardProbeResult::Failed;
}
void NativeCard::end(){if(card){if(!xtouch_idf::cardIoGate().claimUnmount())return;esp_vfs_fat_sdcard_unmount("/sdcard",card);card=nullptr;Serial.println("[SD] unmounted after I/O drained");}
 const auto &profile=xtouch_board_storage_profile();if(profile.release)profile.release();
}
