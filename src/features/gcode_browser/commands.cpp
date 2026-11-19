#include "browser_internal.h"
#include "browser.h"
#include "print_request.h"
#include "xtouch/command_result.h"
#include "xtouch/printers.h"
#include "ui/ui.h"
#include "features/network/ftps_session.h"
namespace xtouch_files {
namespace {
struct Verification {
    Entry entry;
    unsigned plate = 1, epoch = 0;
    char host[64]{}, code[9]{}, serial[32]{};
    uint32_t started = 0;
    bool valid = false;
};
Verification verification;
std::atomic<bool> verifying{false}, verified{false};
bool awaiting = false, acknowledged = false;
unsigned sentEpoch = 0;
uint32_t sentAt = 0;
String sentSequence, sentCommand;
bool idle() {
    return !bambuStatus.non_fdm_mode && bambuStatus.print_status != XTOUCH_PRINT_STATUS_RUNNING &&
           bambuStatus.print_status != XTOUCH_PRINT_STATUS_PAUSED &&
           bambuStatus.print_status != XTOUCH_PRINT_STATUS_PREPARE;
}
bool stop() {
    return !open || verification.epoch != epoch ||
           uint32_t(millis() - verification.started) > 10000;
}
void worker(void *) {
    {
        xtouch_network::FtpsSession ftp(verification.host, verification.code, 990, false, stop);
        uint32_t size = 0;
        verification.valid = ftp.connect() && ftp.size(verification.entry.path.c_str(), size) &&
                             size == verification.entry.size && ftp.finish() && !stop();
    }
    verified.store(true, std::memory_order_release);
    vTaskDelete(nullptr);
}
} // namespace
bool printBusy() {
    return verifying || awaiting;
}
void printPoll(bool (*publish)(const char *), String (*sequence)()) {
    if (awaiting) {
        if (!open || sentEpoch != epoch || !idle())
            awaiting = false;
        else if (uint32_t(millis() - sentAt) > 15000) {
            awaiting = false;
            replyResult(acknowledged ? Accepted : NoReply);
        }
    }
    if (verified.load(std::memory_order_acquire)) {
        verified = false;
        verifying = false;
        bool sent = false;
        char host[64]{}, code[9]{};
        const bool sameRoute = xtouch_printer_file_credentials(host, sizeof(host), code, sizeof(code)) &&
            !strcmp(host, verification.host) && !strcmp(code, verification.code);
        memset(code, 0, sizeof(code));
        memset(verification.code, 0, sizeof(verification.code));
        if (sameRoute && verification.valid && !stop() && idle() && xtouch_printer_online() &&
            xtouch_files_available() && !strcmp(verification.serial, xtouch_printer_serial())) {
            DynamicJsonDocument doc(2048);
            auto seq = sequence();
            if (printRequest(doc, verification.entry, verification.plate,
                             xtouch_printer_model_at(xtouch_printer_active()), seq.c_str())) {
                String wire;
                serializeJson(doc, wire);
                sent = publish(wire.c_str());
                if (sent) {
                    sentSequence = seq;
                    sentCommand = doc["print"]["command"] | "";
                    sentAt = millis();
                    sentEpoch = epoch;
                    awaiting = true;
                    acknowledged = false;
                }
            }
        }
        printResult(sent);
    }
    if (verifying || busy || awaiting)
        return;
    Entry entry;
    unsigned plate = 1;
    bool ams = false;
    if (!takePrint(entry, plate, ams))
        return;
    if (!idle() || !xtouch_printer_online() || !xtouch_files_available()) {
        printResult(false);
        return;
    }
    verification = {};
    verification.entry = entry;
    verification.plate = plate;
    verification.epoch = epoch;
    verification.started = millis();
    strlcpy(verification.serial, xtouch_printer_serial(), sizeof(verification.serial));
    if (!xtouch_printer_file_credentials(verification.host, sizeof(verification.host),
                                          verification.code, sizeof(verification.code))) {
        printResult(false);
        return;
    }
    verifying = true;
    if (xTaskCreate(worker, "gcode-check", 16384, nullptr, 1, nullptr) != pdPASS) {
        verifying = false;
        memset(verification.code, 0, sizeof(verification.code));
        printResult(false);
    }
}
} // namespace xtouch_files

extern "C" void xtouch_files_control_reply(const char *sequence, const char *command,
                                           const char *result, const char *reason, unsigned error) {
    using namespace xtouch_files;
    if (!awaiting || sentEpoch != epoch || sentSequence != sequence || sentCommand != command)
        return;
    auto outcome = xtouch_reply_result(result, error);
    bool authorization = xtouch_reply_auth_failure(reason, error);
    if (outcome == XTOUCH_REPLY_WAIT && !authorization)
        return;
    if (outcome == XTOUCH_REPLY_REJECTED || authorization) {
        awaiting = false;
        replyResult(Rejected, error, authorization);
    } else {
        acknowledged = true;
        replyResult(Accepted);
    }
}
