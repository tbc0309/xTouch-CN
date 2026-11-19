# Installation, updates and accounts

[简体中文](INSTALLATION.md) | English | [Documentation](README.en.md)
Obtain firmware from the online installer or GitHub Releases. Check the version, full board model and feature list. Browser updates use the application image; USB installation uses the full image.

<a id="install"></a>
## Installation and updates

| Board | Application image (browser/OTA/SD update) | Full image (USB installation) |
|---|---|---|
| ESP32-2432S028R | `xtouch-2.8.bin` | `xtouch-2.8-full.bin` |
| ESP32-S3-Touch-LCD-2.8 V2 | `esp32-s3-touch-lcd-2.8-v2.bin` | `esp32-s3-touch-lcd-2.8-v2-full.bin` |
| ESP32-P4-WIFI6-Touch-LCD-4.3 | `esp32-p4-wifi6-touch-lcd-4.3.bin` | `esp32-p4-wifi6-touch-lcd-4.3-full.bin` |
| LCDWIKI ES3C35P | `lcdwiki-es3c35p.bin` | `lcdwiki-es3c35p-full.bin` |

Check the exact board, touch controller and chip revision. Images are not interchangeable. Never upload a full image as an application update or SD `firmware.bin`.

### First USB installation

1. Open the [installer](https://tbc0309.github.io/xTouch-CN/) in desktop Chrome/Edge and select the board and data port.
2. Follow installation prompts. To retain internal settings, leave Erase device unchecked and export a backup first.
3. If download mode is not entered automatically, hold BOOT, tap RESET, release BOOT and reconnect.
4. Complete CYD touch calibration. S3/P4/ES3C35P do not use CYD calibration.

The web installer uses segments to preserve user-data areas. Writing a merged image at `0x0` manually may overwrite NVS and device credential keys. Erasure/reset may make encrypted credentials in old backups unreadable. Remove the SD card for a clean setup to prevent automatic import.

### Updating

When upgrading **S3 / P4 from 1.0.0** to 1.0.1, upload the matching application `.bin` through browser management, or use USB installation. The 1.0.0 online downloader cannot accept the larger images; 1.0.1 uses each board's application partition limit.

- **Browser**: upload the board-specific application in Maintenance and wait for restart.
- **Online OTA**: enable boot checks or triple-tap the version row to check a newer version. Manifest checks have a 15-second limit; confirmed firmware downloads are separate.
- **SD**: power off, put the application at the card root as `firmware.bin`, remove stale `xtouch/ota.json` and `.part` files, then power on.

Online updates use HTTPS, length and MD5 checks before selecting the alternate application slot. MD5 detects corruption, not publisher identity. Maintain power and do not remove the card during writes. Use USB recovery if the device cannot boot.

<a id="accounts"></a>
## Networking and browser management

1. Without a saved network, scan the setup QR code, join the hotspot (default password `12345678`) and configure a home 2.4 GHz network at `http://192.168.4.1/`.
2. Once connected, open Web management from screen settings and scan its address.
3. Sign in with `12345678` and change the password as prompted. A new login replaces the previous browser session.
4. Validate a LAN printer's model, address, access code and serial number, or select devices linked to a cloud account.

New network credentials are saved only after the device receives an IP address. The page reports success and the device address; the hotspot closes about 10 seconds later. A failed connection preserves the previous configuration and allows another attempt. Leaving the setup page opened from settings cancels the pending connection and restores the saved network.

When the saved network is unavailable, each connection round makes at most three attempts before offering recovery setup after about 30 seconds. A setup page opened from settings closes its hotspot after three idle minutes. With no saved network, the first-setup entry remains available. Use recovery setup after changing the router name or password; clearing the device is unnecessary.

Double-click saved editable fields, or focus them and press F2/Enter. Validation buttons appear disabled on unlock and enable for valid changes. Automatic-save fields follow their labels. Connection-source information is read-only. Screen and portal languages are independent: Chinese/English on CYD, eight languages on S3/P4/ES3C35P.

### Cloud sign-in

Standard builds include independently selectable Token and account-login modules. Select the account region, then validate an Access Token or enter the phone/email and password. Complete SMS/email verification when requested and select linked printers.

Transient network failures do not delete accounts. Revalidate an expired Token in its existing card or sign in again. Clearing local login does not revoke the cloud Token. Verification may require user input. Authentication does not authorize every printer command; see [permissions](PRINTERS.en.md#authorization).

### Optional Bluetooth provisioning

Ordinary builds use Wi-Fi hotspot setup. S3 and ES3C35P can optionally include Bluetooth provisioning with ESP BLE Provisioning. Manual device names begin with `PROV_`; the proof of possession is `abcd1234`. Bluetooth provisioning and thermometer reception are mutually exclusive build options; see [build restrictions](DEVELOPMENT.en.md#options).

A Bluetooth session accepts at most three failed credential candidates and permits correction after failure. The service stops on success, cancellation or a three-minute timeout. Cancellation or failure preserves the saved network. Wi-Fi hotspot and Bluetooth setup run separately; switching methods shuts down the previous service first.

<a id="token"></a>
## Obtaining and importing an Access Token

1. In a desktop browser, sign in to the matching [China](https://bambulab.cn/) or [international](https://bambulab.com/) website.
2. Press F12 and open Application → Storage → Cookies for the website domain.
3. Find the `token` Cookie and copy its Value. If absent, confirm sign-in and refresh.
4. Add a cloud account in device management, select the same region, paste and validate the Token, then select printers.

Copy only the value. Do not submit it to third-party collection pages or run console scripts. Treat it as a credential. Website behavior may change; [This illustrated guide](https://bambuhelper.stolaris.dev/cloud-token.html) provides a visual reference.

## Thermometers and backups

Bind the sensor MAC in thermometer settings. Plain broadcasts can omit the BLE Key; encrypted broadcasts require the correct key. See [thermometers](THERMOMETERS.en.md) for models, intervals and key extraction, and [maintenance](MAINTENANCE.en.md) for export/import/reset.

In the alternative configuration generator, select the actual controller before choosing screen and portal languages. Standard CYD firmware provides Simplified Chinese and English only; switching to CYD replaces unsupported languages with English. The generator uploads nothing. Its downloaded files contain plaintext credentials and must be kept private.

CYD supports up to 20 saved devices and 10 accounts. Before upgrading an existing CYD configuration that exceeds either limit, reduce it in the current firmware and export a backup. Imports above the limits are rejected.
