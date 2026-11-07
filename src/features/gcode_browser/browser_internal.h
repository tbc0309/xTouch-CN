#pragma once
#include "policy.h"
#include "platform/idf/runtime.h"
#include "ui/lvgl_compat.h"
#include <atomic>
namespace xtouch_files {
constexpr unsigned pageSize = 8;
struct Item {
    Entry entry;
    uint16_t *pixels = nullptr;
    bool coverRead=false;
    int coverError=0;
    lv_img_dsc_t image{};
};
struct Page {
    Item items[pageSize];
    unsigned count = 0, total = 0, number = 0;
    int result = 0;
    bool fresh = false, refresh = false, imageOnly = false;
    unsigned imageIndex = 0, plate = 1;
    bool detailOnly = false;
    ~Page() {
        for (auto &i : items)
            free(i.pixels);
    }
};
struct Source {
    char host[64]{}, code[9]{}, serial[32]{};
    unsigned slot = 0, epoch = 0;
};
extern std::atomic<unsigned> epoch;
extern std::atomic<bool> open, busy, finished;
extern Page *ready;
extern Source source;
bool cancelled();
void request(unsigned page, bool refresh);
void preparePrint();
void finishPrintPreparation();
void requestDetail(const Entry &entry,unsigned page,unsigned index,unsigned plate);
enum Text {
    Title,
    Loading,
    Refresh,
    Close,
    List,
    Pictures,
    Previous,
    Next,
    Empty,
    Failed,
    Stale,
    NoMemory,
    LocalOnly,
    Print,
    Plate,
    UseAms,
    Confirm,
    Cancel,
    Busy,
    Sent,
    SendFailed,
    External,
    Permissions,
    Accepted,
    Rejected,
    NoReply,
    OriginalUnknown,
    Time,
    Weight,
    Count
};
const char *text(unsigned key);
void show();
void redraw(Page *page);
void updateImage(Page *page);
void refreshDetail();
void refreshPrintAction();
bool catalogReloadReady();
bool showCachedPage(unsigned page);
void close();
void printResult(bool sent);
void replyResult(unsigned key, unsigned error = 0, bool authorization = false);
bool printBusy();
void printPoll(bool (*publish)(const char *), String (*sequence)());
bool takePrint(Entry &entry, unsigned &plate, bool &ams);
} // namespace xtouch_files
