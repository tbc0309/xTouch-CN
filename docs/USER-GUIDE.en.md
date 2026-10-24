# Daily operation and notifications

[简体中文](USER-GUIDE.md) | English | [Documentation](README.en.md)

[Screen and portal](#controls) · [Notifications and faults](#notifications)

<a id="controls"></a>

## Screen and portal

### Touchscreen

| Page or control | Operation |
|---|---|
| Home printer card | Tap to switch; with three or more printers, the picker opens. Hold always opens the picker |
| Home AMS colors | Tap to cycle through multiple units |
| Light | Tap to toggle; the display follows printer feedback |
| Temperature / fan | Open the keypad, select a parameter on the left and confirm with green. Red deletes, or returns when empty |
| Nozzle More menu | View supported nozzle, rack and chamber actions |
| Filament page | Select a source, then load, unload or refresh. Hold an AMS name for details |
| Motion page | Choose a step size and move; home and extrusion actions require confirmation |
| Settings version | Tap three times to check OTA; eight times opens reset confirmation |

Wait for scrolling to stop before operating settings or selecting a printer. All three boards use the same full-screen picker and remember their image/list view (two columns on CYD/S3, three on P4), highlight the current printer in green and show a right-hand scroll indicator when content exceeds one page.

Out-of-range values are not sent. Changing keypad parameters clears unconfirmed input. Related manual controls are unavailable while printing or busy.

During normal operation, short-press the S3 power key or P4 BOOT key to toggle network standby and wake. RESET is a hardware reset; P4 POWER is a hardware power switch. Holding BOOT and tapping RESET still enters P4 download mode.

### Web management

Display and portal languages are saved independently in device settings and default to English. The startup screen uses the saved display language, or English when none is configured. Sign-in and management share the portal language; a choice made before sign-in is saved after successful authentication. Initial Wi-Fi setup initializes both values from the selected language.

Double-click editable fields in saved devices and accounts to unlock them, or focus and press F2 / Enter. Unchanged fields relock on blur; edits requiring validation remain editable until successfully saved. Printer names and account notes accept up to 63 printable ASCII characters.

After Token validation, account notes can be unlocked and saved automatically. Editing a Token requires Verify and retrieve devices again. For account logins, the phone/email and region are read-only. Clearing login retains the account and linked devices, marking it invalid until login succeeds again.

Store up to 50 Bambu accounts and 100 printers, with one active printer connection. Actual capacity depends on available board memory and credential lengths.

| Operation | Saving and application | Restart |
|---|---|---|
| Brightness, language, standby, sound, deep sleep and startup options | Valid changes save and apply automatically | No |
| Printer names, account notes and linked devices | Automatic; changed connection details reconnect the printer | No |
| New account or edited Token | Saved after successful validation, even without linked printers | No |
| Wi-Fi | Saves and reconnects; the device address may change | No |
| Import backup | Validates, saves and applies after confirmation | No |
| Administrator password | Applies on submission; sign in to the portal again | No |
| Firmware update or device reset | Runs after confirmation | Yes |
| Screen flip | Saves and applies after confirmation | Yes |

Invalid input, unfinished new accounts and failed Token validation do not overwrite saved configuration. When required, Save & restart appears with a red outline. Reading configuration on ESP32-2432S028R may briefly reconnect the printer.

### Display, standby and sound

Defaults are brightness 200, volume 20% and a 15-minute standby timeout. Automatic deep sleep, touch sounds and warning sounds default to off. Sound settings are available on the screen and portal for all three boards. Its RGB status LED defaults to on.

Network standby turns off the screen while keeping the portal and print wake available. With automatic deep sleep enabled, the default delay is 60 minutes after network standby, when the printer is idle and no work is active. Deep sleep disconnects networking; touch or RESET restarts the device with settings retained.

The screen and portal support eight languages. Built-in fonts cover interface text; some characters in external names may appear as boxes.

<a id="notifications"></a>

## Notifications and faults

| Type | Display and action |
|---|---|
| Information | Blue banner, usually dismissed after 2–3 seconds; tap to close |
| Warning | Yellow marker; permission hints are brief, important warnings require acknowledgement |
| Error | Red marker with original code; acknowledge to close |
| Confirmation | Shows printer, action and parameters, with Confirm and Cancel |

Temperature, motion, filament, drying, pause, cancel, restart and reset actions require confirmation. Confirmations expire after 30 seconds, a printer switch or a new fault. Timed-out mechanical commands are not automatically resent.

### HMS and print errors

HMS health reports and current print/device errors can share a dialog. Acknowledged persistent codes do not repeatedly notify; new faults do. The display updates when the printer clears codes.

- **Close** dismisses the local notification; it does not clear printer errors or resume printing.
- **Retry** requests recovery only for a current print error that supports it.
- **Done** reports handling complete only for a current print error that supports it.

Authorization error `84033543` and HMS `0500-0500-0001-0007` show “Some controls require LAN mode and developer mode.” They do not automatically repeat within the same printer session. A new user action that is rejected still reports failure. See [Control permissions](PRINTERS.en.md#authorization).

Default firmware displays error codes. Search the [online lookup](https://tbc0309.github.io/xTouch-CN/errors.html) or [code reference](ERROR-CODES.md) for descriptions.

### Portal messages

Automatic-save results appear at the upper right; login and Wi-Fi setup messages appear below their cards. Sign in to device management again when the portal session expires. Update the corresponding account when its Bambu Token expires. Temporary network failures do not delete saved accounts.


While the Wi-Fi setup hotspot is active, setup without signing in is limited to the hotspot interface. Access through your router still requires management authentication. Login, configuration and backup responses are not cached; signing out invalidates the session.
