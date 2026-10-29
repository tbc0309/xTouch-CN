#ifndef _XLCD_NET
#define _XLCD_NET

#include "platform/idf/wifi.h"
#include "http_client.h"
#include "features/diagnostics/stack_margin.h"
#include "platform/idf/digest.h"
#include <time.h>
#include <atomic>
#include <esp_sntp.h>
#include "clock.h"
#include "platform/idf/ota.h"
#include "ota-certs.h"
#include "ota_endpoint.h"
#include "firmware_target.h"
#include "ota_manifest_policy.h"

static std::atomic<bool> xtouch_clock_synced{false};
static void xtouch_clock_on_sync(struct timeval *) { xtouch_clock_synced.store(true); }
bool xtouch_clock_text(char *text, size_t size)
{
    if (!text || size < 6 || !xtouch_clock_synced.load()) return false;
    const time_t utc = time(NULL);
    if (utc < 1704067200) return false;
    struct tm value;
    if (!localtime_r(&utc, &value)) return false;
    return strftime(text, size, "%H:%M", &value) == 5;
}

// NTP stays in UTC; libc applies the configured POSIX time-zone rules.
static bool xtouch_clock_started = false;
static char xtouch_clock_zone[96],xtouch_clock_server[128];
void xtouch_clock_apply_settings()
{
    if (!xtouch_clock_started) return;
    const char *zone = xTouchConfig.xTouchTimezone[0] ? xTouchConfig.xTouchTimezone : "CST-8";
    const char *server = xTouchConfig.xTouchNtpServer[0] ? xTouchConfig.xTouchNtpServer : "ntp.aliyun.com";
    if (!strcmp(zone,xtouch_clock_zone) && !strcmp(server,xtouch_clock_server)) return;
    strlcpy(xtouch_clock_zone,zone,sizeof(xtouch_clock_zone));
    strlcpy(xtouch_clock_server,server,sizeof(xtouch_clock_server));
    configTzTime(xtouch_clock_zone,xtouch_clock_server);
}
static void xtouch_start_clock_sync()
{
    if (xtouch_clock_started) return;
    xtouch_clock_started = true;
    sntp_set_time_sync_notification_cb(xtouch_clock_on_sync);
    xtouch_clock_apply_settings();
}

static bool xtouch_ota_sync_clock()
{
    if (time(NULL) > 1704067200) return true;
    xtouch_start_clock_sync();
    uint32_t start = millis();
    while (time(NULL) < 1704067200 && millis() - start < 15000) delay(50);
    return time(NULL) > 1704067200;
}

// The native HTTP client handles framing, certificate checks and partial reads.
static bool xtouch_ota_fetch_manifest(String &body,bool manual=false){
    const uint32_t started=millis(),budget=manual?14500U:6500U;
    xtouch_start_clock_sync();
    while(time(NULL)<1704067200&&millis()-started<(manual?6000U:3000U))delay(10);
    if(time(NULL)<1704067200)return false;
    auto fetch=[&](const char *path,String &candidate){
        if(millis()-started>=budget)return false;
        XTouchHttpClient http;http.setConnectTimeout(2000);http.setTimeout(1000);
        const String url=String("https://raw.githubusercontent.com")+path;
        if(!http.begin(url.c_str(),xtouch_ota_ca)||http.GET()!=200){http.end();return false;}
        const int length=http.getSize();if(length>16384){http.end();return false;}
        candidate.reserve(length>0?length:1024);
        while(millis()-started<budget&&http.available()&&candidate.length()<=16384)candidate+=char(http.read());
        const bool complete=http.complete()&&candidate.length()>0&&candidate.length()<=16384&&(length<0||candidate.length()==unsigned(length));
        http.end();return complete;
    };
    return XTouchOtaSelection::choose(xtouch_ota_manifest_paths,fetch,XTOUCH_FIRMWARE_FILENAME,XTOUCH_FIRMWARE_MAX_BYTES,body);
}

static bool xtouch_ota_open(XTouchHttpClient &http,
                             const char *url, size_t limit, size_t expectedSize, int &total)
{
    if (!url || !String(url).startsWith("https://") || !xtouch_ota_sync_clock()) {
        ConsoleError.println("[OTA] HTTPS URL or synchronized clock unavailable");
        return false;
    }
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    http.setFollowRedirects(true);
    http.setRedirectLimit(5);
    http.useHTTP10(true);
    if (!http.begin(url, xtouch_ota_ca)) return false;
    int code = http.GET(); total = http.getSize();
    if (code != 200 || total <= 0 || size_t(total) > limit ||
        (expectedSize && size_t(total) != expectedSize)) {
        ConsoleError.printf("[OTA] HTTP=%d length=%d expected=%u\n", code, total, unsigned(expectedSize));
        http.end(); return false;
    }
    return true;
}

template<class Sink>
static bool xtouch_ota_transfer(XTouchHttpClient &http, int total, const char *expectedMD5,
                                Sink sink, void (*onProgress)(int))
{
    XTouchStackMargin::sample(XTouchStackMargin::Path::OtaHttps,true);
    MD5Builder md5; md5.begin();
    NativeHeapBuffer buffer(1024, NativeBufferMemory::PreferInternal);
    if (!buffer) return false;
    size_t received = 0;
    int lastProgress = -1; uint32_t lastData = millis();
    while (received < size_t(total) && millis() - lastData < 10000) {
        int available = http.getStreamPtr()->available();
        if (available > 0) {
            size_t wanted = min(buffer.size(), size_t(total) - received);
            wanted = min(wanted, size_t(available));
            int count = http.getStreamPtr()->readBytes(buffer.data(), wanted);
            if (count <= 0 || sink(buffer.data(), count) != size_t(count)) break;
            md5.add(buffer.data(), count); received += count; lastData = millis();
            XTouchStackMargin::sample(XTouchStackMargin::Path::OtaHttps);
            int progress = received * 100 / total;
            if (onProgress && progress != lastProgress) onProgress(progress);
            lastProgress = progress;
        } else if (!http.connected()) break;
        delay(1);
    }
    md5.calculate();
    bool success = received == size_t(total) && (!expectedMD5 || md5.toString().equalsIgnoreCase(expectedMD5));
    ConsoleInfo.printf("[OTA] download %s bytes=%u/%d\n", success ? "verified" : "failed", unsigned(received), total);
    return success;
}

bool downloadFileToFS(fs::FS &fs, const char *url, const char *fileName,
                       void (*onProgress)(int) = NULL,
                       void (*onMD5Check)(int) = NULL,
                       const char *otaMD5 = NULL, size_t expectedSize = 0)
{
    XTouchStackMargin::Scope stack(XTouchStackMargin::Path::OtaHttps);
    XTouchHttpClient http; int total;
    if (!xtouch_ota_open(http, url, otaMD5 ? XTOUCH_FIRMWARE_MAX_BYTES : 16384, expectedSize, total)) return false;
    String partial = String(fileName) + ".part";
    if (fs.exists(partial)) fs.remove(partial);
    File file = fs.open(partial, FILE_WRITE);
    if (!file) { http.end(); return false; }
    if (onMD5Check) onMD5Check(-1);
    bool success = xtouch_ota_transfer(http, total, otaMD5,
        [&file](uint8_t *data, size_t size) { return file.write(data, size); }, onProgress);
    file.flush(); file.close(); http.end();
    if (onMD5Check) onMD5Check(success);
    if (success) {
        if (fs.exists(fileName)) fs.remove(fileName);
        success = fs.rename(partial, fileName);
    }
    if (!success && fs.exists(partial)) fs.remove(partial);
    return success;
}

bool downloadFirmwareToFlash(const char *url, const char *md5, size_t size,
                               void (*onProgress)(int))
{
    XTouchStackMargin::Scope stack(XTouchStackMargin::Path::OtaHttps);
    XTouchHttpClient http; int total;
    if (!xtouch_ota_open(http, url, XTOUCH_FIRMWARE_MAX_BYTES, size, total)) return false;
    Update.onProgress(nullptr);
    if (!Update.begin(total, U_FLASH)) { http.end(); return false; }
    Update.setMD5(md5);
    bool success = xtouch_ota_transfer(http, total, md5,
        [](uint8_t *data, size_t count) { return Update.write(data, count); }, onProgress);
    http.end();
    if (success) success = Update.end();
    if (!success) Update.abort();
    ConsoleInfo.printf("[OTA] direct install %s\n", success ? "SUCCESS" : "REJECTED");
    return success;
}
#endif
