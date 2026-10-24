// Fill only missing settings: later browser visits never replace a saved zone.
function initializeClockSettings(settings) {
  let changed = false;
  if (!settings.ntpServer) { settings.ntpServer = "ntp.aliyun.com"; changed = true; }
  if (settings.browserTimezoneSync == null) { settings.browserTimezoneSync = true; changed = true; }
  if (settings.timezone || !settings.browserTimezoneSync) return changed;
  try {
    const name = Intl.DateTimeFormat().resolvedOptions().timeZone;
    const zones = typeof XTouchTimezones === "object" ? XTouchTimezones : {};
    let zone = zones[name];
    // Older browsers can return legacy IANA names, such as Asia/Calcutta.
    if (!zone) for (const [candidate, value] of Object.entries(zones)) {
      try { if (Intl.DateTimeFormat("en", {timeZone:candidate}).resolvedOptions().timeZone === name) { zone=value; break; } } catch {}
    }
    if (!zone && ["UTC","Etc/UTC","Etc/GMT","GMT"].includes(name)) zone="UTC0";
    if (zone) { settings.timezone=zone; settings.timezoneName=name; changed=true; }
  } catch {}
  return changed;
}
