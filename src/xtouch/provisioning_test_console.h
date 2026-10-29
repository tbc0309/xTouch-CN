#pragma once
// Explicit laboratory builds only. No command parser enters release firmware.
#if defined(XTOUCH_TEST_AUTOMATION) && XTOUCH_TEST_AUTOMATION
void provisioningTestCommand(JsonObject request){
 const char *command=request["command"]|"";
 StaticJsonDocument<1536> out;
 out["command"]=command;out["ok"]=true;
 if(request.containsKey("_id"))out["_id"]=request["_id"].as<unsigned>();
 if(!strcmp(command,"wifi"))showRequest=2;
 else if(!strcmp(command,"close"))showRequest=3;
 else if(!strcmp(command,"ble")){
#if XTOUCH_FEATURE_BLE_PROVISIONING
  showRequest=5;
#else
  out["ok"]=false;out["error"]="not_compiled";
#endif
 }else if(!strcmp(command,"cancelBle"))showRequest=bootComplete?6:7;
 else if(!strcmp(command,"station")){
  if(!XTouchSchema::text(request["ssid"],33,true)||!XTouchSchema::text(request["pwd"],65))out["ok"]=false;
  else if(wifiSetupState==WifiSetupState::connecting)out["ok"]=false;
  else {
   pendingWifiSsid=request["ssid"].as<const char*>();pendingWifiPassword=request["pwd"]|"";pendingWifiCredentials=true;
   wifiSetupState=WifiSetupState::connecting;wifiSetupStarted=0;apExitAt=0;setupLastActivity=millis();wifiApplyAt=millis()+500;
  }
 }else if(!strcmp(command,"firstSetup")){
  // Exercise a genuinely absent network/printer file, retaining calibration and
  // device-bound keys so the saved backup can be restored after the test.
  if(!request["backedUp"].as<bool>()||xtouch_storage_sd_available()){
   out["ok"]=false;out["error"]="backup_or_sd";
  }else{
   bool cleared=true;
   for(const char *path:{"/xtouch.json.tmp","/xtouch.json.previous","/xtouch.json"})
    if(SPIFFS.exists(path)&&!SPIFFS.remove(path))cleared=false;
   out["ok"]=cleared;
   if(out["ok"].as<bool>())rebootAt=millis()+200;
  }
 }else if(!strcmp(command,"boot")){
  out["ok"]=saveWifiCredentials(request["ssid"]|"",request["pwd"]|"")==0;
  if(out["ok"].as<bool>())rebootAt=millis()+200;
 }else if(!strcmp(command,"ap")){
  if(request["enabled"].as<bool>())out["ok"]=WiFi.mode(WIFI_AP_STA)&&WiFi.softAP(request["ssid"]|"XTOUCH-REGRESSION",request["pwd"]|"fixture123");
  else out["ok"]=WiFi.softAPdisconnect(false);
 }else if(!strcmp(command,"reboot"))rebootAt=millis()+200;
 else if(!strcmp(command,"state")){
  out["uptimeMs"]=millis();out["bootComplete"]=bootComplete;out["screen"]=xTouchConfig.currentScreenIndex;
  out["wifiStatus"]=int(WiFi.status());out["mode"]=WiFi.getMode();out["setupAP"]=apMode;out["qr"]=qrActive;
  out["attempts"]=WiFi.connectionAttempts();out["reason"]=WiFi.lastDisconnectReason();out["exhausted"]=WiFi.retryExhausted();
  out["totalAttempts"]=WiFi.totalConnectionAttempts();
  out["pendingCredentials"]=pendingWifiCredentials;
  out["setupState"]=wifiSetupState==WifiSetupState::connecting?"connecting":wifiSetupState==WifiSetupState::connected?"connected":wifiSetupState==WifiSetupState::failed?"failed":"idle";
  out["setupIdleMs"]=uint32_t(millis()-setupLastActivity);out["heap"]=esp_get_free_heap_size();
  if(request.containsKey("expectSsid")){
   auto saved=xtouch_storage_read("/xtouch.json",0);saved.shrinkToFit();
   out["savedCredentialsMatch"]=!strcmp(saved["ssid"]|"",request["expectSsid"]|"")&&!strcmp(saved["pwd"]|"",request["expectPwd"]|"");
   wifi_ap_record_t associated{};const char *expected=request["expectSsid"]|"";size_t expectedBytes=strlen(expected);
   bool associatedOk=esp_wifi_sta_get_ap_info(&associated)==ESP_OK;size_t associatedBytes=associatedOk?strnlen((const char*)associated.ssid,sizeof(associated.ssid)):0;
   out["associatedSsidBytes"]=associatedBytes;
   out["associatedAuthmode"]=associatedOk?int(associated.authmode):-1;
   out["associatedSsidMatches"]=associatedOk&&expectedBytes<=32&&associatedBytes==expectedBytes&&!memcmp(associated.ssid,expected,expectedBytes);
  }
  if(request["expectIpPrefix"].is<const char*>())out["connectedIpPrefixMatches"]=WiFi.localIP().toString().startsWith(request["expectIpPrefix"].as<const char*>());
#if XTOUCH_FEATURE_BLE_PROVISIONING
  JsonObject radio=out.createNestedObject("bluetooth");board_ble_status(radio);radio.remove("proofOfPossession");
#endif
 }else{out["ok"]=false;out["error"]="unknown_command";}
 String response;serializeJson(out,response);printf("[PROV_TEST] %s\n",response.c_str());fflush(stdout);
}
void provisioningTestPoll(){
 static bool initialized=false;static char line[512];static unsigned used=0;
 if(!initialized){
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED && !defined(XTOUCH_BOARD_P4)
  if(!usb_serial_jtag_is_driver_installed()){
   usb_serial_jtag_driver_config_t config={};config.rx_buffer_size=1024;config.tx_buffer_size=1024;
   usb_serial_jtag_driver_install(&config);
  }
  if(usb_serial_jtag_is_driver_installed())usb_serial_jtag_vfs_use_driver();
#endif
  int flags=fcntl(STDIN_FILENO,F_GETFL,0);if(flags>=0)fcntl(STDIN_FILENO,F_SETFL,flags|O_NONBLOCK);initialized=true;
 }
 for(unsigned n=0;n<128;++n){
  char c;int received=0;
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED && !defined(XTOUCH_BOARD_P4)
  if(usb_serial_jtag_is_driver_installed())received=usb_serial_jtag_read_bytes(&c,1,0);
#else
  received=read(STDIN_FILENO,&c,1);
#endif
  if(received!=1)break;
  if(c=='\n'){
   line[used]=0;StaticJsonDocument<512> request;
   if(used&&!deserializeJson(request,line))provisioningTestCommand(request.as<JsonObject>());
   used=0;
  }else if(c!='\r'){
   if(used+1<sizeof(line))line[used++]=c;else used=0;
  }
 }
}
#endif
