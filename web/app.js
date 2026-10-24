import {websiteLanguage,saveWebsiteLanguage} from './language.js';
import {renderFooter} from './page-footer.js';
const words = {
  "errorLookup": [
    "故障码查询",
    "Error lookup"
  ],
  "projectLink": [
    "项目地址",
    "Project"
  ],
  "eyebrow": [
    "在线安装 · 扫码配网",
    "INSTALL · CONNECT · GET STARTED"
  ],
  "title": [
    "在线安装 xTouch-CN",
    "Install xTouch-CN online"
  ],
  "intro": [
    "选择对应主板固件，刷写后扫码配网，在网页管理打印机。",
    "Select your board, install, scan to join Wi-Fi, and manage printers in your browser."
  ],
  "usbTag": [
    "USB 在线安装",
    "USB WEB INSTALL"
  ],
  "usbTitle": [
    "连接并刷写",
    "Connect and install"
  ],
  "boardSelect": [
    "选择主板",
    "Select board"
  ],
  "usb1": [
    "电脑使用 Chrome 或 Edge，关闭占用串口的软件。",
    "Use desktop Chrome or Edge. Close software using the serial port."
  ],
  "usb2": [
    "确认主板型号，加载刷写工具，再选择它的串口。",
    "Check the board model, load the installer, then select its serial port."
  ],
  "usb3": [
    "按提示安装，CYD 首次开机需触摸校准，再扫码配网。",
    "Follow the installation dialog. CYD needs first-boot touch calibration. Then scan to join Wi-Fi."
  ],
  "erase": [
    "保留设置时不要勾选 Erase device（擦除设备）；勾选会清空内部配置和校准。SD 备份可能在开机时恢复，全新配置请先移除 SD 卡。刷写期间不要断电、拔线或关闭页面。",
    "To keep settings, leave “Erase device” unchecked. Erasing removes internal settings and calibration. SD backups may be restored at startup; remove SD for a fresh setup. Do not unplug USB, turn off power or close the page while installing."
  ],
  "loadInstaller": [
    "加载刷写工具",
    "Load installer"
  ],
  "installerEnglish": [
    "刷写工具的连接、进度和确认窗口由 ESP Web Tools 提供，目前为英文。",
    "Connection, progress and confirmation dialogs are supplied by ESP Web Tools in English."
  ],
  "setupTitle": [
    "扫码配网与初始化",
    "Wi-Fi and setup"
  ],
  "setup1": [
    "CYD 首次开机需触摸校准；S3 V2/P4 无需校准。",
    "Calibrate CYD on first boot; S3 V2/P4 need no calibration."
  ],
  "setup2": [
    "扫描屏幕二维码连接 XTOUCH-CN 热点；密码 12345678。",
    "Scan the screen QR code to join the XTOUCH-CN hotspot. Password: 12345678."
  ],
  "setup3": [
    "打开 192.168.4.1，连接家庭 2.4GHz Wi-Fi。",
    "Open 192.168.4.1 and join your 2.4GHz Wi-Fi."
  ],
  "setup4": [
    "在设置打开网页管理扫码进入后台；初始密码 12345678，首次登录修改后添加局域网或云设备。",
    "Open Web management in Settings and scan its QR code. Sign in with 12345678, change it, then add LAN or cloud printers."
  ],
  "otaHint": [
    "已安装 CN：可在设置开启 OTA，或连续点击版本号三次检查更新。",
    "Already on CN? Enable OTA in Settings or tap the version three times to check for updates."
  ],
  "boardHint": [
    "仅支持 ESP32-2432S028R：ILI9341 显示 + XPT2046 电阻触摸，4MB Flash。",
    "For ESP32-2432S028R: ILI9341 display, XPT2046 resistive touch and 4MB Flash."
  ],
  "version": [
    "固件",
    "Firmware"
  ],
  "connect": [
    "选择串口并连接",
    "Select port and connect"
  ],
  "unavailable": [
    "刷写资源暂不可用，请刷新重试或前往 Releases。",
    "Installer assets are unavailable. Reload or visit Releases."
  ],
  "unsupported": [
    "请在电脑 Chrome / Edge 中通过 HTTPS 打开此页面。",
    "Open this page over HTTPS in desktop Chrome or Edge."
  ],
  "loading": [
    "正在加载刷写工具…",
    "Loading installer…"
  ],
  "recoveryHint": [
    "连接失败时，先关闭其他占用串口的软件或网页。S3/P4 确认已开机，按住 BOOT、点按 RESET、松开 BOOT，再重新连接。写入中断后可重新完整安装。",
    "If connection fails, close other serial tools or tabs. Power on S3/P4, hold BOOT, tap RESET, release BOOT and reconnect. If writing is interrupted, repeat the complete installation."
  ],
  "loadFailed": [
    "工具加载失败，请检查网络后重试，或使用 SD 卡升级。",
    "Installer could not load. Check the network and retry, or use SD installation."
  ]
};
let lang=websiteLanguage(), metadata=null, installerReady=false, boardCatalog=[];
const boardPreferenceKey="xTouch-CN.installer.board";
function rememberedBoard(){try{return localStorage.getItem(boardPreferenceKey);}catch{return null;}}
const $=id=>document.getElementById(id), t=key=>(words[key]||[key,key])[lang==="en"?1:0];
function text(tag,value,className){const el=document.createElement(tag);el.textContent=value;if(className)el.className=className;return el;}
function boardName(board){return board.name.replace(/^Waveshare · /,lang==="zh"?"微雪 · ":"Waveshare · ");}
function applyLanguage(){
 renderFooter(lang);
 $("language").value=lang;document.documentElement.lang=lang==="zh"?"zh-CN":"en";
 document.title=lang==="zh"?"xTouch-CN · 在线刷写":"xTouch-CN · Web installer";
 document.querySelectorAll("[data-i18n]").forEach(el=>el.textContent=t(el.dataset.i18n));
 if(metadata)$("version").textContent=`${t("version")} v${metadata.version}`;
 if(installerReady && $("connect-installer"))$("connect-installer").textContent=t("connect");
 if(metadata){
  $("board-name").textContent=boardName(metadata);
  for(const option of $("board").options){const board=boardCatalog.find(b=>b.id===option.value);if(board)option.textContent=boardName(board);}
  $("board-screen").textContent=metadata.id==="p4"?"800 × 480": "320 × 240";
  document.querySelector('[data-i18n="boardHint"]').textContent=metadata.id==="cyd"?t("boardHint"):
   metadata.id==="p4"?(lang==="zh"?"仅适用于所选 P4 4.3 英寸主板，适用 ESP32-P4 芯片版本 v3.x（最低 v3.0），不适用于早期 v1.x 版本。":"For this P4 4.3-inch board only. Supports ESP32-P4 chip revision v3.x (minimum v3.0), not earlier v1.x revisions."):
   (lang==="zh"?"仅适用于 S3 2.8 英寸 V2 / CST3530，不适用于 V1 或其它后缀型号。":"For S3 2.8-inch V2 / CST3530 only, not V1 or other board variants.");
 }
}
function selectBoard(){
 metadata=boardCatalog.find(board=>board.id===$("board").value);
 if(metadata)try{localStorage.setItem(boardPreferenceKey,metadata.id);}catch{}
 installerReady=false;$("installer-widget").replaceChildren();$("load-installer").disabled=!metadata;
 $("load-installer").hidden=false;
 $("installer-status").textContent="";applyLanguage();
}
$("board").addEventListener("change",selectBoard);
$("language").addEventListener("change",()=>{lang=$("language").value;saveWebsiteLanguage(lang);applyLanguage();});
async function initFirmware() {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), 10000);
  try {
    const response = await fetch("firmware.json", {
      cache: "no-cache",
      signal: controller.signal,
    });
    if (!response.ok) throw Error();
    const data = await response.json();
    if (!/^\d+\.\d+\.\d+$/.test(data.version) || !Array.isArray(data.boards))
      throw Error();
    const targets={cyd:["xtouch-2.8","manifest.json"],s3:["esp32-s3-touch-lcd-2.8-v2","manifest-s3.json"],p4:["esp32-p4-wifi6-touch-lcd-4.3","manifest-p4.json"]};
    boardCatalog=data.boards;
    const ids=new Set();
    for(const board of boardCatalog){
      const target=targets[board.id];
      if(!target||ids.has(board.id)||board.version!==data.version||board.manifest!==target[1]||
         board.fullPath!==`firmware/${target[0]}-full.bin`||
         board.appUrl!==`https://github.com/tbc0309/xTouch-CN/releases/download/${data.version}/${target[0]}.bin`||
         !/^[a-f0-9]{64}$/.test(board.fullSha256))throw Error();
      ids.add(board.id);
    }
    if(!boardCatalog.length)throw Error();
    $("board").replaceChildren(...boardCatalog.map(board=>{const option=text("option",boardName(board));option.value=board.id;return option;}));
    const savedBoard=rememberedBoard();
    if(boardCatalog.some(board=>board.id===savedBoard))$("board").value=savedBoard;
    $("board").disabled=false;selectBoard();

  } catch {
    $("version").textContent = "Releases";
    $("load-installer").disabled = true;
    $("installer-status").textContent = t("unavailable");
  } finally {
    clearTimeout(timer);
  }
}
$("load-installer").addEventListener("click", async () => {
  if (!window.isSecureContext || !("serial" in navigator)) {
    $("installer-status").textContent = t("unsupported");
    return;
  }
  if (!metadata) return;
  $("load-installer").disabled = true;
  $("board").disabled = true;
  $("installer-status").textContent = t("loading");
  let timer;
  try {
    await Promise.race([
      import("./vendor/install-button.js"),
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(Error("installer timeout")), 20000);
      }),
    ]);
    await customElements.whenDefined("esp-web-install-button");
    const installer = document.createElement("esp-web-install-button");
    installer.setAttribute("manifest", metadata.manifest);
    const connect = text("button", t("connect"), "button primary");
    connect.id = "connect-installer";
    connect.slot = "activate";
    installer.append(connect);
    $("installer-widget").replaceChildren(installer);
    $("installer-status").textContent = "";
    installerReady = true;
    $("load-installer").hidden = true;
  } catch {
    $("load-installer").disabled = false;
    $("installer-status").textContent = t("loadFailed");
  } finally {
    clearTimeout(timer);
    $("board").disabled = false;
  }
});
applyLanguage();
initFirmware();
