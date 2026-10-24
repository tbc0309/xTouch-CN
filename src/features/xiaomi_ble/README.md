# Xiaomi BLE thermometer (optional)

Enable `XTOUCH_FEATURE_XIAOMI_BLE=ON` for S3 or P4, using the board Xiaomi BLE
sdkconfig defaults. Included by default in S3/P4 release builds; CYD is excluded. BLE provisioning and
this receiver are mutually exclusive. P4 uses the existing C6 Bluetooth HCI
transport.
For C6 firmware that initializes its controller at boot, also set
`XTOUCH_P4_C6_PREINITIALIZED_BT=ON`; otherwise the P4 initializes it via RPC.

The model table covers 19 model identifiers / 23 product IDs, including Linp KS2
(KS2BB), Mijia Thermometer 2/3/3 mini/3 Pro, Qingping and MiaoMiaoce devices. Only
KS2BB and LYWSD03MMC have physical sensor validation; other entries are protocol
implementations with synthetic tests, not physical-device guarantees. See the
installation guide for product IDs and verification status.

Supports bounded MiBeacon v2/v3 public packets and authenticated AES-CCM v4/v5,
including optional MAC/capability/IO fields and standard integer/float temperature
and humidity objects. Legacy encrypted v2/v3, unknown IDs, other service UUIDs,
and ATC/PVVX/BTHome formats are excluded. Public sensors can bind by MAC alone;
encrypted packets require a valid BLE Key. The model comes from the product ID.
Configure one MAC and BLE Key in the management page. Display modes are Hide,
Alternate (default), and Sensor only, with a 1–30 second alternating interval (default: 5 seconds).
The clock and sensor labels use the same font and centered row geometry.
Last-known readings remain visible with a ~ marker after ten minutes; the web
page shows the independent temperature and humidity report ages.

Encrypt & bind is optional. Checked: the key is sealed to this device vault.
Unchecked: it is stored as plaintext. Status responses never return the key.
Configuration backups include `environmentSensor` with MAC, key/keyEnc, display,
and interval. The encrypted form requires re-entry after a vault reset or on
another board. No cloud ID or cloud Token is needed for these BLE broadcasts.
