#pragma once
#include <Arduino.h>
// Device-local random key. Never exported to configuration or SD backups.
bool xtouch_vault_begin();
bool xtouch_vault_rotate();
String xtouch_vault_seal(const String &plain,const String &binding);
bool xtouch_vault_open(const String &sealed,const String &binding,String &plain);

bool xtouch_vault_selftest();
