#ifndef _XLCD_FIRMWARE
#define _XLCD_FIRMWARE

#include "platform/idf/runtime.h"
#include "features/diagnostics/stack_margin.h"
#include "platform/idf/ota.h"
#include "platform/idf/filesystem.h"
#include "xtouch/card_fs.h"
#include "firmware_target.h"
#include <stdio.h>
static void xtouch_firmware_reportOtaStack(XTouchStackMargin::Path path)
{
    const unsigned minimum = XTouchStackMargin::counter(path).minimum.load();
    if (minimum != UINT_MAX && uxTaskGetStackHighWaterMark(nullptr) >= 2048)
        ConsoleInfo.printf("[OTA] stack-min %s=%u bytes\n", XTouchStackMargin::name(path), minimum);
}
#include "ota_manifest_policy.h"

using SemVer = XTouchOtaSelection::Version;

bool parseSemVer(const char *version, SemVer &semver)
{
    return XTouchOtaSelection::parse(version, semver);
}

int compareSemVer(const SemVer &v1, const SemVer &v2)
{
    return XTouchOtaSelection::compare(v1, v2);
}

bool isSemVerGreaterThan(const SemVer &v1, const SemVer &v2)
{
    return compareSemVer(v1, v2) > 0;
}

bool isSemVerLessThan(const SemVer &v1, const SemVer &v2)
{
    return compareSemVer(v1, v2) < 0;
}

bool isSemVerEqualTo(const SemVer &v1, const SemVer &v2)
{
    return compareSemVer(v1, v2) == 0;
}

static bool xtouch_firmware_manifest_valid(JsonDocument &doc)
{
    const char *version = doc["version"] | "";
    const char *url = doc["url"] | "";
    const char *md5 = doc["md5"] | "";
    SemVer parsed;
    if (!parseSemVer(version, parsed) || strlen(md5) != 32 ||
        !XTouchOtaSelection::imageUrl(url, version, XTOUCH_FIRMWARE_FILENAME) ||
        !doc["size"].is<unsigned>() || doc["size"].as<unsigned>() == 0 ||
        doc["size"].as<unsigned>() > XTOUCH_FIRMWARE_MAX_BYTES) return false;
    for (unsigned i = 0; i < 32; i++) if (!isxdigit(md5[i])) return false;
    return true;
}

void xtouch_firmware_onProgress(size_t current, size_t total)
{
    XTouchStackMargin::sample(XTouchStackMargin::Path::OtaSd);
    // The shared intro caption stays stable while downloading and writing.
    (void)current;
    (void)total;
    lv_timer_handler();
}

static void xtouch_firmware_showUpdating(void)
{
    lv_label_set_text(introScreenCaption, xtouch_local("OTA 更新中…", "OTA updating…"));
    // Paint before blocking network operations or flash erase can begin.
    lv_refr_now(NULL);
}

void xtouch_firmware_checkOnlineFirmwareUpdate(bool manual = false)
{
    if (!manual && !xTouchConfig.xTouchOTAEnabled) return;
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_CHECKING_FOR_OTA_UPDATE);
    lv_refr_now(NULL);
    ConsoleInfo.printf("[OTA] checking current=%s\n", XTOUCH_FIRMWARE_VERSION);
    fs::FS &cache = xtouch_storage_sd_available() ? static_cast<fs::FS &>(SD) : static_cast<fs::FS &>(SPIFFS);
    String manifest;
    if (!xtouch_ota_fetch_manifest(manifest,manual)) {lv_label_set_text(introScreenCaption,XTOUCH_TEXT_UPDATE_ERROR);return;}
    DynamicJsonDocument doc(2048);
    if(deserializeJson(doc,manifest)){lv_label_set_text(introScreenCaption,XTOUCH_TEXT_UPDATE_ERROR);return;}
    manifest=String();
    SemVer current, remote;
    if (!xtouch_firmware_manifest_valid(doc) || !parseSemVer(XTOUCH_FIRMWARE_VERSION, current) ||
        !parseSemVer(doc["version"], remote)) {
        ConsoleError.println("[OTA] invalid manifest");
        lv_label_set_text(introScreenCaption,XTOUCH_TEXT_UPDATE_ERROR);
        if(cache.exists(xtouch_paths_firmware_ota_json))cache.remove(xtouch_paths_firmware_ota_json);
        return;
    }
    if (!isSemVerGreaterThan(remote, current)) {
        ConsoleInfo.println("[OTA] already current");
        lv_label_set_text(introScreenCaption,XTOUCH_TEXT_UP_TO_DATE);
        if(cache.exists(xtouch_paths_firmware_ota_json))cache.remove(xtouch_paths_firmware_ota_json);
        return;
    }
    // Two bounded attempts; a failed update must never prevent normal startup.
    for (unsigned attempt = 0; attempt < 2; attempt++) {
        xtouch_firmware_showUpdating();
        auto progress = [](int) {
            lv_timer_handler();
        };
        bool success;
        if (xtouch_storage_sd_available()) success = downloadFileToFS(SD, doc["url"], xtouch_paths_firmware_ota_fw,
            progress, NULL, doc["md5"], doc["size"]);
        else success = downloadFirmwareToFlash(doc["url"], doc["md5"], doc["size"], progress);
          xtouch_firmware_reportOtaStack(XTouchStackMargin::Path::OtaHttps);
        if(success && xtouch_storage_sd_available()) {
            File record=SD.open(xtouch_paths_firmware_ota_json,FILE_WRITE);
            size_t expected=measureJson(doc);
            bool written=record && serializeJson(doc,record)==expected;
            if(record){record.flush();record.close();}
            if(!written){SD.remove(xtouch_paths_firmware_ota_fw);success=false;}
        }
        if (success) {
            if(!xtouch_storage_sd_available() && cache.exists(xtouch_paths_firmware_ota_json))cache.remove(xtouch_paths_firmware_ota_json);
            ConsoleInfo.println("[OTA] verified, rebooting");
            delay(200);
            ESP.restart();
        }
    }
    if(cache.exists(xtouch_paths_firmware_ota_json))cache.remove(xtouch_paths_firmware_ota_json);
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_FAILED_TO_DOWNLOAD_UPDATE);
    lv_refr_now(NULL);
}

void xtouch_firmware_checkFirmwareUpdate(void)
{
    if (!xtouch_storage_sd_available() || !SD.exists(xtouch_paths_firmware_ota_fw)) return;
    bool manifestPresent = SD.exists(xtouch_paths_firmware_ota_json);
    DynamicJsonDocument doc(1024);
    if (manifestPresent) doc = xtouch_filesystem_readJson(SD, xtouch_paths_firmware_ota_json);
    bool verified = xtouch_firmware_manifest_valid(doc);
    File firmware = SD.open(xtouch_paths_firmware_ota_fw, FILE_READ);
    // Plain firmware.bin on SD remains supported; an OTA manifest, when present,
    // must be valid and match the complete image before touching flash.
    uint8_t imageHeader[36]={0};size_t headerSize=firmware?firmware.read(imageHeader,sizeof(imageHeader)):0;firmware.seek(0);
    bool valid = firmware && firmware.size() <= XTOUCH_FIRMWARE_MAX_BYTES && xtouch_firmware_header_valid(imageHeader,headerSize);
    if (manifestPresent) valid = valid && verified && firmware.size() == doc["size"].as<unsigned>();
    if (valid && manifestPresent) {
        MD5Builder md5;
        md5.begin();
        md5.addStream(firmware, firmware.size());
        md5.calculate();
        valid = md5.toString().equalsIgnoreCase(doc["md5"].as<const char *>());
        firmware.seek(0);
    }
    bool installed = false;
    if (valid) {
        XTouchStackMargin::Scope stack(XTouchStackMargin::Path::OtaSd);
        xtouch_firmware_showUpdating();
        Update.onProgress(xtouch_firmware_onProgress);
        if (Update.begin(firmware.size(), U_FLASH)) {
            if (manifestPresent) Update.setMD5(doc["md5"]);
            size_t written = Update.writeStream(firmware);
            installed = written == firmware.size() && Update.end();
            if (!installed) Update.abort();
        }
    }
    firmware.close();
    xtouch_firmware_reportOtaStack(XTouchStackMargin::Path::OtaSd);
    Update.onProgress(nullptr);
    if(SD.exists(xtouch_paths_firmware_ota_json))SD.remove(xtouch_paths_firmware_ota_json);
    SD.remove(xtouch_paths_firmware_ota_fw);
    ConsoleInfo.printf("[OTA] install %s\n", installed ? "SUCCESS" : "REJECTED");
    lv_label_set_text(introScreenCaption, installed ? XTOUCH_TEXT_UPDATE_FINISHED : XTOUCH_TEXT_UPDATE_ERROR);
    lv_refr_now(NULL);
    if (installed) { delay(300); ESP.restart(); }
}
#endif
