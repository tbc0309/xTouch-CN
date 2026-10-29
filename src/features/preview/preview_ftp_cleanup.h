#pragma once
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace XTouchPreviewFtp {
enum class Transfer { None, Pending, Complete, Prefix };
struct CloseResult {
    int transfer = -1, abort = -1, quit = -1;
    bool confirmed = false;
    bool released = false;
};

// A multiline reply ends only at the matching code followed by a space.
// -2 means another line is needed; -1 means malformed input.
inline int replyLine(const char *line, size_t size, int &multiline) {
    if (size < 4 || line[0] < '1' || line[0] > '5' || line[1] < '0' || line[1] > '9' ||
        line[2] < '0' || line[2] > '9')
        return multiline ? -2 : -1;
    int code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + line[2] - '0';
    if (multiline)
        return code == multiline && line[3] == ' ' ? code : -2;
    if (line[3] == '-') {
        multiline = code;
        return -2;
    }
    return line[3] == ' ' ? code : -1;
}

// A bounded prefix is deliberately not a completed FTP transfer. Release its
// sockets immediately; do not manufacture FTP acknowledgements or drain bytes.
template <class Wire> CloseResult close(Wire &wire, Transfer transfer) {
    wire.beginCleanup();
    struct Guard {
        Wire &wire;
        ~Guard() {
            wire.closeData();
            wire.closeControl();
        }
    } guard{wire};
    CloseResult result;
    if (transfer == Transfer::Prefix || transfer == Transfer::Pending) {
        result.released = wire.closePrefix();
        return result;
    }
    wire.closeData();
    if (!wire.usable())
        return result;
    if (transfer == Transfer::Complete) {
        result.transfer = wire.reply();
        if (result.transfer != 226 && result.transfer != 250)
            return result;
    }
    if (!wire.send("QUIT\r\n"))
        return result;
    result.quit = wire.reply();
    result.confirmed = result.quit == 221;
    return result;
}
} // namespace XTouchPreviewFtp
