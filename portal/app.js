const words = {
  availableNetworks: "Nearby Wi-Fi networks",
  chooseNetwork: "Choose a network",
  systemGroup: "System",
  soundGroup: "Sound",
  soundVolume: "Volume",
  deepGroup: "Deep sleep",
  startupGroup: "Startup",
  standbyGroup: "Standby",
  networkGroup: "Network",
  displayGroup: "Display",
  deepEnabled: "Auto deep sleep when idle",
  touchSound: "Touch sound",
  warningSound: "Warning sound",
  statusLed: "Status LED",
  deepAfter: "Deep sleep after standby (min)",
  deepHint: "Deep sleep disconnects the network. Touch the screen or press RESET to restart.",

  importBackup: "Import backup",
  loginTitle: "Management sign-in",
  passwordLabel: "Password",
  firmware: "Firmware",
  installFirmware: "Upload & upgrade",
  resetDevice: "Reset",
  resetHint:
    "Reset settings and admin password. Keep SD backups.",
  project: "xTouch-CN project",
  adminAccount: "Management account: administrator · password configured",
  currentPrinter: "Current device",
  connection: "Connection",
  freeMemory: "Free memory",
  running: "Uptime",
  wifiSetup: "Wi-Fi setup",
  ssid: "Wi-Fi name",
  scan: "Scan Wi-Fi",
  wifiPassword: "Wi-Fi password",
  connect: "Save and connect",
  wifiHint: "Reconnects after saving.",
  login: "Sign in",
  changePassword: "Change password",
  newPassword: "New password (8–64 bytes)",
  confirmPassword: "Confirm password",
  savePassword: "Save password",
  devices: "Devices",
  settings: "xTouch settings",
  maintenance: "Maintenance",
  logout: "Sign out",
  addPrinter: "Add LAN device",
  accounts: "Bambu accounts",
  addAccount: "Add Bambu account",
  screenLanguage: "Display language",
  soundAlerts: "Notification sounds",
  brightness: "Brightness",
  sleep: "Sleep",
  sleepOff: "Off",
  wifiKeep: "Wi-Fi password (blank to keep)",
  defaultDevice: "Startup device",
  wake: "Wake when printing starts",
  ota: "Check OTA on boot",
  backupHint:
    "Backups contain Wi-Fi credentials, access codes and cloud tokens. Keep them private.",
  backup: "Export backup",
  saveRestart: "Save & restart",
};
// localeWords is prepended by scripts/embed_portal.py from resources/portal-locales.json.
let pendingLanguage = null, languageSaving = null;
let lang = 'en',
  csrf = "",
  cfg = null,
  selected = 0,
  info = {},
  poll = null,
  cloudBusy = false;
// Invalidate asynchronous management work when authentication changes.
let savedSnapshot = null, saveTimer = null, saving = false, restartRequired = false;
let saveRetryCount = 0, saveRetryState = "";
let cleanState = "",
  sessionEpoch = 0;
let labTools = false, passwordTools = false;
const codeResendUntil = new Map();
const codeAttention = new WeakMap();
const pendingPasswords = new WeakMap();
const codeResendKey = a => `${a.region || "cn"}:${a.account || ""}`;
function resendCountdown(button, a) {
  const key = codeResendKey(a);
  button.dataset.resendKey = key;
  const update = () => {
    const seconds = Math.max(0, Math.ceil(((codeResendUntil.get(key) || 0) - Date.now()) / 1000));
    button.textContent = t("获取验证码", "Get code") + (seconds ? ` (${seconds}s)` : "");
    button.disabled = cloudBusy || seconds > 0;
  };
  update();
  const timer = setInterval(() => { if (!button.isConnected) return clearInterval(timer); update(); }, 250);
}
function enableLabTools(e) {
  if (!cfg || $("app").hidden || cloudBusy || e.repeat) return;
  if (!e.ctrlKey || !e.altKey || !e.shiftKey || e.metaKey) return;
  const key = e.key.toLowerCase();
  if (key !== "t" && key !== "p") return;
  e.preventDefault();
  if (key === "t") labTools = !labTools;
  else passwordTools = !passwordTools;
  accounts();
}

document.addEventListener("keydown", enableLabTools);
function collapseAccounts() {
  for (const a of cfg?.cloudAccounts || []) a.expanded = false;
}
function enterLogin() {
  labTools = passwordTools = false;
  clearTimeout(saveTimer); savedSnapshot = null; restartRequired = false;
  saveRetryCount = 0; saveRetryState = "";
  sessionEpoch++;
  collapseAccounts();
  clearInterval(poll);
  cloudBusy = false;
  csrf = "";
  cfg = null;
  cleanState = "";
  $("accounts").replaceChildren();
  $("profiles").replaceChildren();
  $("editor").replaceChildren();
  $("logout").hidden = true;
  $("app").hidden = true;
  $("change").hidden = true;
  $("login").hidden = false;
  $("guestFeedback").append($("status"));
  message("");
  for (const b of document.querySelectorAll("#addAccount,#addSimulator,#save,#language"))
    b.disabled = false;
}
const $ = (id) => document.getElementById(id),
  t = (zh, en) => (lang === "zh" ? zh : localeWords[en]?.[languageCodes.indexOf(lang)] || en),
  defaults = new Map(
    [...document.querySelectorAll("[data-t]")].map((el) => [
      el.dataset.t,
      el.textContent,
    ]),
  );
function language() {
  document.documentElement.lang = lang === "zh" ? "zh-CN" : lang;
  $("language").value = lang;updateSleepValue();
  const firmware = $("appFirmware");
  firmware.textContent = t("仅选择应用固件 {file}。", "Application firmware only: {file}.").replace("{file}", firmware.dataset.firmware);
  for (const el of document.querySelectorAll("[data-t]"))
    el.textContent =
      t(defaults.get(el.dataset.t), words[el.dataset.t]);
  if (cfg) {
    profiles();
    accounts();
    updateInfo(info);
    message("");
  }
}
async function saveLanguageChoice() {
  if (!pendingLanguage || !csrf || info.ap) return;
  if (languageSaving) return languageSaving;
  languageSaving = (async () => {
    while (pendingLanguage) {
      const value = pendingLanguage;
      await api('/api/language', {language:value});
      if (pendingLanguage === value) pendingLanguage = null;
      info.portalLanguage = value;
      if (cfg) cfg.settings.portalLanguage = value;
      if (savedSnapshot) savedSnapshot.settings.portalLanguage = value;
    }
  })();
  try { await languageSaving; } finally { languageSaving = null; }
}
$("language").onchange = async (e) => {
  if (!languageCodes.includes(e.target.value)) return;
  lang = e.target.value;
  pendingLanguage = lang;
  language();
  try { await saveLanguageChoice(); } catch(error) { message(failure(error),true); }
};
language();
const errorText = {
  wifi_scan_failed: ["Wi-Fi 扫描失败，请稍后重试。", "Wi-Fi scan failed. Please retry."],
  lan_connection_failed: ["无法连接设备，请检查地址、网络和局域网模式。", "Cannot connect. Check the address, network and LAN mode."],
  lan_auth_rejected: ["访问码验证失败，请检查访问码。", "Access code rejected. Check the access code."],
  lan_status_timeout: ["未收到设备状态，请检查序列号后重试。", "No device status received. Check the serial number and retry."],
  invalid_name: [
    "自定义名称仅限英文、数字和英文符号。",
    "Custom names allow ASCII letters, numbers and symbols only.",
  ],
  login_required: [
    "登录已失效，请重新登录。",
    "Session expired. Sign in again.",
  ],
  login_throttled: [
    "尝试过多，请稍后重试。",
    "Too many attempts. Retry shortly.",
  ],
  origin: [
    "访问来源不匹配，请通过设备地址重新打开。",
    "Origin mismatch. Reopen the device address.",
  ],
  csrf: ["页面会话已更新，请刷新后重试。", "Session changed. Reload to retry."],
  password_change_required: [
    "请先修改初始管理密码。",
    "Change the initial password first.",
  ],
  password_length: [
    "密码须为8–64字节，且不能使用初始密码。",
    "Use 8–64 bytes and change the default password.",
  ],
  invalid_json: ["请求格式错误。", "Invalid request format."],
  invalid_configuration: [
    "配置不完整或超出设备限制。",
    "Invalid or oversized configuration.",
  ],
  invalid_settings: ["请检查设置的范围。", "Check setting ranges."],
  invalid_wifi: [
    "请检查Wi-Fi名称和密码。",
    "Check the Wi-Fi name and password.",
  ],
  storage_failed: [
    "保存失败，请检查存储空间。",
    "Save failed. Check storage space.",
  ],
  settings_failed: [
    "设置保存失败，请重试。",
    "Settings could not be saved. Retry.",
  ],
  power_unavailable: ["当前状态不支持深睡或设备忙。", "Deep sleep unavailable or device busy."],
  sound_unavailable: ["当前无法播放，请先唤醒屏幕。", "Sound unavailable. Wake the screen first."],
  low_memory: ["可用内存不足，请稍后重试。", "Low memory. Retry shortly."],
  cloud_busy: [
    "云请求处理中，请稍候。",
    "Cloud request in progress. Please wait.",
  ],
  not_ready: ["设备忙，请稍后重试。", "Device busy. Retry shortly."],
  update_failed: ["固件升级失败。", "Firmware failed."],
  application_firmware_required: [
    "请选择ESP32应用固件。",
    "Choose ESP32 application firmware.",
  ],
  one_firmware_required: [
    "每次仅上传一个固件。",
    "Upload one firmware image at a time.",
  ],
  firmware_write_failed: [
    "固件写入失败或过大。",
    "Firmware write failed or image too large.",
  ],
  update_begin_failed: [
    "无法开始升级，请稍后重试。",
    "Unable to start update. Retry shortly.",
  ],
  invalid_firmware: ["固件校验失败。", "Firmware verification failed."],
  upload_aborted: ["上传已中断。", "Upload interrupted."],
  out_of_memory: ["内存不足，请稍后重试。", "Out of memory. Retry shortly."],
  confirmation_required: ["请确认重置操作。", "Confirm the reset operation."],
  credential_key_changed: ["设备密钥已更换，请重新填写 Token 或登录。", "Device key changed. Re-enter the Token or sign in again."],
  password_key_changed: [
    "设备密钥已更换，请重新输入账号密码或导入 Token。",
    "Device key changed. Re-enter the password or import a token.",
  ],
  checking_accounts: [
    "正在检查拓竹账号，请稍候重试。",
    "Checking Bambu accounts. Retry shortly.",
  ],
  wait_before_retry: [
    "请稍候再试，当前请求尚在冷却。",
    "Please wait before retrying.",
  ],
  cloud_blocked: [
    "拓竹安全服务拦截了请求（Cloudflare）。建议使用官方登录取得 Token 后导入。",
    "Bambu security service blocked this request (Cloudflare). We recommend importing a token from official sign-in.",
  ],
  csrf_required: [
    "官方登录要求浏览器会话验证，请在官方网页登录后导入 Token。",
    "Browser session verification is required. Sign in officially and import a token.",
  ],
  cloud_forbidden: [
    "拓竹服务拒绝请求，不能据此判断密码错误。可尝试验证码或导入 Token。",
    "Bambu rejected this request; this does not establish an incorrect password. Try a verification code or import a token.",
  ],
  authentication_rejected: [
    "账号凭据或 Token 未被接受，请检查账号区域与凭据。",
    "Credentials or token rejected. Check the account region and credentials.",
  ],
  verification_required: [
    "请输入收到的验证码。",
    "Enter the verification code you received.",
  ],
  two_factor_required: [
    "请输入身份验证器的验证码。",
    "Enter the code from your authenticator.",
  ],
  clock_not_ready: [
    "设备时间尚未同步，请稍后重试。",
    "Device clock is not synchronized. Retry shortly.",
  ],
  cloud_network_error: [
    "连接拓竹服务失败，请检查网络与设备时间。",
    "Unable to connect. Check the network and device clock.",
  ],
  cloud_rate_limited: [
    "拓竹服务限制了请求频率，请稍后重试。",
    "Bambu rate limit reached. Retry later.",
  ],
  cloud_request_failed: [
    "云端请求失败，请查看 HTTP 状态。",
    "Cloud request failed. Check the HTTP status.",
  ],
  invalid_or_busy: [
    "请检查账号、密码、验证码或 Token 是否填写完整。",
    "Check the account, password, verification code or token.",
  ],
  invalid_password: ["管理密码错误。", "Incorrect management password."],
  response_too_large: [
    "服务端响应超出设备处理上限。",
    "Response exceeds the device limit.",
  ],
};
function failure(error) {
  return error.name === "TimeoutError" || error.name === "AbortError"
    ? t("请求超时，请重试。", "Request timed out. Retry.")
    : error instanceof TypeError
      ? t("网络连接中断，请重试。", "Network connection interrupted. Retry.")
      : error.message;
}
function explain(code) {
  return errorText[code]
    ? t(...errorText[code])
    : String(code);
}
function message(value, error = false) {
  $("status").textContent = value;
  $("status").title = value;
  $("status").classList.toggle("error", error);
}
function updateInfo(d) {
  info = d;
  if (!pendingLanguage && languageCodes.includes(d.portalLanguage) && d.portalLanguage !== lang) {
    lang=d.portalLanguage;
    if(cfg){cfg.settings.portalLanguage=lang;if(savedSnapshot)savedSnapshot.settings.portalLanguage=lang;}
    language();
  }
  if (typeof d.version === "string" && /^\d+\.\d+\.\d+$/.test(d.version))
    $("portalVersion").textContent = d.version;
  $("statPrinter").textContent = d.printer || "—";
  $("statConnection").textContent =
    `${d.mode || "LAN"} · ${d.online ? t("在线", "Online") : t("等待连接", "Waiting")}`;
  $("statHeap").textContent = `${Math.round(d.heap / 1024)} KB`;
  $("statUptime").textContent =
    `${Math.floor(d.uptime / 3600)}h ${Math.floor((d.uptime % 3600) / 60)}m`;
  if (cfg && d.accountHealth && !cloudBusy && !document.activeElement?.closest("#accounts")) {
    let changed = false;
    for (const a of cfg.cloudAccounts || []) {
      if (!(a.id in d.accountHealth)) continue;
      if (a.token && !a.tokenSaved && !a.verified) continue;
      const state = d.accountHealth[a.id];
      const health = state < 0 ? "invalid" : state > 0 ? "valid" : "unknown";
      if (a.health === health) continue;
      a.health = health;
      if (health === "invalid") a.verified = false;
      changed = true;
    }
    if (changed) accounts();
  }
}
async function api(path, data) {
  const epoch = sessionEpoch;
  const response = await fetch(path, {
    method: data === undefined ? "GET" : "POST",
    credentials: "same-origin",
    headers:
      data === undefined
        ? {}
        : { "Content-Type": "application/json", "X-XTouch-CSRF": csrf },
    body: data === undefined ? undefined : JSON.stringify(data),
    signal: AbortSignal.timeout(20000),
  });
  let result;
  try {
    result = await response.json();
  } catch {
    throw Error(
      t(
        "设备返回异常，请刷新重试。",
        "Invalid device response. Reload to retry.",
      ),
    );
  }
  if (epoch != sessionEpoch)
    throw new DOMException("Obsolete session", "AbortError");
  if (!response.ok) {
    if (response.status === 401) enterLogin();
    const error = Error(explain(result.error || response.status));
    error.status = response.status;
    error.code = result.error;
    throw error;
  }
  return result;
}
function showChange() {
  sessionEpoch++;
  cloudBusy = false;
  $("language").disabled = false;
  collapseAccounts();
  clearInterval(poll);
  message("");
  $("guestFeedback").append($("status"));
  $("app").hidden = true;
  $("login").hidden = true;
  $("change").hidden = false;
}
$("loginForm").onsubmit = async (e) => {
  e.preventDefault();
  try {
    let result = await api("/api/login", {
      password: $("loginPassword").value,
    });
    $("loginPassword").value = "";
    csrf = result.csrf;
    await saveLanguageChoice();
    if (result.mustChange) showChange();
    else await load();
  } catch (error) {
    message(failure(error), true);
  }
};
$("passwordForm").onsubmit = async (e) => {
  e.preventDefault();
  if ($("newPassword").value !== $("confirmPassword").value) {
    message(t("两次密码不一致。", "Passwords do not match."));
    return;
  }
  try {
    await api("/api/password", { password: $("newPassword").value });
    $("newPassword").value = "";
    $("confirmPassword").value = "";
    enterLogin();
    message(t("密码已修改，请重新登录。", "Password changed. Sign in again."));
  } catch (error) {
    message(failure(error), true);
  }
};
$("passwordButton").onclick = showChange;
$("logout").onclick = async () => {
  try {
    await api("/api/logout", {});
    enterLogin();
  } catch (error) {
    message(failure(error), true);
  }
};
for (const button of document.querySelectorAll("[data-tab]"))
  button.onclick = () => {
    collapseAccounts();
    for (const tab of document.querySelectorAll(".tab"))
      tab.hidden = tab.id !== button.dataset.tab;
    for (const b of document.querySelectorAll("[data-tab]"))
      { b.classList.toggle("active", b === button); b.setAttribute("aria-pressed", String(b === button)); }
    if (cfg) accounts();
  };
function groupParentheticalNotes(root) {
  if (!root.isConnected) return;
  const nodes = [];
  if (root.nodeType === Node.TEXT_NODE) nodes.push(root);
  else { const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT); while (walker.nextNode()) nodes.push(walker.currentNode); }
  for (const node of nodes) {
    if (node.parentElement?.closest(".inlineNote,script,style,textarea")) continue;
    const parts = node.textContent.split(/([（(][^（）()]*[）)])/g);
    if (parts.length === 1) continue;
    const fragment = document.createDocumentFragment();
    for (const part of parts) {
      if (/^[（(]/.test(part)) {
        const note = document.createElement("span"); note.className = "inlineNote"; note.textContent = part; fragment.append(note);
      } else fragment.append(document.createTextNode(part));
    }
    node.replaceWith(fragment);
  }
}
const noteObserver = new MutationObserver(records => {
  for (const record of records) {
    if (record.type === "characterData") groupParentheticalNotes(record.target);
    else for (const node of record.addedNodes) groupParentheticalNotes(node);
  }
});
groupParentheticalNotes(document.body);
noteObserver.observe(document.body, {childList:true,subtree:true,characterData:true});
// Reserve the first row for the main caption and its option; only hints move.
function naturalFieldWidth(element) {
  const previous = element.getAttribute("style");
  Object.assign(element.style, {width:"max-content",maxWidth:"none",whiteSpace:"nowrap"});
  const width = element.getBoundingClientRect().width;
  if(previous === null)element.removeAttribute("style");else element.setAttribute("style",previous);
  return width;
}
const fieldHeadingObserver = new ResizeObserver(entries => {
  for(const {target:box} of entries) {
    if(!box.isConnected){fieldHeadingObserver.unobserve(box);continue;}
    const caption=box.querySelector(":scope > span:not(.inlineNote)"), option=box.querySelector(":scope > .checkbox"), input=box.querySelector(":scope > input");
    if(!caption || !option || !input)continue;
    const note=caption.querySelector(":scope > .inlineNote") || box.querySelector(":scope > .inlineNote");
    if(note)note.remove();
    box.style.setProperty("--field-title-width", Math.ceil(naturalFieldWidth(caption))+"px");
    if(note){
      caption.append(note);
      const needed=naturalFieldWidth(caption)+naturalFieldWidth(option)+parseFloat(getComputedStyle(box).columnGap);
      if(needed>box.clientWidth+.5)box.insertBefore(note,input);
    }
  }
});
const fieldBaselines = new WeakMap();
const controlValue = input => input.type === "checkbox" ? input.checked : input.value;
function trackField(input, saved) {
  fieldBaselines.set(input, typeof saved === "function" ? saved : () => saved);
  input.classList.toggle("unsaved", fieldDirty(input));
  return input;
}
function fieldDirty(input) {
  const baseline = fieldBaselines.get(input);
  return !!baseline && String(controlValue(input)) !== String(baseline() ?? "");
}
function refreshUnsaved() {
  for (const input of document.querySelectorAll("#app input,#app select")) {
    if (!fieldBaselines.has(input)) trackField(input, controlValue(input));
    input.classList.toggle("unsaved", fieldDirty(input));
  }
}
function accountBaseline(a, key) {
  return savedSnapshot?.cloudAccounts?.find(old => old.id === a.id)?.[key];
}
function field(parent, label, value, change, type = "text", max = 100) {
  const wrap = document.createElement("label"),
    caption = document.createElement("span"),
    input = document.createElement("input");
  caption.textContent = label;
  input.type = type;
  input.value = value ?? "";
  input.maxLength = max;
  input.oninput = () => change(input.value);
  wrap.append(caption, input);
  parent.append(wrap);
  trackField(input, input.value);
  return input;
}
function button(parent, label, click, danger = false) {
  let b = document.createElement("button");
  b.type = "button";
  b.textContent = label;
  if (["↑", "↓", "×"].includes(label)) b.setAttribute("aria-label", label === "↑" ? t("上移设备", "Move device up") : label === "↓" ? t("下移设备", "Move device down") : t("删除设备", "Remove device"));
  b.onclick = click;
  if (danger) b.className = "danger";
  parent.append(b);
  return b;
}
function select(parent, label, options, value, change) {
  let wrap = document.createElement("label"),
    caption = document.createElement("span"),
    input = document.createElement("select");
  caption.textContent = label;
  for (const [key, name] of options) {
    let o = new Option(name, key);
    input.append(o);
  }
  input.value = value;
  input.onchange = () => change(input.value);
  wrap.append(caption, input);
  parent.append(wrap);
  trackField(input, input.value);
  return input;
}
function accountKind(a) {
  return (a?.loginMethod || a?.authMode) === "password" ? (a.region === "global" ? "emailSource" : "phoneSource") : "tokenSource";
}
function source(p) {
  const a = cfg.cloudAccounts.find((a) => a.id === p.accountId);
  if(a?.simulator) return {label:t("模拟器", "Simulator"),kind:"tokenSource"};
  return p.mode !== "cloud"
    ? { label: "LAN", kind: "lan" }
    : (a?.loginMethod || a?.authMode) === "password"
      ? { label: a.region === "global" ? t("邮箱登录", "Email sign-in") : t("手机号登录", "Phone sign-in"), kind: accountKind(a) }
      : { label: "Cloud Token", kind: "tokenSource" };
}
function openAccount(a) {
  document.querySelector('[data-tab="cloudAccounts"]').click();
  a.expanded = true;
  accounts();
  if ((a.token || a.tokenSaved) && !a.devices && !cloudBusy)
    cloud(a, { action: "devices" });
}
const modelNames = {
  "BL-P001": "X1C",
  "BL-P002": "X1",
  C11: "P1P",
  C12: "P1S",
  C13: "X1E",
  N1: "A1 mini",
  N2S: "A1",
  N6: "X2D",
  N7: "P2S",
  N9: "A2L",
  O1C: "H2C",
  O1C2: "H2C",
  O1D: "H2D",
  O1E: "H2D Pro",
  O1S: "H2S",
};
function model(p) {
  const raw = p.printerModel || "";
  return modelNames[raw] || raw || "—";
}
function icon(kind) {
  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("viewBox", "0 0 24 24");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("sourceIcon", kind);
  const paths = {
    lan: [
      "M3 8a15 15 0 0 1 18 0M6 12a10 10 0 0 1 12 0M9 16a5 5 0 0 1 6 0",
      "M12 20h.01",
    ],
    tokenSource: [
      "M7 17a5 5 0 1 1 0-10 6 6 0 0 1 11 1 4.5 4.5 0 0 1-1 9",
      "M13 13a2 2 0 1 0 0 4 2 2 0 0 0 0-4m2 2h6m-2 0v3",
    ],
    phoneSource: ["M17,19H7V5H17M17,1H7C5.89,1 5,1.89 5,3V21A2,2 0 0,0 7,23H17A2,2 0 0,0 19,21V3C19,1.89 18.1,1 17,1Z"],
    emailSource: ["M22 6C22 4.9 21.1 4 20 4H4C2.9 4 2 4.9 2 6V18C2 19.1 2.9 20 4 20H20C21.1 20 22 19.1 22 18V6M20 6L12 11L4 6H20M20 18H4V8L12 13L20 8V18Z"],
  };
  for (const d of paths[kind]) {
    const path = document.createElementNS(svg.namespaceURI, "path");
    path.setAttribute("d", d);
    path.setAttribute("fill", kind === "phoneSource" || kind === "emailSource" ? "currentColor" : "none");
    path.setAttribute("stroke", kind === "phoneSource" || kind === "emailSource" ? "none" : "currentColor");
    path.setAttribute("stroke-width", "2");
    path.setAttribute("stroke-linecap", "round");
    path.setAttribute("stroke-linejoin", "round");
    svg.append(path);
  }
  return svg;
}
function deviceLabel(p) {
  return `[${model(p)}] ${p.name || p.serialNumber || t("新设备", "New device")}`;
}
function profiles() {
  const root = $("profiles");
  root.replaceChildren();
  cfg.printers.forEach((p, i) => {
    const row = document.createElement("div");
    row.className = "profile" + (i === selected ? " selected" : "");
    let pick = button(row, deviceLabel(p), () => {
      selected = i;
      profiles();
      const a = cfg.cloudAccounts.find((a) => a.id === p.accountId);
      if (p.mode === "cloud" && (a?.health === "invalid" || p.health < 0)) {
        openAccount(a);
        message(
          t("账号已失效，请重新登录。", "Account expired. Sign in again."),
          true,
        );
      }
    });
    pick.className = "pick " + source(p).kind;
    pick.prepend(icon(source(p).kind));
    pick.title = source(p).label;
    button(row, "↑", () => move(i, -1)).disabled = i === 0;
    button(row, "↓", () => move(i, 1)).disabled = i === cfg.printers.length - 1;
    button(
      row,
      "×",
      () => {
        const active = cfg.printers[cfg.activePrinter];
        cfg.printers.splice(i, 1);
        cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
        selected = Math.min(selected, Math.max(0, cfg.printers.length - 1));
        profiles();
      },
      true,
    );
    root.append(row);
  });
  $("defaultDevice").replaceChildren(
    ...cfg.printers.map(
      (p, i) => new Option(p.name || p.serialNumber || `${i + 1}`, i),
    ),
  );
  $("defaultDevice").value = cfg.activePrinter || 0;
  editor();
}
function move(i, delta) {
  const active = cfg.printers[cfg.activePrinter],
    current = cfg.printers[selected];
  [cfg.printers[i], cfg.printers[i + delta]] = [
    cfg.printers[i + delta],
    cfg.printers[i],
  ];
  cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
  selected = Math.max(0, cfg.printers.indexOf(current));
  profiles();
}
const newProfiles = new WeakSet();
const lanOrigins = new WeakMap();
const lanVerified = new WeakMap();
const lanFingerprint = p => JSON.stringify([p.printerModel || "", p.serialNumber || "", p.host || "", p.accessCode || "", !!p.accessCodeEncrypted, p.accessCodeEnc || ""]);
function lanOrigin(p) {
  if (!lanOrigins.has(p) && !newProfiles.has(p)) {
    const saved = savedSnapshot?.printers?.find(old => old.mode === p.mode && old.serialNumber === p.serialNumber);
    if (saved) lanOrigins.set(p, {fingerprint:lanFingerprint(p), saved});
  }
  return lanOrigins.get(p);
}
function lanPending(p) {
  const origin = lanOrigin(p);
  return p.mode === "lan" && (!origin || origin.fingerprint !== lanFingerprint(p));
}
async function validateLan(p, grid) {
  if (cloudBusy || saving) return;
  for (const input of grid.querySelectorAll("input,select"))
    if (!input.reportValidity()) return;
  const epoch = sessionEpoch, fingerprint = lanFingerprint(p), origin = lanOrigin(p);
  const current = () => epoch === sessionEpoch && cfg?.printers.includes(p) && fingerprint === lanFingerprint(p);
  cloudBusy = true;
  clearTimeout(saveTimer);
  const controls = [...document.querySelectorAll("#devices button,#editor input,#editor select,#addPrinter,#save,#language")];
  const disabled = controls.map(el => el.disabled);
  controls.forEach(el => el.disabled = true);
  try {
    message(t("正在验证设备连接…", "Verifying device connection…"));
    await api("/api/printer/validate", {name:p.name || p.serialNumber, printerModel:p.printerModel,
      serialNumber:p.serialNumber, host:p.host, accessCode:p.accessCode || "",
      credentialSource:origin?.saved ? {serialNumber:origin.saved.serialNumber,host:origin.saved.host} : undefined});
    const deadline = Date.now() + 45000;
    let verified = false;
    while (Date.now() < deadline) {
      await new Promise(resolve => setTimeout(resolve, 750));
      if (!current()) return;
      const result = await api("/api/printer/validate");
      if (result.busy) continue;
      if (!result.ok) throw new Error(explain(result.error));
      verified = true;
      break;
    }
    if (!verified) throw new Error(t("请求超时，请重试。", "Request timed out. Retry."));
    lanVerified.set(p, fingerprint);
    cloudBusy = false;
    if (await saveConfiguration(true)) { profiles(); message(t("设备已验证并保存。", "Device verified and saved.")); }
  } catch (error) {
    if (current()) message(failure(error), true);
  } finally {
    cloudBusy = false;
    controls.forEach((el, i) => el.disabled = disabled[i]);
  }
}
const validName = (value) => /^[\x20-\x7E]*$/.test(value || "");
function nameField(parent, label, value, change) {
  const input = field(parent, label, value, change, "text", 63);
  input.pattern = "[ -~]*";
  input.title = t(
    "仅限英文字母、数字和英文符号",
    "English letters, numbers and ASCII symbols only",
  );
  input.addEventListener("input", () =>
    input.setCustomValidity(
      validName(input.value)
        ? ""
        : t(
            "仅限英文字母、数字和英文符号",
            "English letters, numbers and ASCII symbols only",
          ),
    ),
  );
  return input;
}
function readOnlyField(parent, label, value, type = "text", maxLength = 100) {
  const input = field(parent, label, value, () => {}, type, maxLength);
  input.readOnly = true;
  input.setAttribute("aria-readonly", "true");
  input.classList.add("readOnlyValue");
  return input;
}
function lockEditor(input, onUnlock) {
  if (input.classList.contains("readOnlyValue")) return;
  const wrap = input.parentElement;
  wrap.classList.add("editLocked");
  input.title = t(
    "双击修改；键盘按 F2 或 Enter",
    "Double-click to edit; F2 or Enter with keyboard",
  );
  let committed = input.value;
  const changed = input.onchange;
  input.onchange = () => {
    if (input.dataset.locked === "1") {
      input.value = committed;
      return;
    }
    if (changed) changed();
    committed = input.value;
  };
  const lock = () => {
    if (fieldDirty(input)) { unlock(false); return; }
    wrap.classList.add("editLocked");
    input.setAttribute("aria-readonly", "true");
    input.dataset.locked = "1";
    if (input.tagName === "INPUT") input.readOnly = true;
    input.dispatchEvent(new Event("locked"));
  };
  const unlock = (focus = true) => {
    wrap.classList.remove("editLocked");
    input.setAttribute("aria-readonly", "false");
    input.dataset.locked = "0";
    if (input.tagName === "INPUT") input.readOnly = false;
    if(focus)input.focus();
    if (onUnlock) onUnlock();
  };
  lock();
  input.addEventListener("dblclick", unlock);
  input.addEventListener("keydown", (e) => {
    if (input.dataset.locked === "1") {
      if (e.key === "F2" || e.key === "Enter") {
        e.preventDefault();
        unlock();
      } else if (input.tagName === "SELECT" && e.key !== "Tab")
        e.preventDefault();
    }
  });
  input.addEventListener("pointerdown", (e) => {
    if (input.tagName === "SELECT" && input.dataset.locked === "1")
      e.preventDefault();
  });
  input.addEventListener("blur", lock);
  input.addEventListener("saved", lock);
}
function editor() {
  const root = $("editor");
  root.replaceChildren();
  const p = cfg.printers[selected];
  if (!p) return;
  if (p.mode === "lan") lanOrigin(p);
  const grid = document.createElement("div");
  grid.className = "grid";
  root.append(grid);
  let accessInput, accessStorage;
  const title = document.createElement("h3");
  title.textContent = t("设备信息", "Device information");
  grid.append(title);
  nameField(grid, t("设备名称", "Device name"), p.name, (v) => (p.name = v));
  const hint = document.createElement("small");
  hint.className = "editHint";
  hint.textContent = t(
    "已有设备双击编辑，或聚焦后按 F2；名称仅限英文、数字和符号。",
    "Double-click existing fields or press F2. Names: ASCII letters, numbers and symbols.",
  );
  grid.insertBefore(hint, title.nextSibling);
  const show = (label, value) => {
    const row = document.createElement("div");
    row.className = "deviceDetail";
    const caption = document.createElement("small"),
      text = document.createElement("div");
    caption.textContent = label;
    text.textContent = value || "—";
    row.append(caption, text);
    grid.append(row);
  };
  if (p.mode === "cloud") {
    show(t("设备型号", "Device model"), model(p));
    show(t("序列号", "Serial number"), p.serialNumber);
    const account = cfg.cloudAccounts.find((a) => a.id === p.accountId);
    const row = document.createElement("div");
    row.className = "deviceDetail";
    const caption = document.createElement("small");
    caption.textContent = t("连接方式", "Connection type");
    const link = button(
      row,
      source(p).label +
        " (" +
        (account?.name || account?.account || account?.id || "—") +
        ")",
      () => {
        if (account) openAccount(account);
      },
    );
    link.className = "accountLink readOnlyValue " + source(p).kind;
    row.prepend(caption);
    grid.append(row);
    if (info?.capabilities?.camera) {
      const candidates = cfg.printers.filter(x => x.mode === "lan" && x.serialNumber === p.serialNumber && /^(10\.|192\.168\.|172\.(1[6-9]|2[0-9]|3[01])\.)/.test(x.host || "") && x.host.split(".").length === 4 && x.host.split(".").every(n => /^\d+$/.test(n) && Number(n) <= 255) && !newProfiles.has(x) && !lanPending(x));
      const options = [["", t("自动关联同一设备", "Automatically link the same device")], ...candidates.map(x => [x.host, `${x.name} (${x.host})`])];
      if (p.cameraLanHost && !options.some(([host]) => host === p.cameraLanHost)) options.push([p.cameraLanHost, `${p.cameraLanHost} (${t("不可用", "Unavailable")})`]);
      const camera = select(grid, t("摄像头局域网设备", "LAN device for camera"), options, p.cameraLanHost || "", value => {p.cameraLanHost = value;});
      camera.setAttribute("aria-label", t("摄像头局域网设备", "LAN device for camera"));
      trackField(camera, () => savedSnapshot?.printers?.find(x => x.mode === "cloud" && x.accountId === p.accountId && x.serialNumber === p.serialNumber)?.cameraLanHost || "");
      const hint = document.createElement("small");
      hint.id = "cameraLinkHint";
      camera.setAttribute("aria-describedby", hint.id);
      hint.textContent = t("默认自动关联；多个本地连接冲突时请选择。此关联仅用于摄像头。", "Linked automatically by default. Select a device if local connections conflict. This link is only for the camera.");
      camera.parentElement.append(hint);
    }
  } else {
    select(
      grid,
      t("设备型号", "Device model"),
      [["", t("请选择型号", "Select model")], ...Object.entries(modelNames)],
      p.printerModel || "",
      (v) => (p.printerModel = v),
    );
    field(
      grid,
      t("序列号", "Serial number"),
      p.serialNumber,
      (v) => (p.serialNumber = v),
      "text",
      31,
    );
    field(
      grid,
      t("IP / 主机名", "IP / hostname"),
      p.host,
      (v) => (p.host = v),
      "text",
      63,
    );
    const input = field(
      grid,
      t("访问码（留空保留）", "Access code (blank to keep)"),
      p.accessCode,
      (v) => (p.accessCode = v),
      "password",
      8,
    );
    input.pattern = "[!-~]{8}";
    input.minLength = 8;
    input.placeholder = p.accessCodeSaved ? "••••••••" : "";
    input.setAttribute("aria-label",t("访问码（留空保留）", "Access code (blank to keep)"));
    accessInput = input;
    accessStorage = encryptedStorage(input, p, "accessCodeEncrypted", true);
    readOnlyField(grid, t("连接方式", "Connection type"), t("局域网模式", "LAN mode"));
  }
  const connectionInputs = [...grid.querySelectorAll('input:not([type="checkbox"]),select')].slice(1).filter(input => !input.readOnly);
  let validate;
  if (p.mode === "lan") {
    for (const input of connectionInputs) input.required = input.type !== "password" || !p.accessCodeSaved;
    const serial = connectionInputs[1], host = connectionInputs[2];
    serial.pattern = String.raw`[A-Za-z0-9_\-]+`;
    host.pattern = String.raw`[A-Za-z0-9._\-]+`;
    const saved = () => savedSnapshot?.printers?.find(old => old.mode === "lan" && old.serialNumber === lanOrigin(p)?.saved.serialNumber) || lanOrigin(p)?.saved;
    const fields = [...grid.querySelectorAll('input:not([type="checkbox"]),select')].filter(input => !input.classList.contains("readOnlyValue"));
    for (const [index,key] of ["name","printerModel","serialNumber","host","accessCode"].entries())
      trackField(fields[index], () => saved()?.[key] || "");
    trackField(accessStorage.querySelector("input"), () => !!saved()?.accessCodeEncrypted);
    validate = button(grid, t("验证并保存设备", "Verify & save device"), () => validateLan(p, grid));
    validate.classList.add("primary", "fullWidth");
    const edited = () => {
      const pending = lanPending(p);
      validate.hidden = !pending && !connectionInputs.some(input => input.dataset.locked === "0");
      accessInput.required = !p.accessCodeSaved;
      const encrypt = accessStorage.querySelector("input");
      encrypt.disabled = !accessInput.value;
      if(encrypt.disabled)encrypt.dataset.cloudLocked = "true";else delete encrypt.dataset.cloudLocked;
      // An unchanged saved access code also works when model, serial or host changes.
      validate.disabled = !accessInput.value && !(pending && p.accessCodeSaved);
      if(validate.disabled)validate.dataset.cloudLocked = "true";else delete validate.dataset.cloudLocked;
      refreshUnsaved();
    };
    for (const input of [...connectionInputs,accessStorage.querySelector("input")])
      for (const event of ["input","change","locked"]) input.addEventListener(event, edited);
    if (!newProfiles.has(p)) for (const input of fields) lockEditor(input, connectionInputs.includes(input) ? edited : undefined);
    edited();
  } else if (!newProfiles.has(p)) {
    for (const input of grid.querySelectorAll('input:not([type="checkbox"]),select'))
      if (!input.readOnly) lockEditor(input);
  }

}
$("addPrinter").onclick = () => {
  if (cfg.printers.length >= 100)
    return message(t("最多 100 台。", "Maximum 100 devices."));
  cfg.printers.push({
    mode: "lan",
    name: "",
    host: "",
    accessCode: "",
    serialNumber: "",
    printerModel: "",
  });
  selected = cfg.printers.length - 1;
  newProfiles.add(cfg.printers[selected]);
  profiles();
};
async function discoverLab(a){
  if(cloudBusy)return;
  cloudBusy=true;
  const epoch=sessionEpoch;
  try{
    const result=await api("/api/lab/discover",{host:a.mqttHost,port:Number(a.labHttpPort||8894)});
    if(epoch!==sessionEpoch||!cfg)return;
    a.devices=result.devices;a.token="xtouch-lab";a.userId="xtouch-lab";a.signedIn=true;a.verified=true;a.expanded=true;
    message(t("已获取虚拟设备，勾选后自动保存。", "Virtual devices found. Selected devices save automatically."));
  }catch(e){if(epoch===sessionEpoch)message(failure(e),true);}
  finally{cloudBusy=false;if(epoch===sessionEpoch&&cfg)accounts();}
}
function labAccount(body,a){
  const note=document.createElement("p");note.className="subtle";note.textContent=t("仅连接局域网模拟器，使用固定测试凭据，不发送拓竹 Token 或密码。", "Local simulator only. Fixed test credentials; no Bambu tokens or passwords are sent.");body.append(note);
  const grid=document.createElement("div");grid.className="grid";body.append(grid);
  nameField(grid,t("备注", "Label"),a.name||a.id,v=>a.name=v);
  field(grid,t("模拟器 IP", "Simulator IP"),a.mqttHost,v=>a.mqttHost=v,"text",63);
  field(grid,"TLS MQTT port",a.mqttPort||8883,v=>a.mqttPort=Number(v),"number",5);
  field(grid,"Web port",a.labHttpPort||8894,v=>a.labHttpPort=Number(v),"number",5);
  button(body,t("获取虚拟设备", "Discover virtual devices"),()=>discoverLab(a));
  for(const p of a.devices||cfg.printers.filter(p=>p.accountId===a.id)){
    const row=document.createElement("label");row.className="checkbox deviceRow tokenSource";
    const check=document.createElement("input");check.type="checkbox";check.checked=cfg.printers.some(x=>x.accountId===a.id&&x.serialNumber===p.serialNumber);
    check.onchange=()=>{
      if(check.checked){if(cfg.printers.length>=100){check.checked=false;return message(t("最多 100 台设备。", "Maximum 100 devices."));}
        cfg.printers.push({name:p.name,serialNumber:p.serialNumber,printerModel:p.printerModel,mode:"cloud",accountId:a.id});
      }else{const active=cfg.printers[cfg.activePrinter||0];cfg.printers=cfg.printers.filter(x=>!(x.accountId===a.id&&x.serialNumber===p.serialNumber));cfg.activePrinter=Math.max(0,cfg.printers.indexOf(active));}
      profiles();editor();
    };
    row.append(check,icon("tokenSource"),document.createTextNode(deviceLabel(p)));body.append(row);
  }
}
function accountAuthenticated(a) {
  return !a.loginCleared && !a.credentialLocked && !a.passwordLocked && a.health !== "invalid" &&
    !!(a.token || a.tokenSaved) && !!(a.verified || a.tokenSaved);
}
function updateAccountBadge(a, badge) {
  const invalid = a.loginCleared || a.credentialLocked || a.passwordLocked || a.health === "invalid";
  const valid = !invalid && accountAuthenticated(a) && (a.verified || a.health === "valid");
  badge.className = "badge" + (valid ? " ok" : "");
  badge.textContent = invalid ? t("需登录", "Sign-in required") : valid ? t("有效", "Valid") :
    a.token || a.tokenSaved ? t("待检查", "Unchecked") : t("未登录", "Signed out");
}
function encryptedStorage(input, account, key, disabled = false) {
  account[key] ??= false;
  const keep = document.createElement("label"); keep.className = "checkbox";
  const check = document.createElement("input"); check.type = "checkbox";
  check.checked = account[key]; check.disabled = disabled;
  if(key !== "accessCodeEncrypted")trackField(check, () => !!accountBaseline(account,key));
  if (disabled) check.dataset.cloudLocked = "true";
  check.onchange = () => { account[key] = check.checked; account.verified = false; };
  keep.append(check, document.createTextNode(t("加密并绑定", "Encrypt & bind")));
  const help = t("勾选后加密保存，并绑定当前设备密钥；重置或更换设备后需重新填写。未勾选则明文保存。", "Encrypts this credential and binds it to this device key. Re-enter it after a reset or on another device. Unchecked: stored as plaintext.");
  keep.title = help;
  check.title = help;
  check.setAttribute("aria-description", help);
  const wrap = input.parentElement, box = document.createElement("div");
  box.className = "passwordField"; wrap.replaceWith(box); box.append(...wrap.childNodes);
  box.insertBefore(keep, input);
  fieldHeadingObserver.observe(box);
  return keep;
}
function accountPending(a) {
  const saved = savedSnapshot?.cloudAccounts?.find(old => old.id === a.id);
  if (!saved) return true;
  return ["region", "account", "token", "tokenEnc", "password", "passwordEnc"].some(key =>
    (a[key] || (key === "region" ? "cn" : "")) !== (saved[key] || (key === "region" ? "cn" : ""))) ||
    ["tokenEncrypted", "passwordEncrypted", "rememberPassword"].some(key => !!a[key] !== !!saved[key]);
}
function credentialControls(input, storage, validate, locked = false) {
  const disabled = locked || !input.value.length;
  for(const control of [storage.querySelector("input"), validate]) {
    control.disabled = disabled;
    if(disabled)control.dataset.cloudLocked = "true";
    else delete control.dataset.cloudLocked;
  }
}
function tokenFields(body, a, locked = false) {
  const pending = accountPending(a) && !a.verified;
  const storedHealth = a.health;
  const previous = () => savedSnapshot?.cloudAccounts?.find(old => old.id === a.id);
  const fields = document.createElement("div");
  fields.className = "grid formStack";
  body.append(fields);
  const token = field(fields, "Access Token", a.token || "", (value) => {
    a.token = value;
    const stored = savedSnapshot?.cloudAccounts?.find(old => old.id === a.id &&
      (old.region || "cn") === (a.region || "cn") && (old.account || "") === (a.account || ""));
    a.tokenSaved = !value && !!stored && !stored.loginCleared && !!(stored.token || stored.tokenEnc || stored.tokenSaved);
    a.clearToken = false;
    a.verified = false;
    a.health = !value && a.tokenSaved ? storedHealth : "unknown";
    const badge = body.closest?.(".accountCard")?.querySelector(".badge");
    if (badge) updateAccountBadge(a, badge);
    validate.hidden = false;
    controls();
  }, "password", 2048);
  trackField(token, () => accountBaseline(a,"token") || "");
  token.setAttribute("aria-label", "Access Token");
  token.required = !a.tokenSaved;
  const guide = token.parentElement.querySelector("span");
  const helpGroup = document.createElement("span"); helpGroup.className = "inlineNote";
  helpGroup.append("(", t("获取教程", "Guide"), ": ");
  guide.append(" ", helpGroup);
  for (const [code, file] of [["CN", "INSTALLATION.md"], ["EN", "INSTALLATION.en.md"]]) {
    if (code === "EN") helpGroup.append(" / ");
    const link = document.createElement("a");
    link.href = `https://github.com/tbc0309/xTouch-CN/blob/main/docs/${file}#token`;
    link.target = "_blank";
    link.rel = "noopener noreferrer";
    link.className = "tokenHelp";
    link.textContent = code;
    helpGroup.append(link);
  }
  helpGroup.append(")");
  token.placeholder = a.tokenSaved ? t("已保存；留空保留", "Saved; blank to keep") : "";
  const storage = encryptedStorage(token, a, "tokenEncrypted", locked && !pending);
  const validate = button(fields, t("验证并获取设备", "Validate & get devices"), () => token.reportValidity() && cloud(a, {action:"validate", credentialSource: previous() ? {region:previous().region || "cn",account:previous().account || ""} : undefined}));
  validate.classList.add("formSubmit", "primary");
  const controls = () => {
    credentialControls(token, storage, validate, locked && !accountPending(a) && token.dataset.locked !== "0");
    const regionChanged = previous() && (previous().region || "cn") !== (a.region || "cn");
    if (regionChanged && a.tokenSaved && !a.credentialLocked) {
      validate.disabled = false; delete validate.dataset.cloudLocked;
    }
    refreshUnsaved();
  };
  validate.hidden = locked && !pending;
  trackField(storage.querySelector("input"), () => !!accountBaseline(a,"tokenEncrypted"));
  if (locked) lockEditor(token, () => { validate.hidden = false; controls(); });
  for (const event of ["saved","locked"]) token.addEventListener(event, () => {
    if (!accountPending(a)) { validate.hidden = true; controls(); }
  });
  controls();
  return validate;
}
function accounts() {
  const root = $("accounts");
  root.replaceChildren();
  $("addSimulator")?.remove();
  if(labTools) {
    const addSimulator = button($("addAccount").parentElement,t("添加 MQTT 模拟器", "Add MQTT simulator"),()=>{
    if(cfg.cloudAccounts.length>=50)return message(t("最多 50 个账号。", "Maximum 50 accounts."));
    let n=1;while(cfg.cloudAccounts.some(a=>a.id===`Lab-${n}`))n++;
    cfg.cloudAccounts.push({id:`Lab-${n}`,name:`Lab-${n}`,simulator:true,mqttHost:"",mqttPort:8883,labHttpPort:8894,region:"cn",token:"xtouch-lab",userId:"xtouch-lab",signedIn:false,expanded:true});
    accounts();
    });
    addSimulator.id = "addSimulator";
    $("addAccount").before(addSimulator);
    addSimulator.disabled = cloudBusy;
  }
  cfg.cloudAccounts.forEach((a, i) => {
    const open = a.expanded === true,
      kind = accountKind(a);
    const card = document.createElement("article");
    card.className = "accountCard";
    root.append(card);
    const head = document.createElement("div");
    head.className = "accountHeader";
    card.append(head);
    const passwordState =
      a.simulator ? t("模拟器", "Simulator") : kind === "tokenSource"
        ? "Access Token"
        : a.passwordLocked
          ? t("重输密码", "Re-enter password")
          : a.passwordSaved
            ? t("密码已存", "Password saved")
            : t("密码未存", "Password not saved");
    const toggle = button(
      head,
      `${a.name || a.account || a.id}（${passwordState}）`,
      () => {
        a.expanded = !open;
        accounts();
        if (a.expanded && (a.token || a.tokenSaved) && !a.devices && !cloudBusy)
          cloud(a, { action: "devices" });
      },
    );
    const accountName = document.createElement("span");
    accountName.className = "accountName";
    accountName.textContent = toggle.textContent;
    toggle.title = toggle.textContent;
    toggle.replaceChildren(icon(kind), accountName);
    const chevron = document.createElement("span");
    chevron.className = "accountChevron";
    chevron.setAttribute("aria-hidden", "true");
    chevron.innerHTML = '<svg viewBox="0 0 24 24" width="22" height="22" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m6 9 6 6 6-6"/></svg>';
    toggle.append(chevron);
    toggle.className = "accountToggle " + kind;
    card.classList.toggle("isOpen", open);
    toggle.setAttribute("aria-expanded", String(open));
    const badge = document.createElement("span");
    updateAccountBadge(a, badge);
    head.append(badge);
    const tools = document.createElement("div");
    tools.className = "accountTools";
    head.append(tools);
    if (!a.simulator) button(
      tools,
      t("清除登录", "Clear login"),
      () => {
        a.token = "";
        a.tokenSaved = false;
        a.userId = "";
        a.clearToken = true;
        a.signedIn = false;
        a.needsCode = false;
        delete a.tfaKey;
        codeAttention.delete(a);
        pendingPasswords.delete(a);
        a.verified = false;
        a.health = "invalid";
        a.loginCleared = true;
        a.authMode = a.loginMethod === "password" ? "password" : "token";
        a.passwordSaved = false;
        a.passwordLocked = false;
        a.credentialLocked = false;
        a.passwordEnc = "";
        a.tokenEnc = "";
        a.password = "";
        a.rememberPassword = false;
        a.devices = null;
        a.expanded = true;
        accounts();
        message(t("正在自动保存…", "Saving automatically…"));
      },
      true,
    );
    button(
      tools,
      t("删除账号", "Delete account"),
      () => {
        if (
          cfg.printers.some((p) => p.mode === "cloud" && p.accountId === a.id)
        )
          return message(
            t("请先移除关联设备。", "Remove linked devices first."),
            true,
          );
        cfg.cloudAccounts.splice(i, 1);
        accounts();
        editor();
      },
      true,
    );
    const body = document.createElement("div");
    body.className = "accountBody";
    body.id = "account-body-" + i;
    toggle.setAttribute("aria-controls", body.id);
    body.hidden = !open;
    card.append(body);
    if(a.simulator){labAccount(body,a);return;}
    const authenticated = accountAuthenticated(a);
    if (authenticated) {
      if (a.loginMethod === "token") {
        const identity = document.createElement("div");
        identity.className = "grid";
        body.append(identity);
        const name = nameField(identity, t("备注", "Label"), a.name ?? a.id, value => { a.name = value; });
        trackField(name, () => accountBaseline(a,"name") ?? a.id);
        lockEditor(name);
        const regionField = select(identity, t("区域", "Region"), [["cn",t("中国区","China")],["global",t("国际区","Global")]], a.region || "cn", value => {
          a.region = value; a.verified = false; a.health = "unknown"; a.devices = null; accounts();
        });
        trackField(regionField, () => accountBaseline(a,"region") || "cn");
        lockEditor(regionField);
        tokenFields(body, a, true);
      } else {
        const identity = document.createElement("div");
        identity.className = "grid";
        body.append(identity);
        const name = nameField(identity, t("备注", "Label"), a.name ?? a.id, value => { a.name = value; });
        trackField(name, () => accountBaseline(a,"name") ?? a.id);
        lockEditor(name);
        readOnlyField(identity, t("区域", "Region"), (a.region || "cn") === "cn" ? t("中国区", "China") : t("国际区", "Global"));
        readOnlyField(identity, (a.region || "cn") === "cn" ? t("手机号", "Phone number") : t("邮箱", "Email"), a.account || "", (a.region || "cn") === "cn" ? "tel" : "email");
      }
    } else {
    const identity = document.createElement("div");
    identity.className = "grid";
    body.append(identity);
    nameField(
      identity,
      t("备注", "Label"),
      a.name ?? (a.account || a.id),
      (v) => (a.name = v),
    );
    select(
      identity,
      t("区域", "Region"),
      [
        ["cn", t("中国区", "China")],
        ["global", t("国际区", "Global")],
      ],
      a.region || "cn",
      (v) => {
        a.region = v;
        pendingPasswords.delete(a);
        a.needsCode = false; delete a.tfaKey; a.signInMode = "password"; a.account = "";
        a.signedIn = false;
        a.verified = false;
        a.tokenSaved = false;
        a.token = "";
        a.passwordSaved = false;
        a.devices = null;
        accounts();
      },
    );
    const modes = document.createElement("div");
    modes.className = "methodTabs";
    modes.hidden = !passwordTools;
    body.append(modes);
    for (const [mode, label] of [
      ["token", "Access Token"],
      ["password", t("账号登录", "Account sign-in")],
    ]) {
      const b = button(modes, label, () => {
        a.authMode = mode;
        if (mode === "token" && a.passwordLocked) {
          a.rememberPassword = false;
          a.passwordEnc = "";
          a.passwordLocked = false;
        }
        a.expanded = true;
        accounts();
        profiles();
      });
      b.classList.toggle("active", (passwordTools ? a.authMode || "token" : "token") === mode);
      b.setAttribute("aria-pressed", String(b.classList.contains("active")));
    }
    const fields = document.createElement("div");
    fields.className = "grid";
    body.append(fields);
    if ((passwordTools || a.loginMethod === "password") && a.authMode === "password") {
      const china = (a.region || "cn") === "cn";
      const codeLogin = china && a.signInMode === "code";
      fields.classList.add("formStack");
      const identity = field(fields, china ? t("手机号", "Phone number") : t("邮箱", "Email"),
        a.account || "", (v) => {
          a.account = v; a.signedIn = false; a.passwordSaved = false; pendingPasswords.delete(a);
          a.devices = null; a.needsCode = false; delete a.tfaKey;
        }, china ? "tel" : "email", 100);
      identity.required = true;
      if (china) identity.pattern = "1[0-9]{10}";
      if (china && !a.needsCode) {
        const methods = document.createElement("div");
        methods.className = "methodTabs methodTabsInline";
        fields.append(methods);
        for (const [mode, label] of [["password", t("密码登录", "Password sign-in")], ["code", t("验证码登录", "Code sign-in")]]) {
          const tab = button(methods, label, () => {
            a.signInMode = mode; a.lastError = ""; accounts();
          });
          tab.classList.toggle("active", a.signInMode === mode);
          tab.setAttribute("aria-pressed", String(tab.classList.contains("active")));
        }
      }
      if (!china || a.signInMode === "password") {
        a.rememberPassword = true;
        let password = "";
        const passwordFields = document.createElement("div"); passwordFields.className = "grid formStack"; body.append(passwordFields);
        const pw = field(passwordFields, t("密码", "Password"), "", (v) => password = v, "password", 128);
        pw.required = !a.passwordSaved;
        pw.placeholder = a.passwordSaved ? t("已保存；留空使用", "Saved; leave blank to use") : "";
        pw.setAttribute("aria-label", t("密码", "Password"));
        if (a.needsCode) {
          pw.placeholder = t("密码已提交，请完成下方验证", "Password submitted; complete verification below");
          pw.disabled = true; pw.dataset.cloudLocked = "true";
        }
        encryptedStorage(pw, a, "passwordEncrypted", !!a.needsCode);
        const actions = passwordFields;
        if (!a.needsCode) {
        const confirm = button(actions, t("确认登录", "Confirm sign-in"), async () => {
          if (!identity.reportValidity() || !pw.reportValidity()) return;
          if (!a.account || (!password && !a.passwordSaved))
            return message(t("请填写账号和密码。", "Enter account and password."), true);
          await cloud(a, {action: "login", account: a.account, password, rememberPassword: true});
          password = ""; pw.value = "";
        });
        confirm.classList.add("formSubmit");
        }
      }
      if (codeLogin || a.needsCode) {
        const verify = document.createElement("div"); verify.className = "grid formStack"; body.append(verify);
        if (a.needsCode) {
          const note = document.createElement("p"); note.className = "hint";
          const masked = china ? (a.account || "").replace(/^(\d{3})\d+(\d{4})$/, "$1****$2") : (a.account || "").replace(/^(.).+(@.+)$/, "$1***$2");
          note.textContent = a.tfaKey ? t("请输入身份验证器验证码。", "Enter your authenticator code.") :
            t(`请填写 ${masked} 的验证码。`, `Enter the verification code for ${masked}.`);
          body.insertBefore(note, verify);
        }
        let code = "";
        const codeField = field(verify, a.tfaKey ? t("身份验证器验证码", "Authenticator code") : t("验证码", "Verification code"), "", v => code = v, "text", 6);
        codeField.setAttribute("aria-label", a.tfaKey ? t("身份验证器验证码", "Authenticator code") : t("验证码", "Verification code"));
        codeField.inputMode = "numeric"; codeField.autocomplete = "one-time-code";
        codeField.required = true; codeField.pattern = "[0-9]{6}";
        const attention = codeAttention.get(a);
        if (attention) {
          codeField.classList.add("codeAwaiting");
          codeField.classList.add("codePulse");
          codeField.addEventListener("input", () => {
            codeAttention.delete(a);
            codeField.classList.remove("codeAwaiting", "codePulse");
          }, {once:true});
        }
        if (!a.tfaKey && codeLogin) {
          const codeWrap = codeField.parentElement;
          const inputRow = document.createElement("div"); inputRow.className = "inputActionRow";
          codeWrap.append(inputRow); inputRow.append(codeField);
          const send = button(inputRow, t("获取验证码", "Get code"), () => {
            if (!identity.reportValidity()) return;
            if (!a.account) return message(t("请填写账号。", "Enter your account."), true);
            if ((codeResendUntil.get(codeResendKey(a)) || 0) > Date.now()) return;
            return cloud(a, {action: "send_code", account: a.account});
          });
          resendCountdown(send, a);
        }
        const confirm = button(verify, t("确认登录", "Confirm sign-in"), () => {
          if (!identity.reportValidity() || !codeField.reportValidity()) return;
          if (!/^\d{6}$/.test(code)) return message(t("请输入6位验证码。", "Enter the 6-digit code."), true);
          return cloud(a, {action: a.tfaKey ? "login_tfa" : "login_code", account: a.account, code, tfaKey: a.tfaKey || ""});
        });
        confirm.classList.add("formSubmit");
      }
    } else {
      fields.remove();
      tokenFields(body, a);
    }
    }
    if (a.token || a.tokenSaved || a.devices) {
      const section = document.createElement("div");
      section.className = "linkedDevices";
      body.append(section);
      const heading = document.createElement("div");
      heading.className = "sectionHead";
      section.append(heading);
      const title = document.createElement("h3");
      title.textContent = t("关联设备", "Linked devices");
      heading.append(title);
      button(heading, t("刷新设备", "Refresh devices"), () =>
        cloud(a, { action: "devices" }),
      );
      if (Array.isArray(a.devices) && !a.devices.length) {
        const empty = document.createElement("p");
        empty.className = "muted";
        empty.textContent = t("该账号暂无设备。", "No devices on this account.");
        section.append(empty);
      }
      for (const p of a.devices || []) {
        const label = document.createElement("label");
        label.className = "checkbox deviceRow " + kind;
        const check = document.createElement("input");
        check.type = "checkbox";
        check.checked = cfg.printers.some(
          (x) =>
            x.mode === "cloud" &&
            x.accountId === a.id &&
            x.serialNumber === p.serialNumber,
        );
        check.onchange = () => {
          if (check.checked) {
            if (cfg.printers.length >= 100) {
              check.checked = false;
              return message(t("最多 100 台。", "Maximum 100 devices."), true);
            }
            cfg.printers.push({
              name: validName(p.name) ? p.name : "",
              serialNumber: p.serialNumber,
              printerModel: p.printerModel || "",
              mode: "cloud",
              accountId: a.id,
            });
          } else {
            const active = cfg.printers[cfg.activePrinter];
            cfg.printers = cfg.printers.filter(
              (x) =>
                !(
                  x.mode === "cloud" &&
                  x.accountId === a.id &&
                  x.serialNumber === p.serialNumber
                ),
            );
            cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
          }
          profiles();
        };
        label.append(
          check,
          icon(kind),
          document.createTextNode(
            deviceLabel(p) +
              (p.online === false ? t(" · 离线", " · Offline") : ""),
          ),
        );
        section.append(label);
      }
    }
  });
  for (const [i,card] of [...root.querySelectorAll(".accountCard")].entries()) {
    const a = cfg.cloudAccounts[i], identity = card.querySelector(".accountBody > .grid");
    if(!identity)continue;
    const note = identity.querySelector("input");
    if(note)trackField(note, () => accountBaseline(a,"name") ?? a.id);
    const region = identity.querySelector("select");
    if(region)trackField(region, () => accountBaseline(a,"region") || "cn");
    const account = card.querySelector('input[type="tel"],input[type="email"]');
    if(account)trackField(account, () => accountBaseline(a,"account") || "");
  }
  refreshUnsaved();
  if (cloudBusy)
    for (const el of root.querySelectorAll("button,input,select"))
      el.disabled = true;
}
$("addAccount").onclick = () => {
  if (cfg.cloudAccounts.length >= 50)
    return message(t("最多 50 个账号。", "Maximum 50 accounts."));
  let n = cfg.cloudAccounts.length + 1;
  while (
    cfg.cloudAccounts.some(
      (a) => a.id === `Cloud-${n}` || a.name === `Cloud-${n}`,
    )
  )
    n++;
  cfg.cloudAccounts.push({
    id: `Cloud-${n}`,
    name: `Cloud-${n}`,
    region: "cn",
    token: "",
    tokenEncrypted: false,
    passwordEncrypted: false,
    rememberPassword: true,
    authMode: "token",
    loginMethod: "token",
    signInMode: "password",
    userId: "",
  });
  accounts();
  editor();
};
// Cloud results belong to the session that started them, not the next login.
async function cloud(a, input) {
  if(a.simulator)return discoverLab(a);
  if (cloudBusy || !cfg || $("app").hidden) return;
  if (input.action === "login") pendingPasswords.delete(a);
  const epoch = sessionEpoch;
  const current = () => epoch === sessionEpoch && !!cfg && !$("app").hidden;
  cloudBusy = true;
  for (const b of document.querySelectorAll(
    "#accounts button,#accounts input,#accounts select,#addAccount,#addSimulator,#save,#language",
  ))
    b.disabled = true;
  try {
    message(
      t("正在联系拓竹服务，请等待结果…", "Contacting Bambu. Please wait…"),
    );
    await api("/api/cloud", {
      region: a.region || "cn",
      token: a.token || "",
      accountId: a.id,
      ...input,
    });
    const until = Date.now() + 45000;
    while (Date.now() < until) {
      await new Promise((r) => setTimeout(r, 750));
      if (!current()) return;
      const result = await api("/api/cloud");
      if (result.busy) continue;
      if (result.passwordEnc) pendingPasswords.set(a, result.passwordEnc);
      if (
        result.error === "verification_required" ||
        result.error === "two_factor_required"
      ) {
        a.needsCode = true;
        a.tfaKey = result.tfaKey || "";
        codeAttention.set(a, Date.now());
        a.lastError = "";
        a.expanded = !$("cloudAccounts").hidden;
        accounts();
        message(t("请填写验证码以继续登录。", "Enter the verification code to continue."));
        return;
      }
      if (!result.ok) {
        if (result.error === "authentication_rejected") { a.health = "invalid"; a.verified = false; }
        throw Error(
          explain(result.error || "cloud_request_failed") +
            (result.httpStatus ? ` (HTTP ${result.httpStatus})` : ""),
        );
      }
      a.lastError = "";
      if (input.action === "send_code") {
        codeResendUntil.set(codeResendKey(a), Date.now() + 60000);
        codeAttention.set(a, Date.now());
        if (!(a.region === "cn" && a.signInMode === "code")) a.needsCode = true;
        a.expanded = !$("cloudAccounts").hidden;
        accounts();
        message(
          t(
            "验证码已发送，请填写收到的验证码。",
            "Code sent. Enter the verification code.",
          ),
        );
        return;
      }
      a.signedIn = true;
      a.credentialLocked = false;
      a.passwordLocked = false;
      a.loginCleared = false;
      if (result.token) {
        a.token = result.token;
        a.tokenSaved = false;
        a.clearToken = false;
      }
      if (result.userId) a.userId = result.userId;
      if (input.action !== "devices") {
        a.verified = true;
        a.needsCode = false; delete a.tfaKey;
        a.loginMethod = input.action === "validate" ? "token" : "password";
        if (input.action === "validate" || (input.action === "login_code" && a.signInMode === "code" && !pendingPasswords.has(a))) {
          a.rememberPassword = false;
          a.password = "";
          a.passwordEnc = "";
          a.passwordSaved = false;
          pendingPasswords.delete(a);
        }
      }
      if (a.token) {
        try {
          const encoded = a.token
            .split(".")[1]
            .replace(/-/g, "+")
            .replace(/_/g, "/");
          const payload = JSON.parse(
            atob(encoded.padEnd(Math.ceil(encoded.length / 4) * 4, "=")),
          );
          a.expiresAt = payload.exp || 0;
        } catch {
          a.expiresAt = 0;
        }
      }
      if (pendingPasswords.has(a)) {
        a.passwordEnc = pendingPasswords.get(a);
        pendingPasswords.delete(a);
        a.passwordSaved = true;
        a.rememberPassword = true;
      }
      if (result.devices) {
        a.devices = result.devices;
        a.health = "valid";
        accounts();
        profiles();
        message(
          t(
            `已获取 ${result.devices.length} 台设备。`,
            `Found ${result.devices.length} devices.`,
          ),
        );
      } else {
        a.needsCode = false;
        delete a.tfaKey;
        accounts();
        message(t("登录成功，正在获取设备…", "Signed in. Fetching devices…"));
        cloudBusy = false;
        await cloud(a, { action: "devices" });
      }
      return;
    }
    throw Error(t("请求超时，请稍后重试。", "Request timed out. Retry later."));
  } catch (error) {
    if (!current()) return;
    a.lastError = failure(error);
    if (input.action !== "devices") a.verified = false;
    a.expanded = !$("cloudAccounts").hidden && input.action !== "devices";
    if (
      /verification_required/.test(error.message) ||
      error.message.startsWith(explain("verification_required"))
    )
      a.needsCode = true;
    accounts();
    message(failure(error), true);
  } finally {
    if (current()) {
      cloudBusy = false;
      updatePending();
      for (const b of document.querySelectorAll(
        "#accounts button,#accounts input,#accounts select,#addAccount,#addSimulator,#save,#language",
      ))
        b.disabled = b.dataset.cloudLocked === "true" || (!!b.dataset.resendKey && (codeResendUntil.get(b.dataset.resendKey) || 0) > Date.now());
    }
  }
}
async function load() {
  cfg = await api("/api/config");
  const active = (cfg.printers || [])[cfg.activePrinter || 0];
  cfg.printers = (cfg.printers || []).filter(Boolean);
  for (const p of cfg.printers) p.mode = p.mode || "lan";
  cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
  cfg.cloudAccounts = cfg.cloudAccounts || [];
  for (const a of cfg.cloudAccounts) {
    a.expanded = false;
    a.loginMethod = a.loginMethod || "token";
    a.authMode = a.loginMethod === "password" ? "password" : "token";
    a.signInMode = a.signInMode || "password";
    a.signedIn = !!(
      a.signedIn ||
      a.tokenSaved ||
      a.passwordSaved ||
      a.passwordLocked ||
      a.userId
    );
  }
  cfg.settings = cfg.settings || {};
  await saveLanguageChoice();
  lang = configuredLanguage(cfg.settings.portalLanguage);
  info.portalLanguage=lang;
  language();
  savedSnapshot = JSON.parse(JSON.stringify(cfg));
  for (const p of cfg.printers) if (p.mode === "lan") lanOrigin(p);
  $("login").hidden = true;
  $("change").hidden = true;
  $("app").hidden = false;
  $("logout").hidden = false;
  $("saveFeedback").append($("status"));
  applyFields();
  selected = 0;
  profiles();
  accounts();
  info = await api("/api/info");
  updateInfo(info);
  message("");
  savedSnapshot = JSON.parse(JSON.stringify(cfg));
  cleanState = configState();
  initializeClockSettings(cfg.settings);
  for (const input of document.querySelectorAll("#settings input,#settings select")) trackField(input,controlValue(input));
  refreshUnsaved();
  updatePending();
  if (location.hash.startsWith("#account=")) {
    const a = cfg.cloudAccounts.find(
      (a) => a.id === decodeURIComponent(location.hash.slice(9)),
    );
    if (a) openAccount(a);
  }
  startPolling();
}
function startPolling() {
  clearInterval(poll);
  poll = setInterval(async () => {
    try {
      const session = await api("/api/session");
      csrf = session.csrf;
      let d = await api("/api/info");
      updateInfo(d);
    } catch {}
  }, 5000);
}
function persistableConfiguration() {
  const payload = JSON.parse(JSON.stringify(cfg));
  payload.cloudAccounts = payload.cloudAccounts.flatMap((a, index) => {
    const source = cfg.cloudAccounts[index];
    const previous = savedSnapshot?.cloudAccounts?.find(old => old.id === a.id);
    const locked = (a.credentialLocked || a.passwordLocked) && !a.verified;
    const ready = validName(a.name) && ((a.simulator && a.signedIn) || a.imported || (a.clearToken && previous) || (!locked && accountAuthenticated(a) && (!accountPending(source) || a.verified)));
    if (ready) return [a];
    if (!previous) return [];
    const retained = JSON.parse(JSON.stringify(previous));
    if (validName(a.name)) retained.name = a.name;
    return [retained];
  });
  payload.printers = payload.printers.flatMap((p, index) => {
    const source = cfg.printers[index];
    const origin = p.mode === "lan" ? lanOrigin(source) : null;
    const previous = origin?.saved || savedSnapshot?.printers?.find(old => old.serialNumber === p.serialNumber && old.mode === p.mode && (p.mode !== "cloud" || old.accountId === p.accountId));
    const validSerial = typeof p.serialNumber === "string" && p.serialNumber.length > 0 && p.serialNumber.length < 32 && !/[\x00-\x1f\x7f]/.test(p.serialNumber);
    const validLan = /^[A-Za-z0-9._-]+$/.test(p.host || "") && (p.accessCode ? /^[!-~]{8}$/.test(p.accessCode) : p.accessCodeSaved || p.accessCodeEnc);
    const ready = validName(p.name) && validSerial && (p.mode === "cloud" ? p.accountId : validLan);
    const validated = p.mode !== "lan" || !lanPending(source) || lanVerified.get(source) === lanFingerprint(source);
    if (ready && validated) {
      if (p.mode === "lan" && previous && !p.accessCode && p.accessCodeSaved && (previous.host !== p.host || previous.serialNumber !== p.serialNumber))
        p.credentialSource = {host:previous.host,serialNumber:previous.serialNumber};
      return [p];
    }
    if (!previous) return [];
    const retained = JSON.parse(JSON.stringify(previous));
    if (validName(p.name)) retained.name = p.name;
    return [retained];
  });
  const selectedPrinter = cfg.printers[cfg.activePrinter];
  const active = payload.printers.find(p => p.mode === selectedPrinter?.mode && p.serialNumber === (lanOrigin(selectedPrinter || {})?.saved.serialNumber || selectedPrinter?.serialNumber) && (p.mode !== "cloud" || p.accountId === selectedPrinter?.accountId)) || payload.printers.find(p => p.mode === selectedPrinter?.mode && p.serialNumber === selectedPrinter?.serialNumber && (p.mode !== "cloud" || p.accountId === selectedPrinter?.accountId));
  payload.printers = payload.printers.filter(
    (p) =>
      p.mode !== "cloud" ||
      payload.cloudAccounts.some((a) => a.id === p.accountId),
  );
  payload.activePrinter = Math.max(0, payload.printers.indexOf(active));
  for (const p of payload.printers) { delete p.health; delete p.verified; }
  for (const a of payload.cloudAccounts) {
    delete a.imported;
    delete a.verified;
    delete a.lastError;
    delete a.devices;
    delete a.expanded;
    delete a.needsCode;
    delete a.health;
    delete a.tfaKey;

  }
  return payload;
}
async function saveConfiguration(automatic = false) {
  if (!cfg || saving || cloudBusy || $("app").hidden) return;
  const invalid = !automatic && [...cfg.printers, ...cfg.cloudAccounts].some(
    (x) => !validName(x.name),
  );
  if (invalid)
    return message(
      t(
        "自定义名称仅限英文字母、数字和英文符号。",
        "Custom names allow ASCII letters, numbers and symbols only.",
      ),
      true,
    );
  for (const input of document.querySelectorAll("#settings input,#settings select"))
    if (automatic ? !input.checkValidity() : !input.reportValidity()) return;
  const b = $("save");
  b.disabled = true;
  saving = true;
  let sentState = "", failed = false;
  try {
    cfg.ssid = $("ssid").value;
    cfg.pwd = $("wifiPassword").value;
    cfg.activePrinter = Number($("defaultDevice").value) || 0;
    Object.assign(cfg.settings, {
      language: $("screenLanguage").value,
      backlight: Number($("brightness").value),
      tftOff: Number($("sleep").value),
      wop: $("wake").checked,
      soundVolume: Number($("soundVolume").value), touchSound: $("touchSound").checked, warningSound: $("warningSound").checked, statusLed: $("statusLed").checked,
      deepSleepAfter: $("deepEnabled").checked?Number($("deepSleepAfter").value):0,
      ota: $("ota").checked,
    });
    const imported = cfg.cloudAccounts.some(a => a.imported);
    const payload = persistableConfiguration();
    sentState = configState();
    const relockLan = cfg.printers.filter(p => p.mode === "lan" && lanPending(p) && lanVerified.get(p) === lanFingerprint(p));
    for (const a of payload.cloudAccounts) {
      const previous = savedSnapshot?.cloudAccounts?.find(old => old.id === a.id);
      if(previous && !a.token && (previous.region || "cn") !== (a.region || "cn"))
        a.credentialSource = {region:previous.region || "cn",account:previous.account || ""};
    }
    const sentControls = [...document.querySelectorAll("#app input,#app select")].map(input => [input,controlValue(input)]);
    const result = await api("/api/config", payload);
    for (const p of payload.printers) delete p.credentialSource;
    for (const a of payload.cloudAccounts) delete a.credentialSource;
    for (const [input,value] of sentControls) if (input.closest("#settings")) trackField(input,value);
    for (const a of cfg.cloudAccounts) delete a.imported;
    savedSnapshot = payload;
    for (const p of cfg.printers) if (p.mode === "lan" && (!lanPending(p) || lanVerified.get(p) === lanFingerprint(p))) {
      const saved = payload.printers.find(old => old.mode === "lan" && old.serialNumber === p.serialNumber);
      if (saved) {
        lanOrigins.set(p, {fingerprint:lanFingerprint(p), saved}); newProfiles.delete(p);
        if (lanVerified.get(p) === lanFingerprint(p)) lanVerified.delete(p);
      }
    }
    if (automatic && relockLan.length) profiles();
    for (const input of document.querySelectorAll("#app input,#app select")) input.dispatchEvent(new Event("saved"));
    saveRetryCount = 0; saveRetryState = "";
    cleanState = sentState;
    if (result.restarting) {
      clearInterval(poll);
      message(t("已保存，设备正在重启；请稍后刷新。", "Saved. Restarting; reload shortly."));
    } else if (result.reconnecting) {
      clearInterval(poll);
      message(t("已保存，正在重新连接 Wi-Fi；请使用新的网络地址访问。", "Saved. Reconnecting Wi-Fi; use the new network address."));
    } else {
      if (!automatic || imported) await load();
      message(t("已自动保存。", "Saved automatically."));
    }
  } catch (error) {
    message(failure(error), true);
    failed = true;
    if (automatic && [409, 429, 503].includes(error.status)) {
      const state = configState();
      if (saveRetryState !== state) { saveRetryState = state; saveRetryCount = 0; }
      if (saveRetryCount < 3) {
        ++saveRetryCount;
        clearTimeout(saveTimer);
        saveTimer = setTimeout(updatePending, 3000);
      }
    }
  } finally {
    saving = false; b.disabled = false;
    refreshUnsaved();
    if (!failed) updatePending();
  }
  return !failed;
}
$("save").onclick = () => saveConfiguration(false);
let scanTimer;
async function scan(refresh = false, attempt = 0) {
  clearTimeout(scanTimer);
  $("scanSetup").disabled = true;
  try {
    const data = await api("/api/wifi/scan"+(refresh?"?refresh=1":""));
    const networks = (data.networks || []).filter((n, i, all) => n.ssid && all.findIndex(x => x.ssid === n.ssid) === i);
    $("setupNetworks").replaceChildren(
      new Option(t("选择网络", "Choose a network"), ""),
      ...networks.map(
        (n) => new Option(`${n.ssid} (${n.rssi} dBm)`, n.ssid),
      ),
    );
    $("setupNetworks").firstElementChild.dataset.t = "chooseNetwork";
    $("setupNetworkChoice").hidden = networks.length === 0;
    $("setupNetworks").value = $("setupSsid").value;
    if (data.scanning) scanTimer = setTimeout(()=>scan(false,attempt), 1500);
    else {
      $("scanSetup").disabled = false;
      if (!networks.length) message(t("未找到 Wi-Fi，可手动填写名称或重试。", "No Wi-Fi networks found. Enter the name manually or retry."));
    }
  } catch (error) {
    if(error.code==="wifi_scan_failed"&&attempt<2){scanTimer=setTimeout(()=>scan(true,attempt+1),1500);return;}
    $("scanSetup").disabled = false;
    message(failure(error), true);
  }
}
$("scanSetup").onclick = () => scan(true);
$("setupNetworks").onchange = () => {
  if ($("setupNetworks").value) $("setupSsid").value = $("setupNetworks").value;
};
$("saveWifi").onclick = async () => {
  try {
    await api("/api/wifi", {
      ssid: $("setupSsid").value,
      language: lang,
      pwd: $("setupPassword").value,
    });
    pendingLanguage = null;
    message(t("配网已保存，正在重新连接。", "Wi-Fi saved. Reconnecting."));
  } catch (error) {
    message(failure(error), true);
  }
};
(async () => {
  try {
    info = await api("/api/info");
    if (!pendingLanguage) { lang=configuredLanguage(info.portalLanguage);language(); }
    if (info.ap) {
      $("setup").hidden = false;
      $("login").hidden = true;
      scan(true);
      return;
    }
    try {
      let session = await api("/api/session");
      csrf = session.csrf;
      if (session.mustChange) showChange();
      else await load();
    } catch (error) {
      if (error.status !== 401) message(failure(error), true);
    }
  } catch (error) {
    message(failure(error), true);
  } finally {
    document.querySelector("main").removeAttribute("aria-busy");
  }
})();

$("resetDevice").onclick = async () => {
  if (
    !confirm(
      t(
        "重置内部配置和管理密码？SD 备份将保留；拓竹账号密码需重新输入。",
        "Reset internal settings and admin password? SD backups remain; Bambu account passwords must be re-entered.",
      ),
    )
  )
    return;
  try {
    await api("/api/reset", { confirm: "RESET" });
    clearInterval(poll);
    message(t("正在重置并重启。", "Resetting and restarting."));
  } catch (e) {
    message(failure(e), true);
  }
};
$("uploadFirmware").onclick = () => {
  const file = $("firmwareFile").files[0];
  if (!file)
    return message(t("请选择应用固件。", "Choose application firmware."), true);
  if (!confirm(t("上传此固件并重启？", "Install this firmware and restart?")))
    return;
  const form = new FormData();
  form.append("firmware", file);
  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/api/firmware");
  xhr.setRequestHeader("X-XTouch-CSRF", csrf);
  xhr.timeout = 180000;
  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      $("uploadProgress").hidden = false;
      $("uploadProgress").value = (e.loaded / e.total) * 100;
    }
  };
  xhr.onload = () => {
    let result;
    try {
      result = JSON.parse(xhr.responseText);
    } catch {}
    message(
      xhr.status === 200
        ? t("升级成功，正在重启。", "Updated. Restarting.")
        : explain(result?.error || "update_failed"),
      xhr.status !== 200,
    );
    $("uploadFirmware").disabled = false;
    if (xhr.status !== 200) startPolling();
  };
  xhr.onerror = xhr.ontimeout = () => {
    message(t("上传中断，请重试。", "Upload interrupted. Retry."), true);
    $("uploadFirmware").disabled = false;
    startPolling();
  };
  $("uploadFirmware").disabled = true;
  clearInterval(poll);
  xhr.send(form);
};

function authLayout() {
  const app = $("app"),
    head = document.querySelector("header"),
    card = ["login", "change", "setup"].map($).find((x) => !x.hidden);
  const auth = app.hidden && !!card;
  // Keep shared chrome out of the first paint until the session route is known.
  const waiting = app.hidden && !card, footer = document.querySelector("footer");
  if (head.hidden !== waiting) head.hidden = waiting;
  if (footer.hidden !== app.hidden) footer.hidden = app.hidden;
  document.body.classList.toggle("authMode", auth);
  if (auth) {
    if (head.parentElement !== card) card.prepend(head);
  } else if (head.parentElement !== document.body) document.body.prepend(head);
}
new MutationObserver(authLayout).observe(document.querySelector("main"), {
  subtree: true,
  attributes: true,
  attributeFilter: ["hidden"],
});
authLayout();

function applyFields() {
  $("ssid").value = cfg.ssid || "";
  $("wifiPassword").value = "";
  for (const id of ["ssid", "wifiPassword"]) {
    const input = $(id);
    if (!input.dataset.editBound) { lockEditor(input); input.dataset.editBound = "1"; }
    input.dispatchEvent(new Event("saved"));
  }
  $("screenLanguage").value = languageCodes.includes(cfg.settings.language) ? cfg.settings.language : "en";
  $("brightness").value = cfg.settings.backlight ?? 200;$("brightnessValue").textContent=$("brightness").value;
  $("sleep").value = cfg.settings.tftOff ?? 15;updateSleepValue();
  $("wake").checked = cfg.settings.wop ?? true;
  $("ota").checked = cfg.settings.ota ?? true;
  $("touchSound").checked=cfg.settings.touchSound??false;$("warningSound").checked=cfg.settings.warningSound??false;$("statusLed").checked=cfg.settings.statusLed??true;
  $("deepSleepAfter").value=cfg.settings.deepSleepAfter??0;
  $("deepEnabled").checked=Number(cfg.settings.deepSleepAfter??0)>0;
  if(!$("deepEnabled").checked)$("deepSleepAfter").value=60;
  $("soundVolume").value=cfg.settings.soundVolume??20;$("soundVolumeValue").textContent=$("soundVolume").value+"%";
  updateDeepFields();
}

function updateDeepFields(){const enabled=$("deepEnabled").checked;$("deepTimers").hidden=!enabled;$("deepSleepAfter").disabled=!enabled;}
function configState() {
  return JSON.stringify(
    [
      persistableConfiguration(),
      ...[
        "ssid",
        "wifiPassword",
        "screenLanguage",
        "brightness",
        "sleep",
        "defaultDevice", "deepSleepAfter", "soundVolume",
      ].map((id) => $(id).value),
      $("deepEnabled").checked, $("wake").checked, $("touchSound").checked, $("warningSound").checked, $("statusLed").checked,
      $("ota").checked,
    ],
    (key, value) =>
      [
        "authMode", "imported", "credentialSource", "portalLanguage",
        "health",
        "devices",
        "expanded",
        "lastError",
        "needsCode",
        "tfaKey",
        "expiresAt",
        "accessCodeSaved", "accessCodeLocked",
        "tokenSaved",
        "passwordSaved",
        "passwordLocked", "credentialLocked",
      ].includes(key)
        ? undefined
        : value,
  );
}
function updatePending() {
  refreshUnsaved();
  clearTimeout(saveTimer);
  const dirty = !!cfg && !!cleanState && configState() !== cleanState;
  $("save").hidden = !restartRequired;
  $("save").classList.toggle("pending", restartRequired && dirty);
  if (!dirty || saving || cloudBusy || restartRequired || $("app").hidden) return;
  if (["ssid", "wifiPassword"].includes(document.activeElement?.id)) return;
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => saveConfiguration(true), 1000);
}
for (const event of ["input", "change", "click", "focusout"])
  document.addEventListener(event, () => queueMicrotask(updatePending));
$("importBackup").onclick = () => $("backupFile").click();
$("backupFile").onchange = async () => {
  const file = $("backupFile").files[0];
  if (!file) return;
  try {
    if (file.size > 196608)
      throw Error(t("备份文件过大。", "Backup file too large."));
    const next = JSON.parse(await file.text());
    if (
      !next ||
      typeof next.ssid !== "string" ||
      !Array.isArray(next.printers) ||
      next.printers.length > 100 ||
      next.printers.some(
        (p) => p && (!p.serialNumber || typeof p.serialNumber !== "string"),
      ) ||
      (next.cloudAccounts &&
        (!Array.isArray(next.cloudAccounts) || next.cloudAccounts.length > 50))
    )
      throw Error(t("配置备份格式错误。", "Invalid configuration backup."));
    if (
      !confirm(
        t(
          "导入并应用此备份？有效配置将自动保存。",
          "Import and apply this backup? Valid configuration will save automatically.",
        ),
      )
    )
      return;
    next.settings = next.settings || cfg.settings;
    next.cloudAccounts = next.cloudAccounts || [];
    for (const a of next.cloudAccounts) {
      a.expanded = false;
      a.verified = false;
      a.health = "unknown";
      delete a.tokenSaved;
      delete a.passwordSaved;
      delete a.passwordLocked;
      delete a.credentialLocked;
      a.signedIn = !!(a.signedIn || a.token || a.tokenSaved || a.userId || a.passwordEnc);
      a.imported = true;
      a.loginMethod = a.loginMethod || "token";
      a.authMode = a.loginMethod === "password" ? "password" : "token";
      a.signInMode = a.signInMode || "password";
    }
    cfg = next;
    cfg.printers = cfg.printers.filter(Boolean);
    for (const p of cfg.printers) p.mode = p.mode || "lan";
    for (const p of cfg.printers) if(p.mode === "lan")p.accessCodeEncrypted ??= !!p.accessCodeEnc;
    for (const p of cfg.printers) if (p.mode === "lan") lanVerified.set(p, lanFingerprint(p));
    cfg.activePrinter = Math.min(
      cfg.activePrinter || 0,
      Math.max(0, cfg.printers.length - 1),
    );
    applyFields();
    $("wifiPassword").value = cfg.pwd || "";
    selected = 0;
    profiles();
    accounts();
    updatePending();
    message(
      t(
        "备份已载入，正在自动保存。",
        "Backup loaded. Saving automatically.",
      ),
    );
  } catch (e) {
    message(
      e instanceof SyntaxError
        ? t("配置备份格式错误。", "Invalid configuration backup.")
        : failure(e),
      true,
    );
  } finally {
    $("backupFile").value = "";
  }
};

$("deepEnabled").onchange=()=>{updateDeepFields();updatePending();};

function updateSleepValue(){const n=Number($("sleep").value);$("sleepValue").textContent=n<5?t("关闭","Off"):n+"m";}
$("brightness").oninput=()=>{$("brightnessValue").textContent=$("brightness").value;updatePending();};
$("sleep").oninput=()=>{updateSleepValue();updatePending();};
$("soundVolume").oninput=()=>{$("soundVolumeValue").textContent=$("soundVolume").value+"%";updatePending();};
