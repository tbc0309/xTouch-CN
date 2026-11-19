# Using the screen and handling alerts

[简体中文](USER-GUIDE.md) | English | [Documentation](README.en.md)
<a id="controls"></a>
## Screen controls

| Control | Action and feedback |
|---|---|
| Home device card | Tap cycles up to two devices; with three or more it opens the picker. Long-press always opens it |
| Device picker | Green border marks the selected device; CYD is list-only, S3/P4/ES3C35P offer image/list modes |
| Progress | Yellow during preparation, green during actual printing, driven by printer reports |
| Temperatures/fans | Choose a keypad parameter, enter a value and confirm with green; red deletes or returns when empty |
| Motion | Select a step before moving; homing/extrusion require confirmation and are disabled while busy |
| Filament | Selecting does not load; confirm a load/unload command. Long-press an AMS name for details |
| Settings version row | Triple-tap checks updates; eight taps request reset confirmation |

Let lists stop scrolling before selecting. Out-of-range values are not sent. Switching devices clears the old selection and alerts. Source colors identify LAN/Token/phone/email, not online status.

Filament colour indicates remaining material: bright right/faded left on the filament page, bright bottom/faded top on Home. Unknown amounts remain solid. AMS temperature and humidity appear when space permits; long readings and additional AMS units can be browsed horizontally.

### Camera

The camera icon appears when the printer reports a camera. White means recording and timelapse are off; green means surveillance recording is enabled; enabled timelapse makes it blink green. A blue dot at the upper-right of the icon box marks an available local viewer. Configuration and actual recording state are interpreted separately by protocol.

Supported JPEG cameras open full-screen with light, pause/play and reconnect controls, plus a close button. An entry point does not guarantee the camera is currently reachable. See [camera protocols](../boards/CAMERA.md).

### Display, audio and standby

Defaults are brightness 200, volume 20% and 15-minute connected standby. Touch sounds, warning sounds and automatic deep sleep are off. Connected standby turns off the display but retains networking and print/touch wake. Enabled automatic deep sleep waits 60 minutes and requires idle conditions. Deep sleep disconnects networking and wake restarts the device. ES3C35P requires BOOT to wake from deep sleep; its touch input cannot wake that state. Follow the board guide for other models.

A short S3 power-key or P4/ES3C35P BOOT press toggles connected standby. RESET restarts hardware; P4 POWER switches physical power. BOOT standby and deep sleep are different states: ES3C35P retains touch wake during connected standby.

## Browser management

Double-click saved editable fields or focus and press F2/Enter. Unchanged fields relock on blur; pending changes stay editable. Validation/binding buttons first appear disabled, enable for valid changes and disappear after a successful saved binding. Read-only fields cannot unlock. Names, notes and ordinary settings save automatically; connection credentials require validation.

Screen and portal languages save independently. CYD has Chinese/English; S3/P4/ES3C35P have eight languages. Refresh uses the device's portal language rather than replacing it with browser preferences.

| Change | Result |
|---|---|
| Brightness, language, volume, display interval | Valid changes save and apply automatically |
| LAN credentials, Token, thermometer binding | Save after validation/binding; failure preserves the old configuration |
| Wi-Fi | Reconnects; the address may change |
| Management password | Requires login again |
| Firmware, reset, rotation | Restart following confirmation |

<a id="notifications"></a>
## Alerts and errors

Information is blue, warnings yellow and errors red. Confirmations identify the target and parameters. Expired confirmations, device switches and invalid state prevent execution. Timed-out mechanical commands are not automatically retried.

HMS and print errors retain original codes. S3/P4/ES3C35P add English descriptions; CYD shows codes with [online lookup](https://tbc0309.github.io/xTouch-CN/errors.html). Dialogs grow to a screen cap, then scroll the body. Long detail lines scroll individually while headings and actions remain visible.

- Close dismisses the local alert; it does not clear a printer fault.
- Retry/Done appear only for faults supporting those actions.
- Acknowledged persistent faults do not repeatedly pop up; new faults still do.
- Authorization error `84033543` or HMS `0500-0500-0001-0007` adds a yellow LAN/developer-mode warning. See [permissions](PRINTERS.en.md#authorization).

Sign in again when the management session expires, or update the affected cloud account when its credentials expire. Ordinary network errors do not erase accounts or settings.

## Print files

With an available SD card on an ESP32-S3 / ESP32-P4 controller, tap the home printer image to browse files. Names and cached images appear first; new thumbnails load progressively. Switch list/image mode or refresh manually. Sorting uses file modification time, not the mobile app's cloud print history.

Open a file to inspect its plate, thumbnail and available estimated time/weight. Reprinting is enabled only for verified printable files and plates. Check the printer, material and build plate before confirming. Cloud devices require a saved LAN profile with the same serial number to access printer storage. The controller SD caches listings, images and metadata, not entire print files.

## 32-pixel display

Expand pixel-display settings, scan and select a device, or choose manual MAC entry from the device dropdown. Enter the six-digit device password if configured, optionally saving it encrypted and device-bound. Enable and save to connect. Successful authentication collapses connection settings; reopen them to edit. Disconnect the mobile app first to avoid occupying the display connection.

Display power turns the screen on/off. Disabling linking disconnects while keeping settings and does not turn off the screen. Adjust brightness, rotation and printing layout. Choose Colour printer or Progress view while printing. The completed progress section flows right in rainbow colours; the rest uses the filament colour. Standby defaults to the autonomous built-in clock. Enable idle temperature and humidity to alternate every 10 seconds when valid readings are available; missing readings keep the clock. Printer changes settle for 3 seconds before updating the display. The display and thermometer are configured independently and can run together.
