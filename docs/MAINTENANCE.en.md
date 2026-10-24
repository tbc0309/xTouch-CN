# Configuration, backups and security

[简体中文](MAINTENANCE.md) | English | [Documentation](README.en.md)

[Configuration and backups](#storage) · [Security](#security)

<a id="storage"></a>

## Configuration and backups

Settings are stored internally; SD is optional for daily use. An inserted card can synchronize these backup files:

| File | Contents |
|---|---|
| Root `xtouch.json` | Wi-Fi, printers, Bambu accounts and startup printer |
| `xtouch/settings.json` | Language, brightness, standby, sounds and OTA options |
| `xtouch/touch.json` | Device-specific touch calibration |
| `xtouch/hardware.json` | Screen orientation |

Calibration belongs to its device. S3/P4 use factory touch coordinates and do not require CYD calibration. The portal Maintenance page also exports and imports configuration backups.

### Time settings

`xtouch/settings.json` supports `timezone` (POSIX time-zone rules), `timezoneName` (a descriptive name), `ntpServer` and `browserTimezoneSync`:

```json
{
  "timezone": "CST-8",
  "timezoneName": "Asia/Shanghai",
  "ntpServer": "ntp.aliyun.com",
  "browserTimezoneSync": true
}
```

Browser time-zone synchronization is enabled by default, with `ntp.aliyun.com` as the default NTP server. With browser synchronization enabled and an empty time zone, the first portal sign-in fills and saves the browser's zone. Later visits preserve the saved configuration. `timezoneName` is descriptive; `timezone` determines local time. For example, New York uses `EST5EDT,M3.2.0,M11.1.0`, including daylight saving rules. Beijing time is used until a zone is configured; the clock appears only after NTP synchronization. Time-zone and NTP changes apply without restarting.

### SD synchronization and recovery

When manually editing `xtouch.json` on a TF card, a nonempty `token` takes priority over old ciphertext. To use a new plaintext Token, remove `tokenEnc` and set `tokenEncrypted` to `false`. Safely eject and reinsert the card, or restart the device, to synchronize and load the configuration.

When no valid internal configuration exists, valid SD settings are imported. When both copies exist, modification times, revisions and synchronization state determine which is used. Unsynchronized internal changes take priority when the order is uncertain. When editing `xtouch.json`, the firmware-managed `_storage` block can be kept or removed.

Importing changed SD configuration during operation restarts the device; writing a backup alone does not. Invalid JSON cannot overwrite valid internal settings. Internal changes remain saved when no card is present or a card write fails, and synchronize later.

| Additional files | Purpose |
|---|---|
| `*.previous` | Previous valid save, used to recover a damaged main file |
| `*.conflict` / `*.invalid` | Conflicting or invalid input backups; not loaded automatically |
| `*.tmp` / `*.part` | Intermediate saves and downloads |
| Root `firmware.bin` | Application firmware for SD updates |
| `xtouch/ota.json` | Temporary update manifest |

Remove unneeded backups after confirming the current configuration. Keep power and the card connected during saves and updates. See [Installation and updates](INSTALLATION.en.md#install).

### Erase and reset

Normal application updates retain internal configuration. Leaving **Erase device** unchecked in the web USB installer preserves user data. Selecting it erases internal Flash but not SD backups. Remove the SD card for a clean setup, or previous settings may be imported at startup.

Device reset clears internal configuration and the administrator password, retaining SD files. With a card, settings can be imported again; without one, repeat Wi-Fi setup. Resetting rotates the device key. An account restored with an undecryptable Token is retained but requires Token entry again.

If internal storage cannot mount, preserve available backups before reinstalling through the web installer. Board partitions differ; do not reuse another board's erase addresses.

<a id="security"></a>

## Security

- The portal uses a password and one browser session. A new login replaces the previous session. Change the initial password `12345678` at first login.
- The portal uses local HTTP. Use a trusted network and do not expose it directly to the internet.
- “Encrypt & bind” is unchecked by default for Tokens and LAN access codes, storing them as plaintext. Checking it enables device-bound encryption. After double-clicking a Token or access code, the encryption option and verification button remain disabled while the input is empty. Enter a value to enable them; changes save only after successful verification. The previous checkbox selection is retained. Keys are not exported; encrypted credentials must be entered again after restoring to another board or resetting.
- Wi-Fi passwords remain plaintext. Keep configuration and SD backups private. The administrator password is stored separately as a salted PBKDF2 hash, independently of the credential key, and excluded from backups. Importing a normal configuration does not replace it or log out the current session. Changing the administrator password logs out the current session.
- Hardware Flash encryption is not enabled; internal storage does not prevent physical extraction.
- Cloud API, cloud MQTT and online updates verify certificates. Local printer connections accept self-signed certificates. Firmware digests detect corruption, not publisher identity.
- Failed saves or insufficient memory preserve the original configuration. Limits are 50 Bambu accounts and 100 printers, with one active connection. Actual capacity depends on memory and credential lengths.

See [Compatibility and permissions](PRINTERS.en.md#authorization) for control restrictions.

A device's connection type is determined by how it was added and is read-only. After changing a LAN printer model, serial number, address or access code, select **Verify & save device**. An unchanged saved access code can be reused for verification. Unsaved settings have a red border and edited fields remain editable after a failed verification; successful saving restores the saved appearance. Reloading discards unsaved edits. Token and region changes also require verification; account labels still save automatically.

### Configuration example

The [complete configuration example](../examples/xtouch.json) contains placeholder Wi-Fi and printer details. Replace them before importing. It uses the backup format: Wi-Fi, printers and Bambu accounts at the root, with preferences inside `settings`. On a TF card, put the contents of that object directly in `xtouch/settings.json`, without another `settings` wrapper.

- `activePrinter` is a zero-based printer index; use 0 for an empty list. Limits are 100 printers and 50 Bambu accounts. Each cloud printer's `accountId` must reference an existing account `id`.
- Use UTF-8 JSON, native booleans and unquoted numbers. Omitted settings use defaults. `deepSleepAfter: 0` disables automatic deep sleep.
- Create accounts through backend verification. Do not copy UI health flags, masked credentials or verification codes into configuration. Let the device generate encrypted values; keep only the plaintext or encrypted representation of each credential.
- Firmware maintains `_storage` synchronization metadata; omit it from hand-written examples. Writes use a temporary file, content verification and the previous valid version. Unchanged content does not trigger another write.
