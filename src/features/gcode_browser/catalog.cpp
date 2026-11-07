#include "browser_internal.h"
#include "browser.h"
#include "time_order.h"
#include "cover_policy.h"
#include "listing_policy.h"
#include "plate_catalog.h"
#include "slice_stats.h"
#include <ctime>
#include "xtouch/printers.h"
#include "xtouch/storage.h"
#include "xtouch/card_fs.h"
#include "xtouch/types.h"
#include "xtouch/portal.h"
#include "features/preview/print_preview.h"
#ifdef XTOUCH_WAVESHARE
#include "features/camera/camera.h"
#endif
#include "features/network/ftps_session.h"
#include "features/preview/preview_zip_stream.h"
#include <esp_heap_caps.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <cerrno>
#include <PNGdec.h>
#include "features/preview/png_pixels.h"

namespace xtouch_files {
std::atomic<unsigned> epoch{0};
std::atomic<bool> open{false}, busy{false}, finished{false};
Page *ready = nullptr;
static std::atomic<bool> updateReady{false},printPreparation{false};
Source source;
char browserSerial[32]{}, browserHost[64]{}, browserCode[9]{};
bool previewHeld = false;
bool catalogFresh = false;
static std::atomic<bool> catalogChanged{false};
static std::atomic<unsigned> catalogRevision{0};
namespace {
unsigned requestedPage = 0;
bool refreshRequested = false, detailRequested = false, machineListing = true;
Entry detailEntry;
unsigned detailIndex=0, detailPlate=1;
std::atomic<bool> requestPending{false};
uint32_t started = 0, imageStarted=0, imageBudget=0;
struct ImageBudget {explicit ImageBudget(unsigned ms){imageStarted=millis();imageBudget=ms;}~ImageBudget(){imageBudget=0;}};
constexpr unsigned maxEntries = 2000;
std::vector<uint64_t> failedCatalogs,failedLinks,failedCovers,failedStats;
struct CatalogSnapshot {std::string path;unsigned revision=0;std::vector<Entry> entries;};
std::shared_ptr<const CatalogSnapshot> catalogSnapshot;
void catalogUpdated(){++catalogRevision;catalogChanged=true;}
void copyPage(Page &page,const CatalogSnapshot &snapshot){
    page.total=snapshot.entries.size();page.count=0;
    page.number=page.total?std::min(page.number,(page.total-1)/pageSize):0;
    for(unsigned at=page.number*pageSize;at<page.total && page.count<pageSize;++at)page.items[page.count++].entry=snapshot.entries[at];
}
void *allocate(size_t size) {
    void *p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : heap_caps_malloc(size, MALLOC_CAP_8BIT);
}
String root() {
    return String("/xtouch/gcode/") + source.serial;
}
bool directories(const String &path) {
    for (unsigned i = 1; i < path.length(); ++i)
        if (path[i] == '/' && !SD.mkdir(path.substring(0, i)))
            return false;
    return true;
}
bool line(File &file, String &out) {
    out = "";
    for (;;) {
        int c = file.read();
        if(c<0)break;
        if (c == '\n')
            return true;
        if (c != '\r') {
            if (out.length() >= 1024)
                return false;
            out += (char)c;
        }
    }
    return out.length() > 0;
}
bool decodeEntry(const String &wire, Entry &entry) {
    StaticJsonDocument<768> doc;
    if (deserializeJson(doc, wire))
        return false;
    entry = {doc["path"] | "", doc["stamp"] | "", doc["size"] | 0u};
    std::string title;unsigned plate=0;
    if(extractedPlate(entry.path,title,plate))entry.plate=plate;
    return pathValid(entry.path) && printable(entry.path);
}
bool append(File &file, const Entry &entry) {
    StaticJsonDocument<768> doc;
    doc["path"] = entry.path.c_str();
    doc["stamp"] = entry.stamp.c_str();
    doc["size"] = entry.size;
    String wire;
    serializeJson(doc, wire);
    return file.write((const uint8_t *)wire.c_str(), wire.length()) == wire.length() &&
           file.write(uint8_t('\n')) == 1;
}
uint64_t key(const std::string &path) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : path) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}
struct CoverLink {Entry source;unsigned plate=0;std::string digest;};
String metadataPath(const Entry &entry,const char *extension){return String(cachePath(source.serial,entry.path).c_str())+extension;}
const std::vector<std::string> *metadataNames=nullptr;
struct MetadataLookup {
 std::vector<std::string> names;const std::vector<std::string> *previous;
 MetadataLookup():previous(metadataNames){
  bool complete=true;
  for(const char *part:{"","/cache","/model","/data"}){
   String logical=root()+"/cache"+part;File directory=SD.open(logical);
   if(!directory){if(SD.exists(logical))complete=false;continue;}
   if(!directory.isDirectory()){complete=false;continue;}
   std::unique_ptr<DIR,decltype(&closedir)> scan(opendir(directory.path()),closedir);
   if(!scan){complete=false;continue;}
   while(!cancelled()){errno=0;auto item=readdir(scan.get());if(!item){if(errno)complete=false;break;}std::string name=item->d_name;
    if(suffix(name,".stats")||suffix(name,".stats.old")||suffix(name,".catalog")||suffix(name,".link")||suffix(name,".catalog.old")||suffix(name,".link.old"))names.push_back(std::string(logical.c_str())+"/"+name);
   }
  }
  std::sort(names.begin(),names.end());metadataNames=complete?&names:previous;
 }
 ~MetadataLookup(){metadataNames=previous;}
};
File openMetadata(const String &path){
 std::string name=path.c_str();
 if(metadataNames && !std::binary_search(metadataNames->begin(),metadataNames->end(),name) && !std::binary_search(metadataNames->begin(),metadataNames->end(),name+".old"))return {};
 File file=SD.open(path);if(file)return file;
 if(SD.exists(path+".old")){SD.rename(path+".old",path);return SD.open(path);}return {};
}
bool readPlateCatalog(const Entry &entry,std::vector<unsigned> &plates){
 File f=openMetadata(metadataPath(entry,".catalog"));if(!f)return false;
 DynamicJsonDocument d(16384);if(deserializeJson(d,f) || d["version"]!=1 || d["size"]!=entry.size || std::string(d["stamp"]|"")!=entry.stamp || !d["plates"].is<JsonArray>())return false;
 plates.clear();for(auto v:d["plates"].as<JsonArray>()){if(!v.is<unsigned>())return false;unsigned n=v.as<unsigned>();if(!n||n>999||std::find(plates.begin(),plates.end(),n)!=plates.end())return false;plates.push_back(n);}return true;
}
bool saveJson(const String &path,JsonDocument &d){
 if(cancelled() || !directories(path))return false;
 String tmp=path+".tmp";File f=SD.open(tmp,FILE_WRITE);if(!f)return false;
 bool good=serializeJson(d,f)>0;f.flush();f.close();
 if(good && !cancelled()){
  String old=path+".old";if(!SD.exists(path)&&SD.exists(old))SD.rename(old,path);SD.remove(old);
  bool had=SD.exists(path),moved=!had||SD.rename(path,old);
  good=moved&&SD.rename(tmp,path);
  if(good)SD.remove(old);else if(had&&moved)SD.rename(old,path);
 }else good=false;
 SD.remove(tmp);return good;
}
bool readLink(const Entry &raw,CoverLink &link){
 File f=openMetadata(metadataPath(raw,".link"));if(!f)return false;DynamicJsonDocument d(2048);
 if(deserializeJson(d,f) || d["version"]!=1 || d["size"]!=raw.size || std::string(d["stamp"]|"")!=raw.stamp)return false;
 link.source={d["source"]|"",d["sourceStamp"]|"",d["sourceSize"]|0u};link.plate=d["plate"]|0u;link.digest=d["digest"]|"";
 if(!pathValid(link.source.path)||!archive(link.source.path)||!link.source.size||!link.plate||link.plate>999||link.digest.size()!=32)return false;
 for(unsigned char c:link.digest)if(!isxdigit(c))return false;return true;
}
void saveLink(const Entry &raw,const Entry &package,unsigned plate,const char *digest){
 DynamicJsonDocument d(2048);d["version"]=1;d["size"]=raw.size;d["stamp"]=raw.stamp;d["source"]=package.path;d["sourceSize"]=package.size;d["sourceStamp"]=package.stamp;d["plate"]=plate;d["digest"]=digest;if(saveJson(metadataPath(raw,".link"),d))catalogUpdated();
}
bool currentFile(const Entry &wanted){File f=SD.open(root()+"/index.jsonl");String wire;Entry entry;while(f && !cancelled() && line(f,wire))if(decodeEntry(wire,entry)&&sameFile(wanted,entry))return true;return false;}
struct Listing {
    File &output;
    const char *directory;
    std::vector<uint64_t> &keys;
    unsigned &count;
    unsigned *folders=nullptr;
    const Entry *wanted=nullptr;
    bool *matched=nullptr;
    bool accept(const String &wire) {
        if(folders) {
            unsigned bit=1;
            for(const char *name:{"cache","model","data"}) {
                if(listedDirectory(wire.c_str(),name))*folders|=bit;
                bit<<=1;
            }
        }
        Entry entry;
        bool malformed = false;
        if (!parse(wire.c_str(), directory, entry, &malformed))
            return !malformed;
        if(wanted){if(matched && sameFile(entry,*wanted))*matched=true;return true;}
        auto hash = key(entry.path);
        if (std::find(keys.begin(), keys.end(), hash) != keys.end())
            return true;
        if (count >= maxEntries || !append(output, entry))
            return false;
        keys.push_back(hash);
        ++count;
        return true;
    }
};
// MLSD is preferred; LIST fallback is used only when the server rejects MLSD.
int listDirectory(const char *directory, bool machine, Listing &listing) {
    xtouch_network::FtpsSession ftp(source.host,source.code,990,false,cancelled);
    if(!ftp.connect())return -1;
    String passive;
    if (ftp.command("PASV", &passive) != 227)
        return -1;
    int a, b, c, d, hi, lo, at = passive.indexOf('(');
    if (at < 0 ||
        sscanf(passive.c_str() + at + 1, "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &hi, &lo) != 6 ||
        hi < 0 || hi > 255 || lo < 0 || lo > 255 || !(hi || lo))
        return -1;
    ftp.transfer = XTouchPreviewFtp::Transfer::Pending;
    String wire = String(machine ? "MLSD " : "LIST ") + directory + "\r\n";
    if (ftp.control.print(wire) != wire.length())
        return -1;
    // Servers may reject the command before opening their passive data endpoint.
    ftp.dataClosed=false;
    if (!ftp.data.connect(source.host, hi * 256 + lo, 4000, &ftp.control, false)) {
        int reply = ftp.reply();
        if(reply==500 || reply==502 || reply==504 || reply==550) {
            ftp.data.stop(false);ftp.dataClosed=true;ftp.transfer=XTouchPreviewFtp::Transfer::None;
        }
        return reply == 500 || reply == 502 || reply == 504 ? 1 : reply == 550 ? 2 : -1;
    }
    int reply = ftp.reply();
    if (reply != 125 && reply != 150) {
        if(reply==500 || reply==502 || reply==504 || reply==550) {
            ftp.data.stop(false);ftp.dataClosed=true;ftp.transfer=XTouchPreviewFtp::Transfer::None;
        }
        return reply == 500 || reply == 502 || reply == 504 ? 1 : reply == 550 ? 2 : -1;
    }
    String pending;
    size_t bytes = 0;
    uint32_t progress = millis();
    while (!cancelled()) {
        if (ftp.data.available() > 0) {
            uint8_t buffer[512];
            int n = ftp.data.read(buffer, std::min(size_t(512), size_t(ftp.data.available())));
            if (n <= 0)
                return -1;
            progress = millis();
            ftp.received += n;
            bytes += n;
            if (bytes > 1024 * 1024)
                return -1;
            for (int i = 0; i < n; ++i) {
                int ch = buffer[i];
                if (ch == '\n') {
                    if (!listing.accept(pending))
                        return -1;
                    pending = "";
                } else if (ch != '\r') {
                    if (pending.length() >= 512)
                        return -1;
                    pending += (char)ch;
                }
            }
        } else if (!ftp.data.connected()) {
            if (pending.length() && !listing.accept(pending))
                return -1;
            ftp.transfer = XTouchPreviewFtp::Transfer::Complete;
            return ftp.finish()?0:-1;
        } else if (uint32_t(millis() - progress) > 3000)
            return -1;
        else
            delay(1);
    }
    return -1;
}
bool validateSelectedEntry(const Entry &entry){
    auto slash=entry.path.find_last_of('/');
    std::string parent=slash?entry.path.substr(0,slash):"/";
    File unused;std::vector<uint64_t> keys;unsigned count=0;bool matched=false;
    Listing listing{unused,parent.c_str(),keys,count,nullptr,&entry,&matched};
    int result=listDirectory(parent.c_str(),machineListing,listing);
    if(result==1){machineListing=false;result=listDirectory(parent.c_str(),false,listing);}
    Serial.printf("[FILES] selected validation result=%d matched=%u elapsed=%u\n",result,unsigned(matched),unsigned(millis()-started));
    return result==0 && matched && !cancelled();
}
bool synchronize() {
    String index = root() + "/index.jsonl", temporary = index + ".tmp", old = index + ".old";
    if (!SD.mkdir("/xtouch") || !SD.mkdir("/xtouch/gcode") || !SD.mkdir(root()))
        return false;
    // Recover an interrupted index replacement before attempting a network refresh.
    if (!SD.exists(index) && SD.exists(old) && !SD.rename(old, index))
        return false;
    File out = SD.open(temporary, FILE_WRITE);
    if (!out)
        return false;
    std::vector<uint64_t> keys;
    keys.reserve(128);
    unsigned count = 0,folders=0,folder=0;
    bool machine=machineListing;
    for (const char *dir : {"/", "/cache", "/model", "/data"}) {
        if(folder && !(folders & (1u<<(folder-1)))){++folder;continue;}
        Listing listing{out, dir, keys, count,folder?nullptr:&folders};
        ++folder;
        int result = listDirectory(dir, machine, listing);
        if (result == 1) {
            machine=machineListing=false;
            result = listDirectory(dir, false, listing);
        }
        Serial.printf("[FILES] listing dir=%s result=%d count=%u machine=%u\n",dir,result,count,unsigned(machine));
        Serial.printf("[FILES] listing cancelled=%u elapsed=%u\n",unsigned(cancelled()),unsigned(millis()-started));
        if (result < 0 || (result == 2 && !strcmp(dir, "/")) || cancelled()) {
            out.close();
            SD.remove(temporary);
            return false;
        }
    }
    out.flush();
    out.close();
    if (cancelled())
        return false;
    if (SD.exists(old) && !SD.remove(old))
        return false;
    bool previous = SD.exists(index);
    if (previous && !SD.rename(index, old))
        return false;
    if (!SD.rename(temporary, index)) {
        if (previous)
            SD.rename(old, index);
        return false;
    }
    // Prune only paths from the previous valid manifest, after a complete refresh.
    File before = SD.open(old);
    String wire;
    while (before && line(before, wire) && !cancelled()) {
        Entry entry;
        if (!decodeEntry(wire, entry))
            continue;
        if (std::find(keys.begin(), keys.end(), key(entry.path)) != keys.end())
            continue;
        String cache = cachePath(source.serial, entry.path).c_str();
        for (const char *ext : {".png", ".png.tmp", ".png.old", ".stamp", ".catalog", ".catalog.tmp", ".catalog.old", ".stats", ".stats.tmp", ".stats.old", ".link", ".link.tmp", ".link.old"})
            SD.remove(cache + ext);
        String plates=cache+".plates";File variants=SD.open(plates);
        while(variants && !cancelled()) {File f=variants.openNextFile();if(!f)break;String name=f.name();bool directory=f.isDirectory();f.close();if(!directory)SD.remove(plates+"/"+name);}
        variants.close();SD.rmdir(plates);
    }
    before.close();
    SD.remove(old);
    ++catalogRevision;
    return true;
}
template<class Consume>
int readRemote(const std::string &path,uint32_t expectedSize,size_t budget,Consume consume) {
    if(!pathValid(path))return 25;
    xtouch_network::FtpsSession ftp(source.host, source.code, 990, false, cancelled);
    if (!ftp.connect())
        return 3;
    uint32_t actualSize = 0;
    if (!ftp.size(path.c_str(), actualSize) || !actualSize || (expectedSize && actualSize != expectedSize))
        return 25;
    String passive;
    if (ftp.command("PASV", &passive) != 227)
        return 5;
    int a, b, c, d, hi, lo, at = passive.indexOf('(');
    if (at < 0 ||
        sscanf(passive.c_str() + at + 1, "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &hi, &lo) != 6 ||
        hi < 0 || hi > 255 || lo < 0 || lo > 255 || !(hi || lo))
        return 5;
    if (!ftp.requestTransfer(path.c_str()) ||
        !ftp.data.connect(source.host, hi * 256 + lo, 4000, &ftp.control, false))
        return 6;
    int reply = ftp.reply();
    if (reply != 125 && reply != 150)
        return 6;
    auto raw=[&](uint8_t *dest,size_t length)->int {
        if(ftp.received>=actualSize || ftp.received>=budget)return 0;
        length=std::min(length,std::min(size_t(actualSize-ftp.received),budget-ftp.received));
        uint32_t progress=millis();
        while(!cancelled()) {
            int n=ftp.data.available();
            if(n>0){n=ftp.data.read(dest,std::min(length,size_t(n)));if(n>0)ftp.received+=n;return n;}
            if(!ftp.data.connected() || millis()-progress>3000)return 0;
            delay(1);
        }
        return 0;
    };
    int result=consume(raw,actualSize);
    ftp.transfer=ftp.received>=actualSize?XTouchPreviewFtp::Transfer::Complete:XTouchPreviewFtp::Transfer::Prefix;
    bool closed=ftp.finish();
    Serial.printf("[FILES] cover read result=%d bytes=%u size=%u closed=%u\n",result,unsigned(ftp.received),unsigned(actualSize),unsigned(closed));
    return closed?result:13;
}
int smallFile(const std::string &path,size_t limit,uint8_t *&bytes,size_t &size) {
    bytes=nullptr;size=0;
    return readRemote(path,0,limit,[&](auto raw,uint32_t fileSize){
        if(fileSize>limit)return 11;
        auto data=(uint8_t*)allocate(fileSize);if(!data)return 12;
        size_t used=0;while(used<fileSize){int n=raw(data+used,fileSize-used);if(n<=0){free(data);return 13;}used+=n;}
        bytes=data;size=used;return 0;
    });
}
bool imageDigest(const std::string &path,unsigned plate,char (&digest)[33]) {
    uint8_t *data=nullptr;size_t size=0;
    int result=smallFile(printerCover(path,plate,true),32,data,size);
    std::unique_ptr<uint8_t,decltype(&free)> owned(data,free);
    if(result || size!=32)return false;
    for(unsigned i=0;i<32;++i){unsigned char c=data[i];if(!isxdigit(c))return false;digest[i]=toupper(c);}
    digest[32]=0;return true;
}
bool readStats(const Entry &entry,std::vector<PlateStats> &stats){
 File f=openMetadata(metadataPath(entry,".stats"));if(!f)return false;
 DynamicJsonDocument d(16384);
 if(deserializeJson(d,f)||d["version"]!=1||d["size"]!=entry.size||std::string(d["stamp"]|"")!=entry.stamp||!d["plates"].is<JsonArray>())return false;
 stats.clear();for(auto v:d["plates"].as<JsonArray>()){
  PlateStats s;s.plate=v["plate"]|0u;if(!s.plate||s.plate>999)return false;
  for(const auto &old:stats)if(old.plate==s.plate)return false;
  if(v["seconds"].is<uint32_t>()){s.seconds=v["seconds"].as<uint32_t>();s.hasTime=true;}
  if(v["grams"].is<double>()){s.grams=v["grams"].as<double>();if(!std::isfinite(s.grams)||s.grams<0)return false;s.hasWeight=true;}
  stats.push_back(s);
 }return true;
}
bool indexStats(const Entry &entry){
 std::vector<PlateStats> stats;if(readStats(entry,stats))return false;
 uint8_t *xml=nullptr;size_t bytes=0;
 int rc=readRemote(entry.path,entry.size,XTouchPreviewArchive::scanLimit,[&](auto raw,uint32_t){
  size_t consumed=0;XTouchPreviewArchive::MemberProbe member{"Metadata/slice_info.config",65536};
  return XTouchPreviewArchive::extract(raw,cancelled,allocate,1,xml,bytes,consumed,nullptr,&member);
 });std::unique_ptr<uint8_t,decltype(&free)> owned(xml,free);
 if(rc||cancelled()||!sliceStats((const char*)xml,bytes,stats))return false;
 DynamicJsonDocument d(16384);d["version"]=1;d["size"]=entry.size;d["stamp"]=entry.stamp;
 auto values=d.createNestedArray("plates");for(const auto &s:stats){auto v=values.createNestedObject();v["plate"]=s.plate;if(s.hasTime)v["seconds"]=s.seconds;if(s.hasWeight)v["grams"]=s.grams;}
 return !d.overflowed()&&saveJson(metadataPath(entry,".stats"),d);
}
void attachStats(Entry &entry){
 Entry package=entry;unsigned plate=entry.plate;
 if(!archive(entry.path)){CoverLink link;if(!readLink(entry,link)||!currentFile(link.source))return;package=link.source;plate=link.plate;}
 std::vector<PlateStats> stats;if(!plate||!readStats(package,stats))return;
 for(const auto &s:stats)if(s.plate==plate){entry.seconds=s.seconds;entry.grams=s.grams;entry.hasTime=s.hasTime;entry.hasWeight=s.hasWeight;break;}
}
int thumbnail(const Entry &entry,uint8_t *&png,size_t &size,unsigned plate) {
    if(archive(entry.path)) {
        // The source package path and plate directly select the printer's PNG.
        // A sidecar existence check does not authenticate its contents and adds
        // another complete FTPS session. Raw-file associations still verify MD5.
        int cached=smallFile(printerCover(entry.path,plate),XTouchPreviewArchive::imageLimit,png,size);
        if(!cached)return 0;
        free(png);png=nullptr;size=0;
        if(cancelled())return 13;
        if(transientCoverError(cached))return cached;
        return readRemote(entry.path,entry.size,XTouchPreviewArchive::scanLimit,[&](auto raw,uint32_t){
            size_t consumed=0;return XTouchPreviewArchive::extract(raw,cancelled,allocate,plate,png,size,consumed);
        });
    }
    CoverLink saved;
    if(readLink(entry,saved) && currentFile(saved.source)){
        char digest[33]{};
        if(imageDigest(saved.source.path,saved.plate,digest) && saved.digest==digest)
            return smallFile(printerCover(saved.source.path,saved.plate),XTouchPreviewArchive::imageLimit,png,size);
    }
    std::string title;unsigned rawPlate=0;
    if(!extractedPlate(entry.path,title,rawPlate) || entry.path.rfind("/cache/",0)!=0)return 25;
    const size_t prefixBytes=std::min<size_t>(1024,entry.size);
    std::unique_ptr<uint8_t,decltype(&free)> prefix((uint8_t*)allocate(prefixBytes),free);
    if(!prefix)return 12;
    int result=readRemote(entry.path,entry.size,prefixBytes,[&](auto raw,uint32_t){
        size_t used=0;while(used<prefixBytes){int n=raw(prefix.get()+used,prefixBytes-used);if(n<=0)return 13;used+=n;}return 0;
    });
    if(result)return result;
    File index=SD.open(root()+"/index.jsonl");String wire;Entry candidate;
    Entry selectedSource;char selectedDigest[33]{};
    while(index && !cancelled() && line(index,wire)) {
        if(!decodeEntry(wire,candidate) || !sourceCandidate(entry.path,candidate.path,rawPlate))continue;
        char digest[33]{};if(!imageDigest(candidate.path,rawPlate,digest))continue;
        XTouchPreviewArchive::GcodeProbe probe{prefix.get(),prefixBytes,entry.size};
        result=readRemote(candidate.path,candidate.size,XTouchPreviewArchive::scanLimit,[&](auto raw,uint32_t){
            uint8_t *unused=nullptr;size_t output=0,consumed=0;
            int code=XTouchPreviewArchive::extract(raw,cancelled,allocate,rawPlate,unused,output,consumed,&probe);free(unused);return code;
        });
        Serial.printf("[FILES] cover probe result=%d matched=%u digestMatch=%u cancelled=%u elapsed=%u\n",result,unsigned(probe.matched),unsigned(!strcmp(probe.md5,digest)),unsigned(cancelled()),unsigned(millis()-started));
        if(result || !probe.matched || strcmp(probe.md5,digest))continue;
        if(!selectedSource.path.empty() && strcmp(selectedDigest,digest))return 25;
        selectedSource=candidate;strlcpy(selectedDigest,digest,sizeof(selectedDigest));
    }
    if(cancelled() || selectedSource.path.empty())return 25;
    saveLink(entry,selectedSource,rawPlate,selectedDigest);
    return smallFile(printerCover(selectedSource.path,rawPlate),XTouchPreviewArchive::imageLimit,png,size);
}
bool indexPlates(const Entry &entry){
 if(!archive(entry.path) || entry.size>XTouchPreviewArchive::scanLimit)return false;
 std::vector<unsigned> plates;if(readPlateCatalog(entry,plates))return false;
 constexpr size_t tailBytes=65557;
 std::unique_ptr<uint8_t,decltype(&free)> tail((uint8_t*)allocate(std::min<size_t>(tailBytes,entry.size)),free);
 if(!tail)return false;size_t kept=0,total=0;
 int result=readRemote(entry.path,entry.size,XTouchPreviewArchive::scanLimit,[&](auto read,uint32_t size){
  uint8_t block[1024];while(total<size){int n=read(block,std::min<size_t>(sizeof(block),size-total));if(n<=0)return 13;
   size_t capacity=std::min<size_t>(tailBytes,entry.size),at=total%capacity,take=std::min<size_t>(n,capacity-at);
   memcpy(tail.get()+at,block,take);if(size_t(n)>take)memcpy(tail.get(),block+take,n-take);
   total+=n;kept=std::min(capacity,total);
  }return 0;
 });
 if(result)return false;
 size_t first=total>kept?total%kept:0;
 if(first){std::unique_ptr<uint8_t,decltype(&free)> linear((uint8_t*)allocate(kept),free);if(!linear)return false;memcpy(linear.get(),tail.get()+first,kept-first);memcpy(linear.get()+kept-first,tail.get(),first);tail.swap(linear);}
 if(!archivePlates(tail.get(),kept,total-kept,total,plates))return false;
 DynamicJsonDocument d(16384);d["version"]=1;d["size"]=entry.size;d["stamp"]=entry.stamp;auto array=d.createNestedArray("plates");for(unsigned n:plates)array.add(n);
 return !d.overflowed() && saveJson(metadataPath(entry,".catalog"),d);
}
struct Decode {
    PNG *png;
    uint16_t *pixels;
    unsigned sw, sh, w, h, next = 0;
};
int draw(PNGDRAW *line) {
    auto &d = *(Decode *)line->pUser;
    if (cancelled())
        return 0;
    while (d.next < d.h && d.next * d.sh / d.h == unsigned(line->y)) {
        size_t offset=size_t(d.next)*d.w;
        if(!XTouchPngPixels::row(*d.png,*line,d.pixels+offset,XTouchPngPixels::alpha(d.pixels,d.w,d.h)+offset,d.w,d.sw))return 0;
        ++d.next;
    }
    if((unsigned(line->y)&15u)==15u)delay(1);
    return 1;
}
bool decode(const uint8_t *bytes, size_t count, Item &item) {
    std::unique_ptr<PNG> png(new (std::nothrow) PNG);
    if (!png || png->openRAM((uint8_t *)bytes, count, draw) != PNG_SUCCESS)
        return false;
    unsigned w = png->getWidth(), h = png->getHeight();
    if (!w || !h || w > 512 || h > 512 || png->isInterlaced()) {
        png->close();
        return false;
    }
    unsigned dw = w, dh = h;
    if (w > 128 || h > 128) {
        dw = w >= h ? 128 : std::max(1u, w * 128 / h);
        dh = h >= w ? 128 : std::max(1u, h * 128 / w);
    }
    item.pixels = (uint16_t *)allocate(XTouchPngPixels::bytes(dw,dh));
    if (!item.pixels) {
        png->close();
        return false;
    }
    Decode ctx{};
    ctx.png = png.get();
    ctx.pixels = item.pixels;
    ctx.sw = w;
    ctx.sh = h;
    ctx.w = dw;
    ctx.h = dh;
    int result = png->decode(&ctx, PNG_CHECK_CRC);
    png->close();
    if (result != PNG_SUCCESS || ctx.next != dh || cancelled()) {
        free(item.pixels);
        item.pixels = nullptr;
        return false;
    }
    item.image.header.magic = LV_IMAGE_HEADER_MAGIC;
    item.image.header.stride = dw * 2;
    item.image.header.cf = LV_COLOR_FORMAT_RGB565A8;
    item.image.header.w = dw;
    item.image.header.h = dh;
    item.image.data = (uint8_t *)item.pixels;
    item.image.data_size = XTouchPngPixels::bytes(dw,dh);
    return true;
}
void loadImage(Item &item, bool allowNetwork,unsigned plate=1) {
    item.coverError=0;
    if(item.entry.plate)plate=item.entry.plate;
    if (!printable(item.entry.path) || cancelled())
        return;
    String cache = cachePath(source.serial, item.entry.path).c_str();
    if(archive(item.entry.path) && plate>1) cache += String(".plates/")+String(plate);
    String signature = String("cover2:") + String(item.entry.size) + ":" + item.entry.stamp.c_str();
    File stamp = SD.open(cache + ".stamp");
    String saved;
    if (stamp)
        line(stamp, saved);
    stamp.close();
    if (saved == signature) {
        File existing = SD.open(cache + ".png");
        size_t n = existing ? existing.size() : 0;
        if (n && n <= XTouchPreviewArchive::imageLimit) {
            auto bytes = (uint8_t *)allocate(n);
            bool valid = bytes && existing.read(bytes, n) == n;
            existing.close();
            if (valid)
                valid = decode(bytes, n, item);
            free(bytes);
            if (valid){item.coverRead=true;return;}
        }
    }
    if (cancelled() || !allowNetwork){item.coverRead=!cancelled();return;}
    size_t n = 0;
    uint8_t *bytes = nullptr;
    int result = thumbnail(item.entry, bytes, n,plate);
    Serial.printf("[FILES] cover result=%d bytes=%u fresh=%u\n",result,unsigned(n),unsigned(allowNetwork));
    if (result || !bytes) {
        item.coverError=result?result:20;
        free(bytes);
        item.coverRead=!cancelled();
        return;
    }
    item.coverRead=true;
    bool decoded=decode(bytes,n,item);
    item.coverError=decoded?0:20;
    Serial.printf("[FILES] cover decoded=%u\n",unsigned(decoded));
    if (decoded && directories(cache)) {
        String temp = cache + ".png.tmp", target = cache + ".png", old = target + ".old";
        File file = SD.open(temp, FILE_WRITE);
        bool good = file && file.write(bytes, n) == n;
        if (file) {
            file.flush();
            file.close();
        }
        if (good && !cancelled()) {
            SD.remove(old);
            bool had = SD.exists(target);
            bool moved = !had || SD.rename(target, old);
            if (moved && SD.rename(temp, target)) {
                SD.remove(old);
                File tag = SD.open(cache + ".stamp", FILE_WRITE);
                if (tag) {
                    tag.print(signature);
                    tag.print("\n");
                    tag.flush();
                }
            } else if (had && moved)
                SD.rename(old, target);
        }
        SD.remove(temp);
    }
    free(bytes);
}
void loadNetworkImage(Item &item,unsigned budgetMs,unsigned plate=1){
    ImageBudget budget(budgetMs);
    for(unsigned attempt=0;attempt<2 && !cancelled();++attempt){
        loadImage(item,true,plate);
        if(item.pixels || !transientCoverError(item.coverError) || attempt==1)break;
        uint32_t until=millis();
        while(!cancelled() && uint32_t(millis()-until)<1000)delay(10);
    }
}
bool deliver(Page *page) {
    ready = page;
    updateReady.store(true, std::memory_order_release);
    while (updateReady.load(std::memory_order_acquire)) vTaskDelay(pdMS_TO_TICKS(10));
    return !cancelled();
}
bool readSortedPage(Page &page,const String &path) {
    auto snapshot=std::atomic_load(&catalogSnapshot);unsigned revision=catalogRevision.load();
    if(snapshot && snapshot->path==path.c_str() && snapshot->revision==revision){copyPage(page,*snapshot);return !cancelled();}
    uint32_t buildStarted=millis();
    MetadataLookup metadata;
    File index=SD.open(path);if(!index)return false;
    struct Row {int64_t time;uint32_t offset;unsigned plate;};
    std::unique_ptr<Row,decltype(&free)> rows((Row*)allocate(maxEntries*sizeof(Row)),free);
    if(!rows)return false;
    struct Alias {Entry raw;CoverLink link;};std::vector<Alias> aliases;std::vector<Entry> packages;
    String wire;Entry entry;
    while(!cancelled()) {if(!line(index,wire))break;if(!decodeEntry(wire,entry))continue;if(archive(entry.path)){packages.push_back(entry);continue;}CoverLink link;if(readLink(entry,link))aliases.push_back({entry,std::move(link)});}
    if(!index.seek(0))return false;
    time_t now=time(nullptr);tm fileLocalTime{},fileUtcTime{};localtime_r(&now,&fileLocalTime);gmtime_r(&now,&fileUtcTime);
    FileTimeContext context{now>=1577836800?fileLocalTime.tm_year+1900:0,int64_t(now),
        int(calendarSeconds(fileLocalTime.tm_year+1900,fileLocalTime.tm_mon+1,fileLocalTime.tm_mday,fileLocalTime.tm_hour,fileLocalTime.tm_min,fileLocalTime.tm_sec)-
            calendarSeconds(fileUtcTime.tm_year+1900,fileUtcTime.tm_mon+1,fileUtcTime.tm_mday,fileUtcTime.tm_hour,fileUtcTime.tm_min,fileUtcTime.tm_sec))};
    unsigned count=0;
    while(!cancelled()) {
        auto offset=index.position();if(!line(index,wire))break;
        if(!decodeEntry(wire,entry))continue;
        std::vector<unsigned> plates;
        if(!archive(entry.path)){
            bool represented=false;
            for(const auto &alias:aliases)if(sameFile(entry,alias.raw))
                for(const auto &package:packages)if(sameFile(package,alias.link.source)){represented=true;break;}
            if(represented)continue;
            plates.push_back(entry.plate);
        }else{
            bool known=readPlateCatalog(entry,plates);
            // A verified extracted G-code also proves its exact source plate.
            for(const auto &alias:aliases)if(sameFile(entry,alias.link.source)&&alias.link.plate && std::find(plates.begin(),plates.end(),alias.link.plate)==plates.end())plates.push_back(alias.link.plate);
            if(!known && plates.empty())plates.push_back(0);
            std::sort(plates.begin(),plates.end());
        }
        for(unsigned plate:plates){
            if(count>=maxEntries)return false;
            rows.get()[count++]={fileTime(entry.stamp,context),uint32_t(offset),plate};
        }
    }
    if(cancelled())return false;
    std::sort(rows.get(),rows.get()+count,[](const Row &a,const Row &b){return a.time!=b.time?a.time>b.time:a.offset!=b.offset?a.offset<b.offset:a.plate<b.plate;});
    auto built=std::make_shared<CatalogSnapshot>();built->path=path.c_str();built->revision=revision;built->entries.reserve(count);
    for(unsigned at=0;at<count && !cancelled();++at){
        if(!index.seek(rows.get()[at].offset) || !line(index,wire) || !decodeEntry(wire,entry))return false;
        entry.plate=rows.get()[at].plate;attachStats(entry);built->entries.push_back(entry);
    }
    if(cancelled() || revision!=catalogRevision.load())return false;
    std::shared_ptr<const CatalogSnapshot> immutable=built;std::atomic_store(&catalogSnapshot,immutable);
    copyPage(page,*immutable);
    Serial.printf("[FILES] catalog snapshot entries=%u build-ms=%u\n",unsigned(immutable->entries.size()),unsigned(millis()-buildStarted));
    return true;
}

void worker(void *argument) {
    std::unique_ptr<Page> page((Page *)argument);
    started = millis();
    try {
        if(page->detailOnly) {
            // Selecting cached content may cancel the list refresh. Verify the
            // catalog here before resolving a cover; this does not enable print.
            if(!page->fresh || !currentFile(page->items[0].entry))page->fresh=validateSelectedEntry(page->items[0].entry);
            auto validated=std::make_unique<Page>();validated->number=page->number;validated->imageIndex=page->imageIndex;validated->plate=page->plate;validated->detailOnly=true;validated->imageOnly=true;validated->fresh=page->fresh;validated->items[0].entry=page->items[0].entry;
            if(!deliver(validated.release()))page->fresh=false;
            started=millis();if(page->fresh)loadNetworkImage(page->items[0],30000,page->plate);else loadImage(page->items[0],false,page->plate);
            page->items[0].coverRead=true;
            attachStats(page->items[0].entry);
            // Deliver the image before optional parameter I/O; keep the dialog
            // responsive and update only its own selected file/plate afterwards.
            auto image=std::make_unique<Page>();image->number=page->number;image->imageIndex=page->imageIndex;image->plate=page->plate;image->detailOnly=true;image->imageOnly=true;image->fresh=page->fresh;
            std::swap(image->items[0],page->items[0]);
            page->items[0].entry=image->items[0].entry;page->items[0].coverRead=true;
            bool missing=!page->items[0].entry.hasTime||!page->items[0].entry.hasWeight;
            bool delivered=deliver(image.release());
            if(delivered && page->fresh && missing && !cancelled()){
                Entry package=page->items[0].entry;
                bool usable=archive(package.path);
                if(!usable){CoverLink link;if(readLink(package,link)&&currentFile(link.source)){package=link.source;usable=true;}}
                if(usable){started=millis();bool changed=false;{ImageBudget budget(15000);changed=indexStats(package);}
                    if(changed)catalogUpdated();attachStats(page->items[0].entry);}
            }
        } else {
        if (page->refresh || !page->fresh) {
            if(page->refresh){failedStats.clear();failedCatalogs.clear();failedLinks.clear();failedCovers.clear();}
            auto cached = std::make_unique<Page>();
            cached->number=page->number;cached->refresh=true;cached->result=2;
            if(readSortedPage(*cached,root()+"/index.jsonl") && cached->count){
                for(unsigned i=0;i<cached->count && !cancelled();++i)loadImage(cached->items[i],false);
                if(!cancelled())deliver(cached.release());
            }
            page->fresh = synchronize();
        }
        if(!readSortedPage(*page,root()+"/index.jsonl"))page->result=1;
        else {
            if (!page->fresh)
                page->result = 2;
            auto initial = std::make_unique<Page>();
            initial->count = page->count; initial->total = page->total; initial->number = page->number;
            initial->fresh = page->fresh; initial->refresh = page->refresh; initial->result = page->result;
            for(unsigned i=0;i<page->count;++i) initial->items[i].entry=page->items[i].entry;
            deliver(initial.release());
            bool cached[pageSize]{};
            for(unsigned i=0;i<page->count && !cancelled();++i){
                auto image=std::make_unique<Page>();image->imageOnly=true;image->number=page->number;image->imageIndex=i;image->items[0].entry=page->items[i].entry;
                loadImage(image->items[0],false);cached[i]=image->items[0].pixels;
                if(cached[i])deliver(image.release());
            }
            for(unsigned pass=0;pass<2 && !cancelled();++pass)for(unsigned i=0;i<page->count && !cancelled();++i){
                if(cached[i] || archive(page->items[i].entry.path)!=(pass==0))continue;
                auto image=std::make_unique<Page>();image->imageOnly=true;image->number=page->number;image->imageIndex=i;image->items[0].entry=page->items[i].entry;
                auto &entry=image->items[0].entry;auto identity=key(entry.path+entry.stamp+std::to_string(entry.size)+"/"+std::to_string(entry.plate));
                bool failed=std::find(failedCovers.begin(),failedCovers.end(),identity)!=failedCovers.end();
                if(!failed){
                    started=millis();if(page->fresh)loadNetworkImage(image->items[0],15000);else loadImage(image->items[0],false);
                    if(page->fresh && !image->items[0].pixels && !cancelled())failedCovers.push_back(identity);
                }
                image->items[0].coverRead=true;
                if(!cancelled())deliver(image.release());
            }
            // Metadata discovery is lower priority than every visible cover.
            if(page->fresh)for(unsigned i=0;i<page->count && !cancelled();++i){
                Entry entry=page->items[i].entry;
                if(!archive(entry.path)){CoverLink link;if(!readLink(entry,link)||!currentFile(link.source))continue;entry=link.source;}
                auto identity=key(entry.path+entry.stamp+std::to_string(entry.size));
                if(std::find(failedCatalogs.begin(),failedCatalogs.end(),identity)!=failedCatalogs.end())continue;
                bool changed=false;started=millis();{ImageBudget budget(15000);changed=indexPlates(entry);}
                if(changed)catalogUpdated();
                else {std::vector<unsigned> plates;if(!cancelled() && !readPlateCatalog(entry,plates))failedCatalogs.push_back(identity);}
            }
            // Per-plate estimates are optional and never block the visible covers.
            if(page->fresh)for(unsigned i=0;i<page->count && !cancelled();++i){
                Entry entry=page->items[i].entry;
                if(!archive(entry.path)){CoverLink link;if(!readLink(entry,link)||!currentFile(link.source))continue;entry=link.source;}
                auto identity=key(entry.path+entry.stamp+std::to_string(entry.size));
                if(std::find(failedStats.begin(),failedStats.end(),identity)!=failedStats.end())continue;
                std::vector<PlateStats> stats;if(readStats(entry,stats))continue;
                started=millis();bool changed=false;{ImageBudget budget(15000);changed=indexStats(entry);}
                if(changed)catalogUpdated();else if(!cancelled())failedStats.push_back(identity);
            }
            // Resolve one older cached cover at a time after all visible art.
            if(page->fresh)for(unsigned i=0;i<page->count && !cancelled();++i){
                CoverLink link;
                if(!cached[i] || archive(page->items[i].entry.path) || readLink(page->items[i].entry,link))continue;
                auto &entry=page->items[i].entry;auto identity=key(entry.path+entry.stamp+std::to_string(entry.size));
                if(std::find(failedLinks.begin(),failedLinks.end(),identity)!=failedLinks.end())continue;
                uint8_t *png=nullptr;size_t bytes=0;started=millis();
                {ImageBudget budget(15000);thumbnail(entry,png,bytes,1);}free(png);
                if(readLink(entry,link))break;
                if(!cancelled())failedLinks.push_back(identity);
            }
            page.reset();
        }
        }
    } catch (const std::bad_alloc &) {
        page->result = 3;
    }
    Serial.printf("[FILES] worker done stack-min=%u internal-free=%u internal-largest=%u\n",unsigned(uxTaskGetStackHighWaterMark(nullptr)),unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)),unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)));
    memset(source.code, 0, sizeof(source.code));
    ready = page.release();
    finished.store(true, std::memory_order_release);
    vTaskDelete(nullptr);
}
} // namespace
bool showCachedPage(unsigned number){
    auto snapshot=std::atomic_load(&catalogSnapshot);
    std::string path=std::string("/xtouch/gcode/")+browserSerial+"/index.jsonl";
    if(!snapshot || snapshot->path!=path)return false;
    try {
        auto page=std::make_unique<Page>();page->number=number;copyPage(*page,*snapshot);
        page->refresh=true;page->fresh=catalogFresh && snapshot->revision==catalogRevision.load();page->result=page->fresh?0:2;
        redraw(page.release());return true;
    } catch(const std::bad_alloc &){return false;}
}
bool cancelled() {
    return printPreparation.load() || requestPending.load() || !open.load() || epoch.load() != source.epoch || !xtouch_storage_sd_available() ||
           xtouch_portal_maintenance() || uint32_t(millis() - started) > 90000 ||
           (imageBudget && uint32_t(millis()-imageStarted)>=imageBudget);
}
void requestDetail(const Entry &entry,unsigned number,unsigned index,unsigned plate){
    detailEntry=entry;detailIndex=index;detailPlate=plate;requestedPage=number;detailRequested=true;refreshRequested=false;requestPending=true;
}
void preparePrint(){
    // Cancel optional reads first; printPoll waits for their worker and sockets.
    printPreparation=true;
    requestPending=false;
}
void finishPrintPreparation(){printPreparation=false;}
void request(unsigned number, bool refresh) {
    detailRequested=false;
    requestedPage = number;
    refreshRequested = refresh;
    requestPending = true;
}
void service() {
    refreshPrintAction();
    if (updateReady.load(std::memory_order_acquire)) {
        Page *p = ready; ready = nullptr;
        if (open && !requestPending && source.epoch == epoch) {
            if(p->imageOnly) updateImage(p);
            else { if(p->refresh) catalogFresh=p->fresh; redraw(p); }
        } else delete p;
        updateReady.store(false, std::memory_order_release);
    }
    if (finished.load(std::memory_order_acquire)) {
        finished = false;
        busy = false;
        refreshDetail();
        Page *p = ready;
        ready = nullptr;
        if (p && open && !requestPending && source.epoch == epoch) {
            if(p->detailOnly){updateImage(p);}
            else {if (p->refresh) catalogFresh = p->fresh;redraw(p);}
        } else
            delete p;
        // Apply the selected file's validation result before refreshing its button.
        refreshDetail();
    }
    if(printPreparation || printBusy())return;
    if(open && !busy && !requestPending && catalogChanged && catalogReloadReady()){
        catalogChanged=false;request(requestedPage,false);
    }
    if (!open || busy || printBusy() || !requestPending || xtouch_print_preview_status().busy)
        return;
    requestPending = false;
    source = {};
    source.slot = xtouch_printer_active();
    source.epoch = epoch;
    strlcpy(source.serial, xtouch_printer_serial(), sizeof(source.serial));
    if (!serialValid(source.serial) ||
        !xtouch_printer_file_credentials(source.host, sizeof(source.host), source.code,
                                          sizeof(source.code))) {
        auto p = new (std::nothrow) Page;
        if (p) {
            p->result = 4;
            redraw(p);
        }
        return;
    }
    auto p = new (std::nothrow) Page;
    if (!p) {
        replyResult(NoMemory);
        return;
    }
    p->number = requestedPage;
    p->refresh = refreshRequested;
    p->fresh = catalogFresh;
    if(detailRequested) {p->detailOnly=true;p->imageOnly=true;p->imageIndex=detailIndex;p->plate=detailPlate;p->items[0].entry=detailEntry;}
    busy = true;
    refreshDetail();
    if (xTaskCreate(worker, "gcode-files", 16384, p, 1, nullptr) != pdPASS) {
        delete p;
        busy = false;
        memset(source.code, 0, sizeof(source.code));
        replyResult(NoMemory);
    }
}
} // namespace xtouch_files
extern "C" bool xtouch_files_network_busy() {
    return xtouch_files::busy || xtouch_files::printBusy();
}
extern "C" bool xtouch_files_available() {
    char host[64]{}, code[9]{};
    const bool available = xtouch_storage_sd_available() &&
                           xtouch_printer_configured(xtouch_printer_active()) &&
                           !xtouch_printer_switch_pending() &&
                           xtouch_printer_file_credentials(host, sizeof(host), code, sizeof(code));
    memset(code, 0, sizeof(code));
    return available;
}
extern "C" void xtouch_files_open() {
    if (!xtouch_files_available() || xtouch_files::open)
        return;
    if (!xtouch_printer_file_credentials(xtouch_files::browserHost, sizeof(xtouch_files::browserHost),
                                         xtouch_files::browserCode, sizeof(xtouch_files::browserCode)))
        return;
    strlcpy(xtouch_files::browserSerial, xtouch_printer_serial(),
            sizeof(xtouch_files::browserSerial));
    xtouch_files::catalogFresh = false;
    xtouch_files::machineListing = true;
    xtouch_files::catalogChanged=false;
    xtouch_files::open = true;
    ++xtouch_files::epoch;
    xtouch_files::previewHeld = true;
    xtouch_print_preview_suspend(true);
    xtouch_files::show();
    xtouch_files::request(0, true);
}
extern "C" void xtouch_files_poll() {
    if (xtouch_files::open) {
        char host[64]{}, code[9]{};
        const bool sameRoute = xtouch_printer_file_credentials(host, sizeof(host), code, sizeof(code)) &&
            !strcmp(host, xtouch_files::browserHost) && !strcmp(code, xtouch_files::browserCode);
        memset(code, 0, sizeof(code));
        if (!xtouch_files_available() || !sameRoute ||
            strcmp(xtouch_files::browserSerial, xtouch_printer_serial()))
            xtouch_files::close();
    }
    if (!xtouch_files::open) {
        memset(xtouch_files::browserCode, 0, sizeof(xtouch_files::browserCode));
        xtouch_files::browserHost[0] = 0;
    }
    xtouch_files::service();
    if (!xtouch_files::open && !xtouch_files::busy && !xtouch_files::printBusy() &&
        xtouch_files::previewHeld) {
        xtouch_files::previewHeld = false;
#ifdef XTOUCH_WAVESHARE
        if (!xtouch_camera_status().open)
#endif
            xtouch_print_preview_suspend(false);
    }
}
