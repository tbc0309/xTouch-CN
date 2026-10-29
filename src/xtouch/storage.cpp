#include "features/settings_validation.h"
#include "storage.h"
#if XTOUCH_FEATURE_XIAOMI_BLE
#include "environment.h"
#endif
#include "printers.h"
#include "profile_schema.h"
#include "platform/idf/filesystem.h"
#include "xtouch/card_fs.h"
#include "platform/idf/digest.h"
#include <esp_partition.h>
#include <cmath>
#include <time.h>
#include <utility>

namespace {
const char *paths[] = {"/xtouch.json", "/xtouch/settings.json", "/xtouch/touch.json", "/xtouch/hardware.json"
#if XTOUCH_FEATURE_XIAOMI_BLE
,xtouch_environment_config_path
#endif
};
size_t capacityFor(const char *path) {
    if (!strcmp(path, paths[0])) return 4096;
    if (!strcmp(path, paths[1])) return 2048;
    return 1024;
}
bool ready = false, sdAvailable = false, restartRequired = false;

bool knownPath(const char *path) {
    for (auto candidate : paths) if (!strcmp(candidate, path)) return true;
    return false;
}
uint32_t now() { time_t value = time(nullptr); return value >= 1577836800 && value < 4102444800LL ? value : 0; }
uint32_t credible(uint32_t value) {
    return value >= 1577836800 && value < 4102444800UL && (!now() || value <= now() + 120) ? value : 0;
}
// Stream digests without allocating another serialized configuration string.
class HashWriter:public Print {
public:
    MD5Builder hash;
    HashWriter(){hash.begin();}
    size_t write(uint8_t c)override{hash.add(&c,1);return 1;}
    size_t write(const uint8_t *data,size_t size)override{hash.add((uint8_t*)data,size);return size;}
    String finish(){hash.calculate();return hash.toString();}
};
String digest(JsonDocument &doc){HashWriter writer;serializeJson(doc,writer);return writer.finish();}
struct Record {
    explicit Record(size_t capacity) : data(capacity) {}
    DynamicJsonDocument data;
    String hash;
    uint32_t revision = 0, modified = 0;
    bool ioError=false;
    bool valid = false, deleted = false, dirty = false, external = false;
};
bool validPayload(const char *path, JsonDocument &doc) {
#if XTOUCH_FEATURE_XIAOMI_BLE
 if(!strcmp(path,xtouch_environment_config_path))return xtouch_environment_valid(doc.as<JsonVariantConst>());
#endif
    if (!doc.is<JsonObject>() || measureJson(doc) > (!strcmp(path,paths[0])?XTouchSchema::MaxConfigBytes:4096)) return false;
    if (!strcmp(path, paths[0])) {
        const char *ssid = doc["ssid"], *pwd = doc["pwd"];
        if (!ssid || !*ssid || strlen(ssid) > 32 || !pwd || strlen(pwd) > 64) return false;
        return XTouchSchema::root(doc);
    }
    if (!strcmp(path, paths[2])) {
        for (const char *key : {"xCalM", "yCalM", "xCalC", "yCalC"})
            if (!doc[key].is<float>() || !std::isfinite(doc[key].as<float>())) return false;
        return fabs(doc["xCalM"].as<float>()) > .001f && fabs(doc["xCalM"].as<float>()) < 10 &&
               fabs(doc["yCalM"].as<float>()) > .001f && fabs(doc["yCalM"].as<float>()) < 10 &&
               fabs(doc["xCalC"].as<float>()) < 4096 && fabs(doc["yCalC"].as<float>()) < 4096;
    }
    if (!strcmp(path, paths[3])) return doc["flip"].is<unsigned>() && doc["flip"].as<unsigned>() <= 1;
    return xtouch_feature_settings_valid(doc.as<JsonVariantConst>());
}
void read(fs::FS &fs, const char *path, Record &record, bool internal, const char *logicalPath = nullptr) {
    record.valid=false;record.ioError=false;
    if (!fs.exists(path)) return;
    File file = fs.open(path, FILE_READ);
    if (!file || file.size() > (!strcmp(logicalPath?logicalPath:path,paths[0]) ? XTouchSchema::MaxConfigBytes+1024 : 33792) || !file.size()) { if (file) file.close(); return; }
    uint32_t modified = credible(file.getLastWrite());
    auto error=deserializeJson(record.data,file);
    // Grow only the file being parsed, never to the size of an unrelated backup.
    bool config=!strcmp(logicalPath?logicalPath:path,paths[0]);
    while(error==DeserializationError::NoMemory && config && record.data.capacity()<XTouchSchema::MaxConfigCapacity){
        size_t next=min(XTouchSchema::MaxConfigCapacity,max((size_t)4096,record.data.capacity()+record.data.capacity()/2));
        record.data.clear();record.data.shrinkToFit();record.data=DynamicJsonDocument(next);
        if(record.data.capacity()<next)break;
        file.seek(0);error=deserializeJson(record.data,file);
    }
    file.close();
    if(error){record.ioError=error==DeserializationError::NoMemory;if(record.ioError)Serial.printf("[STORE] insufficient temporary memory: %s\n",path);return;}
    if(!record.data.is<JsonObject>())return;
    JsonObject meta = record.data["_storage"];
    String previousHash = meta["digest"] | "";
    record.revision = meta["revision"] | 0U;
    record.modified = credible(meta["modified"] | 0U);
    record.deleted = meta["deleted"] | false;
    record.dirty = meta["dirty"] | false;
    record.data.remove("_storage");
    record.hash = digest(record.data);
    record.external = previousHash != record.hash;
    // An edited SD JSON is new input, not trusted metadata from the previous backup.
    if (!internal && record.external) {
        // FAT has no timezone. A PC clock older than the previous backup is
        // ambiguous, so use conflict rules rather than pretending it is UTC.
        record.modified = record.modified && modified < record.modified ? 0 : modified;
        record.deleted = false; record.dirty = false;
    }
    if (internal && record.external) return;
    record.valid = record.deleted ? record.data.size() == 0 && !record.external
                                  : validPayload(logicalPath ? logicalPath : path, record.data);
}
bool copyFile(fs::FS &from, const char *source, fs::FS &to, const char *target) {
    if (!from.exists(source)) return false;
    File input = from.open(source, FILE_READ);
    if (!input || input.size() > XTouchSchema::MaxConfigBytes+1024) return false;
    File output = to.open(target, FILE_WRITE);
    if (!output) { input.close(); return false; }
    uint8_t block[256]; bool ok = true;
    while (input.available()) {
        int count = input.read(block, sizeof(block));
        if (count <= 0 || output.write(block, count) != size_t(count)) { ok = false; break; }
        yield();
    }
    output.flush(); output.close(); input.close(); return ok;
}
bool write(fs::FS &fs, const char *path, Record &record) {
    if(!record.deleted && !validPayload(path,record.data))return false;
    JsonObject meta=record.data.createNestedObject("_storage");
    meta["format"]=1;meta["revision"]=record.revision;meta["modified"]=record.modified;
    meta["digest"]=record.hash;meta["deleted"]=record.deleted;meta["dirty"]=record.dirty;
    if(record.data.overflowed()){record.data.remove("_storage");return false;}
    String expected=digest(record.data);
    String temporary=String(path)+".tmp",backup=String(path)+".previous";
    if(fs.exists(temporary))fs.remove(temporary);
    File file=fs.open(temporary,FILE_WRITE);
    if(!file){record.data.remove("_storage");return false;}
    size_t count=serializeJson(record.data,file);file.flush();file.close();record.data.remove("_storage");
    File verify=fs.open(temporary,FILE_READ);HashWriter actual;uint8_t buffer[256];
    bool ok=verify && verify.size()==count;
    while(ok && verify.available()){int n=verify.read(buffer,sizeof(buffer));if(n<=0){ok=false;break;}actual.write(buffer,n);yield();}
    verify.close();
    if(!ok || !count || actual.finish()!=expected){Serial.printf("[STORE] verification failed %s bytes=%u\n",path,unsigned(count));fs.remove(temporary);return false;}
    if (fs.exists(backup)) fs.remove(backup);
    if (fs.exists(path) && !fs.rename(path, backup.c_str())) { fs.remove(temporary); return false; }
    if (!fs.rename(temporary.c_str(), path)) {
        if (fs.exists(backup)) fs.rename(backup.c_str(), path);
        return false;
    }
    return true;
}
void recover(const char *path) {
    Record current(capacityFor(path)); read(SPIFFS, path, current, true);
    if (current.valid || current.ioError) return;
    String backup = String(path) + ".previous";
    Record previous(capacityFor(path)); read(SPIFFS, backup.c_str(), previous, true, path);
    if (previous.valid) {
        if (SPIFFS.exists(path)) SPIFFS.remove(path);
        if (SPIFFS.rename(backup.c_str(), path)) Serial.printf("[STORE] recovered %s\n", path);
    }
}
bool syncOne(const char *path, bool runtime, bool forceLocal = false) {
    Record local(capacityFor(path));read(SPIFFS,path,local,true);
    if(local.ioError)return false;
    // Retain comparison metadata, release the first JSON tree before reading SD.
    local.data.clear();local.data.shrinkToFit();
    Record card(capacityFor(path));read(SD,path,card,false);
    if(card.ioError)return false;
    if(!local.valid && !card.valid)return false;
    auto loadLocal=[&]()->bool{
        card.data.clear();card.data.shrinkToFit();
        DynamicJsonDocument fresh(capacityFor(path));fresh.to<JsonObject>();local.data=std::move(fresh);read(SPIFFS,path,local,true);
        if(!local.valid || local.ioError)return false;
        return true;
    };
    if (local.valid && card.valid && local.hash == card.hash && local.deleted == card.deleted) {
        if (local.dirty || card.external) { if(!loadLocal())return false;if(write(SD,path,local)){local.dirty=false;write(SPIFFS,path,local);} }
        return false;
    }
    bool importCard = !local.valid;
    if (local.valid && card.valid && !forceLocal) {
        if (!now() && local.dirty) importCard = false;
        else if (local.modified && card.modified && llabs(int64_t(local.modified) - card.modified) > 2)
            importCard = card.modified > local.modified;
        else importCard = !local.dirty && (card.external || card.revision > local.revision);
    }
    if (importCard) {
        if (local.valid) copyFile(SPIFFS, path, SPIFFS, (String(path) + ".conflict").c_str());
        card.revision = max(card.revision, local.revision) + 1;
        card.dirty = true;
        if (!write(SPIFFS, path, card)) return false;
        Serial.printf("[STORE] imported %s revision=%u\n", path, (unsigned)card.revision);
        if (write(SD, path, card)) { card.dirty = false; write(SPIFFS, path, card); }
        if (runtime) restartRequired = true;
        return true;
    }
    if(!loadLocal())return false;
    if (card.valid && (card.external || local.dirty)) copyFile(SD, path, SD, (String(path) + ".conflict").c_str());
    if (!card.valid && SD.exists(path) &&
        !copyFile(SD, path, SD, (String(path) + ".invalid").c_str())) {
        Serial.printf("[STORE] invalid SD file preserved: %s\n", path);
        return false;
    }
    if (write(SD, path, local)) {
        local.dirty = false; write(SPIFFS, path, local);
        Serial.printf("[STORE] exported %s revision=%u\n", path, (unsigned)local.revision);
    }
    return false;
}
}

bool xtouch_storage_begin() {
    if (SPIFFS.begin(false)) ready = true;
    else {
        // Formatting is permitted only for an entirely erased configuration partition.
        auto partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
        bool erased = partition; uint8_t data[256];
        for (size_t offset = 0; erased && offset < partition->size; offset += sizeof(data)) {
            if (esp_partition_read(partition, offset, data, sizeof(data)) != ESP_OK) { erased = false; break; }
            for (auto value : data) if (value != 0xff) { erased = false; break; }
        }
        if (erased && SPIFFS.format()) ready = SPIFFS.begin(false);
    }
    if (ready) for (auto path : paths) recover(path);
    Serial.printf("[STORE] internal %s total=%u used=%u\n", ready ? "ready" : "FAILED", unsigned(SPIFFS.totalBytes()), unsigned(SPIFFS.usedBytes()));
    return ready;
}
bool xtouch_storage_ready() { return ready; }
void xtouch_storage_set_sd(bool available) { sdAvailable = available; }
bool xtouch_storage_sd_available() { return sdAvailable; }
bool xtouch_storage_has(const char *path) {
    if (!ready || !knownPath(path)) return false;
    Record record(capacityFor(path)); read(SPIFFS, path, record, true); return record.valid && !record.deleted;
}
DynamicJsonDocument xtouch_storage_read(const char *path, size_t resultCapacity) {
    DynamicJsonDocument result(resultCapacity);
    if (!ready || !knownPath(path)) return result;
    Record record(capacityFor(path)); read(SPIFFS, path, record, true);
    if(record.valid && !record.deleted){if(!resultCapacity)return std::move(record.data);result.set(record.data);}
    if (result.overflowed()) result.clear();
    return result;
}
bool xtouch_storage_write(const char *path, const JsonDocument &document, bool syncSD) {
    if (!ready || !knownPath(path)) return false;
    Record old(capacityFor(path));read(SPIFFS,path,old,true);
    if(old.ioError)return false;
    old.data.clear();old.data.shrinkToFit();
    Record current(max(capacityFor(path),document.memoryUsage()+1024));current.data.set(document);current.data.remove("_storage");
    if (current.data.overflowed() || !validPayload(path,current.data)){Serial.printf("[STORE] rejected %s overflow=%u bytes=%u capacity=%u\n",path,current.data.overflowed(),unsigned(measureJson(current.data)),unsigned(current.data.capacity()));return false;}
    if (current.data.overflowed()) return false;
    current.hash = digest(current.data);
    if (old.valid && !old.deleted && old.hash == current.hash) {
        if (syncSD && old.dirty && sdAvailable) {current.data.clear();current.data.shrinkToFit();syncOne(path,false,true);}
        return true;
    }
    current.revision = old.revision + 1; current.modified = now(); current.dirty = true;
    if (!write(SPIFFS, path, current)) return false;
    Serial.printf("[STORE] saved %s revision=%u\n", path, (unsigned)current.revision);
    if (syncSD && sdAvailable) {current.data.clear();current.data.shrinkToFit();syncOne(path,false,true);}
    return true;
}
bool xtouch_storage_delete(const char *path) {
    if (!ready || !knownPath(path)) return false;
    Record old(capacityFor(path)), tombstone(capacityFor(path)); read(SPIFFS, path, old, true);
    if (old.valid && old.deleted) return true;
    tombstone.data.to<JsonObject>(); tombstone.hash = digest(tombstone.data); tombstone.deleted = true;
    tombstone.revision = old.revision + 1; tombstone.modified = now(); tombstone.dirty = true;
    if (!write(SPIFFS, path, tombstone)) return false;
    if (sdAvailable) syncOne(path, false, true);
    return true;
}
bool xtouch_storage_sync(bool runtime) {
    if (!ready || !sdAvailable) return false;
    if (!SD.exists("/xtouch")) SD.mkdir("/xtouch");
    bool imported = false;
    for (auto path : paths) imported |= syncOne(path, runtime);
    return imported;
}
bool xtouch_storage_restart_required() { return restartRequired; }
void xtouch_storage_clock_ready() {
    if (!ready || !now()) return;
    for (auto path : paths) {
        Record record(capacityFor(path)); read(SPIFFS, path, record, true);
        if (record.valid && !record.modified) {
            record.modified = now(); record.dirty = true;
            write(SPIFFS, path, record);
        }
    }
}
