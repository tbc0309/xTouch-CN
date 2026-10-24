# Installation, updates and accounts

[简体中文](INSTALLATION.md) | English | [Documentation](README.en.md)

[Install, update and recover](#install) · [Wi-Fi and account management](#accounts) · [Retrieve and import a Token](#token)

<a id="install"></a>

## Install, update and recover

### Release files

| Board | Application for updates | Full image for USB installation |
|---|---|---|
| ESP32-2432S028R | `xtouch-2.8.bin` | `xtouch-2.8-full.bin` |
| ESP32-S3-Touch-LCD-2.8 V2 | `esp32-s3-touch-lcd-2.8-v2.bin` | `esp32-s3-touch-lcd-2.8-v2-full.bin` |
| ESP32-P4-WIFI6-Touch-LCD-4.3 | `esp32-p4-wifi6-touch-lcd-4.3.bin` | `esp32-p4-wifi6-touch-lcd-4.3-full.bin` |

Board images are not interchangeable. Application images support portal, OTA and SD updates. Each board's `ota.json` is an automatic update manifest requiring no manual editing.

Do not use the full image for OTA or rename it to the SD card's `firmware.bin`. Manually writing a full image at `0x0` overwrites NVS and loses device credential keys. Saved encrypted credentials may become unusable even if SPIFFS files remain. To retain settings, use the web installer without erasing.

### Online installation

Open the **[Web installer](https://tbc0309.github.io/xTouch-CN/)** in desktop Chrome or Edge. The page remembers the last selected board. Connect the selected board with a USB data cable, load the installer, select its port and follow the prompts. Leave **Erase device** unchecked to keep internal settings and calibration. CYD needs touch calibration after first installation; S3/P4 do not. Scan to configure Wi-Fi, then add printers in the device portal. SD is optional.

If S3/P4 cannot enter download mode automatically, power it on, hold BOOT, press and release RESET, then release BOOT. Reconnect the serial port and install again.

The installer checks the chip, Flash capacity and MD5 of each written segment. Normal installation preserves NVS and SPIFFS user data; **Erase device** erases the entire chip before installation. SD backups remain and may be imported at startup; remove the card for a fresh configuration. Encrypted credentials restored from backups may need to be entered again after erasing.

Alternatively, use the footer's [Configuration generator](https://tbc0309.github.io/xTouch-CN/config.html?lang=en) to enter Wi-Fi, LAN printers, an existing Access Token, a time zone and an NTP server. Import the downloaded JSON in the device portal, or extract the SD package at the card root. Files are generated locally in your browser and contain plaintext credentials; keep them private. Tokens and printer connections still require verification in the device portal.

### Online OTA

Enable OTA in Settings. On the next startup, the device connects to Wi-Fi and reads its [board-specific OTA manifest](DEVELOPMENT.en.md#release). Only a higher version is installed.

The application is downloaded from GitHub Releases over HTTPS, with certificate, complete length, and MD5 checks. With SD, the verified image is staged on the card, then installed into the inactive application partition after reboot. Without SD, the image is streamed directly to the inactive partition and selected only after verification. Internal Wi-Fi, printer, language, and touch calibration settings are retained.

SD staging needs enough free space for the selected board’s application image and temporary files. Direct installation does not require that card space. Both methods need network access to GitHub, its download CDN, and NTP. Keep power connected and leave the card inserted during an SD-based update. Clock synchronization or network failures skip the update and allow normal startup. Image downloads have at most two attempts. MD5 checks transfer integrity; HTTPS validates the download source.

The OTA source is `tbc0309/xTouch-CN`. Firmware configured with another update source must first be replaced with CN using the online installer or SD.

Tap the version row in Settings three times, with no more than 0.8 seconds between taps, to check and install a newer release even when startup OTA is disabled. Manifest checks have a 15-second budget covering clock synchronization, DNS, TLS and manifest reception. Wi-Fi setup, printer connection and downloading a confirmed newer firmware use separate timing. After a manual check, the printer connection resumes.

### SD updates and recovery

Configuration and backup behavior is described in [Storage](MAINTENANCE.en.md#storage).

Power off, copy the matching board's application image to the SD root as `firmware.bin`, and start the device. Before a manual update, remove `/xtouch/ota.json` and leftover `.part` files so an old manifest cannot conflict with the new image. Update files are removed after processing.

Manual SD installation without a manifest checks the ESP image format and partition size. OTA additionally checks the manifest's MD5 and length. If the device cannot boot, reinstall with the online installer. SD configuration contains access credentials; keep it private.



<a id="accounts"></a>

## Wi-Fi and account management

All boards share the web portal. Settings follow compiled capabilities. CYD, S3 V2 and P4 require their own firmware images, which are not interchangeable.

### Setup and printers

1. Without Wi-Fi settings, scan the on-screen hotspot QR code and open `http://192.168.4.1/`. Search and choose from Nearby Wi-Fi networks, or enter the name manually.
2. Once connected, visit the device address or scan **Web management** in screen settings. Use **Back to settings** below the QR code to return.
3. The initial administrator password is `12345678`; changing it is mandatory at first login. A new login replaces the previous browser session.
4. Add a LAN printer with its model, address, access code and serial number, then click **Verify & save device**. Success saves and locks the fields. Double-click to edit connection details and verify again; name changes save automatically. Add cloud printers by selecting linked devices under the corresponding account.
5. Valid changes save and apply automatically without restarting. Editing or reordering printers does not switch the printer currently shown on the screen.

Changes to connection details, account region or credentials are saved only after successful verification. Names and labels still save automatically.

The screen and management portal support Simplified Chinese, Traditional Chinese, English, French, German, Russian, Japanese and Korean. Their languages are saved independently on the device as `language` and `portalLanguage`, both defaulting to English. Changing one does not change the other. Sign-in and management share the portal language without browser-language detection or local language preferences. A choice made before sign-in is saved after successful authentication. Initial Wi-Fi setup initializes both values from the selected language; they can then be changed independently.

The public installer, configuration generator and error lookup default to English with manual Chinese/English selection. Link language parameters carry the page choice without changing device settings.

### Bluetooth provisioning (optional module)

S3/P4 builds with the optional BLE provisioning module offer Wi-Fi or Bluetooth when no network is configured. Select Bluetooth and scan the on-screen QR code inside the official **ESP BLE Provisioning** app. Choose a 2.4 GHz Wi-Fi network and enter its password. For manual discovery, use the `PROV_` prefix and proof of possession `abcd1234`. The BLE service closes after successful provisioning or three minutes. To change networks later, use Bluetooth at the bottom of the screen settings. Printer and account settings are retained. Builds without this module keep the Wi-Fi hotspot setup flow.

The initial setup chooser uses English. First-time Bluetooth provisioning initializes both languages to English. Provisioning opened from screen settings follows the current display language and retains existing language settings.

### Bambu accounts

Choose the account region, paste an **Access Token**, then validate it to retrieve linked printers. See [Get and import a Token](#token).

Successful validation collapses account details and lists linked printers for selection. Opening account management refreshes valid saved accounts; temporary network failures do not immediately invalidate them. Clearing a local login does not revoke the server Token.

Accounts and printer ownership are checked at startup and every six hours; MQTT authentication rejection also triggers a check. If a Token expires or is rejected, obtain a new one from the official website and update the existing account card.

LAN devices use cyan identifiers and Token accounts use purple. Selecting an invalid cloud profile opens a QR code targeting its account-management card.

### xTouch settings, standby and sound

Touch sounds, warning sounds and automatic deep sleep default to off. All three boards support notification sounds. Volume is adjustable on the screen and portal from 0–100%, default 20%.

Network standby turns off the display and reduces power while retaining MQTT and print wake. Optional deep sleep requires a known idle, online printer without faults, maintenance or transfers. It disconnects the portal and MQTT and cannot receive immediate print wake. Wake restarts the device without clearing configuration. All three boards support touch deep-sleep wake, and hardware reset restarts the device.

Software indicator options appear only for controllable LEDs; the RGB status LED defaults to on. S3/P4 power and charge LEDs are hardware-wired.

### Maintenance and updates

Maintenance supports configuration download, password changes, application firmware upload and reset. Upload the same board's application image, never `-full.bin`; see [Release packaging](DEVELOPMENT.en.md#release). Chip, size and partition boundaries are checked before restarting.

Three quick taps on the screen version check OTA; eight open reset confirmation. Reset clears internal configuration and the administrator password, replaces encryption keys and imports SD configuration if present. Otherwise initial setup starts. Startup OTA is configurable; manifest checks have a 15-second limit.

[Credential storage and backup safety](MAINTENANCE.en.md#security) · [Cloud authorization](PRINTERS.en.md#authorization)

### Automatic deep sleep interval

In xTouch settings → Deep sleep, enable automatic idle deep sleep and set the delay after network standby, default 60 minutes. Automatic deep sleep is off by default. Deep sleep has no timer wake: touch the screen or press RESET to start. Ordinary network standby and print wake are unaffected.

<a id="token"></a>

## Retrieve and import a Token

An Access Token authenticates a Bambu account. It is different from a printer's LAN access code. Use a desktop browser.

### Retrieve the Token

| Region | Official website |
|---|---|
| China | [bambulab.cn](https://bambulab.cn/) |
| Global | [bambulab.com](https://bambulab.com/) |

1. Open the website for your account region and sign in.
2. Press **F12** and open **Application**. Use the toolbar's `»` menu if it is hidden.
3. Expand **Storage → Cookies** and select the current Bambu website domain.
4. Find the cookie named **token** and copy its **Value**, not the entire row or cookie name.

If missing, confirm successful login, refresh and check the matching domain. Do not submit Tokens to third-party collection sites. No console script is required.

### Import into xTouch-CN

1. Open device web management and choose **Bambu accounts → Add Bambu account**.
2. Select the matching account region.
3. Paste the Token and verify it. Linked printers appear after successful verification.
4. Select printers; valid changes save and apply automatically.

Renew an existing account by updating its card instead of adding a duplicate. If the Token expires or is invalid, sign in to the official website again and repeat these steps.

Tokens are account credentials. Keep them out of screenshots, chats and public repositories; backups also contain Tokens. Successful login does not authorize every printer command; see [Cloud control](PRINTERS.en.md#authorization).

Cookie retrieval follows the [BambuHelper Token guide](https://bambuhelper.stolaris.dev/cloud-token.html). Website layouts and login policies can change.

## Thermometer (Xiaomi BLE)

S3/P4 support Xiaomi BLE thermometers. On CYD the module exceeds application Flash capacity with dual OTA slots retained.

1. Open the thermometer section in the web portal and enter the sensor MAC.
2. For encrypted advertisements, enter the BLE Key; optionally encrypt and bind it to this screen.
3. Save the binding and wait for advertisements. The model is detected automatically.
4. Choose Hide, Alternate or Sensor only. Alternation is 1–30 seconds, default 5 seconds.

See [supported models, key extraction, compatibility and reporting](THERMOMETERS.en.md) for details. Display timing does not trigger sensor measurements.
