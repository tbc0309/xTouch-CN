#pragma once
#include "features/preview/preview_transport.h"
#include "features/preview/preview_ftp_cleanup.h"

namespace xtouch_network {
class FtpsSession {
  public:
    const char *host, *code;
    uint16_t port;
    bool plainData;
    bool (*isCancelled)();
    void (*report)(const XTouchPreviewFtp::CloseResult &);
    XTouchPreviewFtp::CloseResult lastClose{};
    FtpsSession(const char *h, const char *c, uint16_t p, bool plain, bool (*stop)(),
                void (*done)(const XTouchPreviewFtp::CloseResult &) = nullptr)
        : host(h), code(c), port(p), plainData(plain), isCancelled(stop), report(done) {}
    FtpsSession(const FtpsSession &) = delete;

    PreviewTransport control{isCancelled}, data{isCancelled};
    bool closing = false, closed = false, usableControl = false, dataClosed = false;
    size_t received = 0;
    uint32_t cleanupStarted = 0;
    XTouchPreviewFtp::Transfer transfer = XTouchPreviewFtp::Transfer::None;
    bool stopped() const {
        return closing ? uint32_t(millis() - cleanupStarted) >= 3000 : isCancelled();
    }
    ~FtpsSession() {
        finish();
    }
    void beginCleanup() {
        closing = true;
        cleanupStarted = millis();
        control.beginCleanup(3000);
        data.beginCleanup(3000);
    }
    void closeData() {
        if (dataClosed)
            return;
        dataClosed = true;
        bool wasOpen = data.connected();
        data.stop(true);
        received += data.shutdownDiscardedBytes();
        if (wasOpen)
            Serial.printf(
                "[PREVIEW] FTPS data closed notifySent=%u peerNotify=%u discarded=%u tls=%d\n",
                unsigned(data.closeNotifySent()), unsigned(data.peerCloseNotify()),
                data.shutdownDiscardedBytes(), data.lastError(nullptr, 0));
    }
    void closeControl() {
        control.stop();
    }
    bool closePrefix() {
        dataClosed = true;
        data.stop(false);
        control.stop(false);
        return data.isClosed() && control.isClosed();
    }
    bool usable() {
        return usableControl && control.connected() && !stopped();
    }
    bool send(const char *wire) {
        size_t bytes = stopped() ? 0 : control.print(wire);
        Serial.printf(
            "[PREVIEW] FTPS cleanup tx=%.4s bytes=%u expected=%u tls=%d connected=%u deadline=%u\n",
            wire, unsigned(bytes), unsigned(strlen(wire)), control.lastError(nullptr, 0),
            unsigned(control.connected()), unsigned(stopped()));
        return bytes == strlen(wire);
    }
    bool requestTransfer(const String &path) {
        transfer = XTouchPreviewFtp::Transfer::Pending;
        String wire = "RETR " + path + "\r\n";
        return !stopped() && control.print(wire) == wire.length();
    }
    bool finish() {
        if (closed)
            return lastClose.confirmed || lastClose.released;
        closed = true;
        Serial.printf("[PREVIEW] FTPS cleanup begin control=%u data=%u tls=%d\n",
                      unsigned(control.connected()), unsigned(data.connected()),
                      control.lastError(nullptr, 0));
        auto result = XTouchPreviewFtp::close(*this, transfer);
        lastClose = result;
        if (report)
            report(result);
        Serial.printf("[PREVIEW] FTPS close mode=%s transfer=%d abort=%d quit=%d confirmed=%u "
                      "released=%u sockets=closed elapsed=%u\n",
                      transfer == XTouchPreviewFtp::Transfer::Pending    ? "cancel"
                      : transfer == XTouchPreviewFtp::Transfer::Complete ? "complete"
                      : transfer == XTouchPreviewFtp::Transfer::Prefix   ? "prefix"
                                                                         : "idle",
                      result.transfer, result.abort, result.quit, unsigned(result.confirmed),
                      unsigned(result.released), unsigned(millis() - cleanupStarted));
        return result.confirmed || result.released;
    }
    bool readLine(String &line) {
        line = "";
        uint32_t at = millis();
        while (!stopped() && uint32_t(millis() - at) < 4000) {
            while (control.available()) {
                int c = control.read();
                if (c == '\n')
                    return true;
                if (c != '\r') {
                    if (line.length() >= 384)
                        return false;
                    line += (char)c;
                }
            }
            if (!control.connected())
                return false;
            delay(1);
        }
        return false;
    }
    int reply(String *text = nullptr) {
        String line;
        int multiline = 0;
        for (unsigned i = 0; i < 48 && !stopped(); ++i) {
            if (!readLine(line)) {
                if (closing)
                    Serial.printf(
                        "[PREVIEW] FTPS cleanup rx=none bytes=%u tls=%d connected=%u deadline=%u\n",
                        unsigned(line.length()), control.lastError(nullptr, 0),
                        unsigned(control.connected()), unsigned(stopped()));
                return -1;
            }
            int code = XTouchPreviewFtp::replyLine(line.c_str(), line.length(), multiline);
            if (code == -2)
                continue;
            if (text)
                *text = line;
            if (closing)
                Serial.printf("[PREVIEW] FTPS cleanup rx=%d bytes=%u tls=%d\n", code,
                              unsigned(line.length()), control.lastError(nullptr, 0));
            return code;
        }
        return -1;
    }
    int command(const String &line, String *text = nullptr) {
        if (isCancelled() || !control.connected())
            return -1;
        // Some printer FTP servers parse each TLS record as a complete command.
        // Keep the command and its terminator in one write (including PASS).
        String wire = line + "\r\n";
        if (control.print(wire) != wire.length())
            return -1;
        return reply(text);
    }
    bool connect() {
        control.setHandshakeTimeoutMs(4000);
        control.setIoTimeoutMs(2000);
        if (isCancelled() || !control.connect(host, port, 4000)) {
            Serial.printf("[PREVIEW] FTPS stage=tls heap=%u\n", unsigned(ESP.getFreeHeap()));
            return false;
        }
        int greeting = reply();
        if (greeting != 220) {
            Serial.printf("[PREVIEW] FTPS stage=greeting rc=%d\n", greeting);
            return false;
        }
        usableControl = true;
        int rc = command("USER bblp");
        if (rc == 331)
            rc = command(String("PASS ") + code);
        if (rc != 230) {
            Serial.printf("[PREVIEW] FTPS stage=login rc=%d\n", rc);
            return false;
        }
        for (const char *cmd : {"PBSZ 0", plainData ? "PROT C" : "PROT P", "TYPE I"}) {
            rc = command(cmd);
            if (rc != 200) {
                Serial.printf("[PREVIEW] FTPS stage=%s rc=%d\n", cmd, rc);
                return false;
            }
        }
        return true;
    }
    bool size(const String &path, uint32_t &value) {
        String s;
        if (command("SIZE " + path, &s) != 213)
            return false;
        long n = s.substring(4).toInt();
        if (n <= 0 || n > 0x7fffffffL)
            return false;
        value = n;
        return true;
    }
};
} // namespace xtouch_network
