# Configuration, backups and security

[简体中文](MAINTENANCE.md) | English | [Documentation](README.en.md)

<a id="storage"></a>
## Storage

An SD card is not required for daily use. Settings live in internal storage; an inserted card can hold synchronized backups. Maintenance supports configuration export/import.

| File | Content |
|---|---|
| `xtouch.json` | Wi-Fi, printers, cloud accounts and default device |
| `xtouch/settings.json` | Languages, display, time, standby, audio and updates |
| `xtouch/touch.json` | CYD calibration, specific to the device |
| `xtouch/hardware.json` | Screen orientation |

Thermometer binding is module-owned and exported as `environmentSensor` in browser backups. Do not import live readings, health state or verification codes as configuration.

Browser backups exclude the administrator password, device credential key, CYD touch calibration and separate hardware-orientation file. They are not full-flash images. Ordinary application updates preserve existing hardware settings; after erasing, recalibrate or configure orientation using the board's setup procedure.

### Backing up and restoring

1. Download a configuration from Maintenance and keep it private.
2. Check its board/source before importing and confirming.
3. Re-enter device-bound encrypted credentials when moving to another board or after a device-key reset.

Without a valid internal configuration, a valid SD copy may be imported. When both exist, versions, timestamps and synchronization state decide; ambiguous cases preserve unsynchronized internal changes. Invalid JSON does not replace valid internal data. Loading new SD configuration while running may restart; writing a backup alone does not.

`*.previous` retains the last valid file; `*.conflict`/`*.invalid` preserve conflicts or invalid input; `*.tmp`/`*.part` are temporary writes. Verify current settings and backups before cleanup. Do not interrupt saves or upgrades.

### Manual JSON

A complete browser backup has printers/accounts and a `settings` object. An SD `xtouch/settings.json` contains only that object's fields. Use UTF-8 JSON and unquoted booleans. `activePrinter` is zero-based; cloud `accountId` must reference an existing account.

CYD supports up to 20 printers and 10 accounts; other boards support up to 100 printers and 50 accounts. Actual capacity depends on memory and credential length, with one active printer. Firmware owns `_storage` and encrypted values; do not invent them. When manually replacing a plaintext Token, remove stale `tokenEnc` and set `tokenEncrypted` to `false`.

### Time

`timezone` uses POSIX rules; `timezoneName` is descriptive. Default NTP is `ntp.aliyun.com`; Beijing time is used before configuration. The clock appears after NTP synchronization. Browser timezone synchronization is on by default and fills an empty timezone once; later browsers do not overwrite it.

```json
{"timezone":"CST-8","timezoneName":"Asia/Shanghai","ntpServer":"ntp.aliyun.com","browserTimezoneSync":true}
```

Timezone/NTP changes do not require restart. A crowded home header may hide the clock despite successful synchronization.

## Reset and recovery

Application updates preserve settings. Full erasure clears internal data but not SD files. Reset clears the management password and replaces credential keys. Remove SD for a clean setup; re-enter credentials if decryption fails. Use [USB recovery](INSTALLATION.en.md#install) if boot fails, never another board's partition addresses.

Browser/SD updates verify both chip and firmware project identity. Different display boards sharing a chip are not interchangeable; legacy framework images may be rejected. Back up settings and use the matching full USB installation package when changing targets or returning to a legacy framework.

<a id="security"></a>
## Security boundaries

- Browser management is HTTP for a trusted LAN; do not expose it directly to the internet. Change initial password `12345678`.
- A new login replaces the previous session. Password changes and logout invalidate the current session.
- Access codes, Tokens, passwords and BLE Keys follow their encryption options. Device-bound encryption is not portable automatic decryption.
- Wi-Fi passwords and unencrypted credentials are plaintext. Backups are not redacted files.
- The management password is independently stored as a salted PBKDF2 hash and is excluded from ordinary configuration backups.
- Hardware Flash encryption is not enabled; credential encryption does not defeat every physical extraction.
- Cloud API/MQTT and updates verify server certificates; local printers allow self-signed certificates. Digests are not firmware signatures.
- Failed validation/saves retain the previous configuration; unfinished credential verification does not overwrite saved values.
