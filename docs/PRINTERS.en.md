# Printer, AMS and command compatibility

[简体中文](PRINTERS.md) | English | [Documentation](README.en.md)

Model recognition, status parsing and command permissions are separate capabilities. Recognizing a model or displaying artwork does not prove every mechanical action was tested. Features require valid reports, accessories and firmware permissions. Unknown values are not invented or borrowed from another device.

<a id="capabilities"></a>
## Status and controls

| Category | Scope |
|---|---|
| Print | Progress, time, layers; pause/resume/cancel where supported |
| Temperatures/fans | Actual/target readings, valid chamber temperature, supported fans/speed modes |
| Filament | AMS/external sources, material, color, valid amount and environmental readings |
| Manual | Idle homing/steps, guarded extrusion/retraction, load/unload and RFID reread |
| Faults | HMS, print errors and supported confirm/retry/done actions |

A sent command is not a completed action. Check reported/physical feedback; timed-out mechanical commands are not automatically repeated. Humidity levels differ from percentages. Remaining filament is shown only when reported.

<a id="ams"></a>
## AMS and nozzles

Blue outlines indicate selection; green marks the loaded source. Tap again to deselect and long-press AMS names for details. Check confirmation before loading, unloading or rereading RFID. External sources do not offer AMS RFID reread.

Loading may unload, heat and move the printer. Conflicting controls are disabled while printing, preparing, paused, AMS-busy or reading tags. Multiple AMS units can be browsed horizontally; external entries remain separate.

### Dual nozzles and chamber heating

T0/T1, external paths and tool storage follow reported bindings. Ambiguous sources disable related controls. Nozzle input is 0–300℃, with 0 off; extrusion/retraction requires at least 170℃. Active chamber heating is offered only on supported heated models: X1E up to 60℃, other adapted heated models up to 65℃. P1S has no active chamber-heating control.

H2C storage exposes protocol-defined position/type/diameter and selected maintenance actions, not complete automatic hotend exchange. New duct/bed fields alone do not establish control support.

### Drying

| Accessory | Temperature | Duration |
|---|---|---|
| AMS 2 Pro | 45–65℃ | 1–24 hours |
| AMS HT | 45–85℃ | 1–24 hours |
| AMS/AMS Lite | No drying control | — |

Starting requires confirmation after editing parameters. Busy states restrict starts; checking/drying/cooling can request stop. Spool rotation, automatic drying and shared-feed reassignment are not offered. New AMS, dual-nozzle and tool-storage behavior depends on protocol versions, not every firmware combination is validated.

<a id="authorization"></a>
## Connections and permissions

LAN uses its address, serial and access code. Cloud uses its own account, region and serial. An account Token is not a command signature. The official signed cloud-command channel is not implemented; some commands require LAN-only and developer modes.

For `84033543` or `HMS 0500-0500-0001-0007`, check printer permissions and the matching LAN entry. Replacing a Token does not remove this restriction. Permissions vary by model/firmware; one test does not establish all-printer behavior. Closing a local alert does not clear the printer/mobile authorization fault.

CYD has limited internal memory. During cloud account verification it temporarily releases MQTT resources for HTTPS and reconnects afterwards. A brief offline indication does not mean the print stopped. Wait for current status before issuing controls and avoid repeatedly refreshing the account.

## Cameras and thumbnails

Camera protocols are independent of MQTT. S3/P4/ES3C35P support local JPEG cameras; RTSPS/H.264 has decoding/memory limits. See [camera guidance](../boards/CAMERA.md). Local camera credentials may associate by the same serial; this exception does not allow borrowing status, temperature, tasks, faults or thumbnails.

LAN thumbnails use the current task file and plate. Cloud thumbnails use the same account, serial and task API. An unknown MQTT plate may be resolved by that cloud response; an explicitly reported plate must match. Unknown never defaults to plate 1 or uses another LAN device's task. Missing thumbnails fall back to model art on S3/P4/ES3C35P. CYD uses green Bambu printing artwork and the original idle xTouch logo.

<a id="models"></a>
## Model recognition

Resources/parsers include codes for X1/X1 Carbon/X1E, P1P/P1S, A1/A1 mini, P2S, X2D, A2L and H2C/H2D/H2D Pro/H2S. Models come from reports/account records. Unknown models keep a logo rather than guessed capabilities. See the [artwork manifest](../resources/printer-images/source.json) and [model parser](../src/ui/printer_models.h).

Cloud, LAN, legacy/new fields, accessory versions and mechanical actions need separate validation. Feature descriptions do not override printer permissions or physical safety restrictions.
