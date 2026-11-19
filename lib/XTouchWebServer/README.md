# XTouchWebServer

Local fork of [Arduino-ESP32 WebServer 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17/libraries/WebServer), LGPL-2.1-or-later; original notices and LICENSE are retained. The public class and include are renamed to avoid linking the unpatched framework copy. No global PlatformIO package modification is required.

Security changes in `src/Parsing.cpp`:

- Multipart boundary must contain 1–70 bytes. The boundary matching buffer is fixed at 75 bytes. This backports the protection for [GHSA-8cmm-3887-r32j](https://github.com/espressif/arduino-esp32/security/advisories/GHSA-8cmm-3887-r32j).
- Request line ≤512 bytes, individual header ≤1024 bytes, combined header text ≤8192 bytes; each line has a 1500 ms deadline.
- Reject duplicate, invalid and overflowing Content-Length; reject unsupported Transfer-Encoding.
- Ordinary request body ≤32768 bytes; upload body ≤OTA slot size +4096 bytes. Application upload authorization, image validation and size checks remain required.
- Plain-body reads never allocate/read beyond the declared remaining length.

The fork preserves the synchronous server API used by the portal. When upgrading the ESP32 framework, compare these changes with upstream and rerun authentication, normal configuration/backup/OTA upload, malformed header and boundary tests before replacing this fork. This patch does not claim to fix every framework vulnerability.
