#include "vault.h"
#include <Preferences.h>
#include <memory>
#include <esp_system.h>
#include <esp_idf_version.h>
#if ESP_IDF_VERSION_MAJOR >= 6
#include <psa/crypto.h>
#else
#include <mbedtls/gcm.h>
#endif
#include <mbedtls/base64.h>
namespace {
uint8_t key[32];bool ready=false;
bool initialize(bool rotate){
 Preferences nvs;if(!nvs.begin("xtouch-vault",false))return false;
 bool valid=!rotate&&nvs.getBytesLength("key")==sizeof(key)&&nvs.getBytes("key",key,sizeof(key))==sizeof(key);
 if(!valid){esp_fill_random(key,sizeof(key));valid=nvs.putBytes("key",key,sizeof(key))==sizeof(key);}
 nvs.end();ready=valid;return valid;
}
#if ESP_IDF_VERSION_MAJOR >= 6
int aead(bool encrypt,const uint8_t *key,const uint8_t *nonce,const String &context,const uint8_t *input,size_t n,uint8_t *output,uint8_t *tag){
 psa_key_attributes_t attributes=PSA_KEY_ATTRIBUTES_INIT;mbedtls_svc_key_id_t id=0;
 psa_set_key_type(&attributes,PSA_KEY_TYPE_AES);psa_set_key_bits(&attributes,256);psa_set_key_usage_flags(&attributes,PSA_KEY_USAGE_ENCRYPT|PSA_KEY_USAGE_DECRYPT);psa_set_key_algorithm(&attributes,PSA_ALG_GCM);
 std::unique_ptr<uint8_t[]> combined(new(std::nothrow) uint8_t[n+16]());if(!combined)return -1;
 int rc=psa_crypto_init();if(!rc)rc=psa_import_key(&attributes,key,32,&id);psa_reset_key_attributes(&attributes);size_t length=0;
 if(!rc&&encrypt){rc=psa_aead_encrypt(id,PSA_ALG_GCM,nonce,12,(const uint8_t*)context.c_str(),context.length(),input,n,combined.get(),n+16,&length);if(!rc&&length==n+16){memcpy(output,combined.get(),n);memcpy(tag,combined.get()+n,16);}else if(!rc)rc=-1;}
 if(!rc&&!encrypt){memcpy(combined.get(),input,n);memcpy(combined.get()+n,tag,16);rc=psa_aead_decrypt(id,PSA_ALG_GCM,nonce,12,(const uint8_t*)context.c_str(),context.length(),combined.get(),n+16,output,n,&length);if(!rc&&length!=n)rc=-1;}
 if(id)psa_destroy_key(id);memset(combined.get(),0,n+16);return rc;
}
#endif
String aad(const String &binding){return String((uint32_t)(ESP.getEfuseMac()>>32),HEX)+String((uint32_t)ESP.getEfuseMac(),HEX)+":"+binding;}
}
bool xtouch_vault_begin(){return ready||initialize(false);}
bool xtouch_vault_rotate(){return initialize(true);}
String xtouch_vault_seal(const String &plain,const String &binding){
 if(!xtouch_vault_begin()||plain.isEmpty()||plain.length()>2048)return "";
 std::unique_ptr<uint8_t[]> buffer(new(std::nothrow) uint8_t[plain.length()+28]());if(!buffer)return "";uint8_t *bytes=buffer.get();esp_fill_random(bytes,12);String context=aad(binding);
#if ESP_IDF_VERSION_MAJOR >= 6
 int rc=aead(true,key,bytes,context,(const uint8_t*)plain.c_str(),plain.length(),bytes+28,bytes+12);
#else
 mbedtls_gcm_context gcm;mbedtls_gcm_init(&gcm);
 int rc=mbedtls_gcm_setkey(&gcm,MBEDTLS_CIPHER_ID_AES,key,256);
 if(!rc)rc=mbedtls_gcm_crypt_and_tag(&gcm,MBEDTLS_GCM_ENCRYPT,plain.length(),bytes,12,(const uint8_t*)context.c_str(),context.length(),(const uint8_t*)plain.c_str(),bytes+28,16,bytes+12);
 mbedtls_gcm_free(&gcm);
#endif
 if(rc)return "";
 size_t capacity=4*((plain.length()+30)/3)+1;std::unique_ptr<unsigned char[]> encoded(new(std::nothrow) unsigned char[capacity]);if(!encoded)return "";size_t n=0;rc=mbedtls_base64_encode(encoded.get(),capacity,&n,bytes,plain.length()+28);memset(bytes,0,plain.length()+28);
 return rc?String():String("v1:")+String((char*)encoded.get(),n);
}
bool xtouch_vault_open(const String &sealed,const String &binding,String &plain){
 plain="";if(!xtouch_vault_begin()||!sealed.startsWith("v1:")||sealed.length()>2771)return false;
 const size_t capacity=3*((sealed.length()-3+3)/4);if(capacity<=28||capacity>2076)return false;
 std::unique_ptr<uint8_t[]> buffer(new(std::nothrow) uint8_t[capacity]),output(new(std::nothrow) uint8_t[capacity-27]());if(!buffer||!output)return false;uint8_t *bytes=buffer.get(),*decoded=output.get();size_t n=0;
 if(mbedtls_base64_decode(bytes,capacity,&n,(const unsigned char*)sealed.c_str()+3,sealed.length()-3)||n<=28)return false;
 String context=aad(binding);
#if ESP_IDF_VERSION_MAJOR >= 6
 int rc=aead(false,key,bytes,context,bytes+28,n-28,decoded,bytes+12);
#else
 mbedtls_gcm_context gcm;mbedtls_gcm_init(&gcm);
 int rc=mbedtls_gcm_setkey(&gcm,MBEDTLS_CIPHER_ID_AES,key,256);
 if(!rc)rc=mbedtls_gcm_auth_decrypt(&gcm,n-28,bytes,12,(const uint8_t*)context.c_str(),context.length(),bytes+12,16,bytes+28,decoded);
 mbedtls_gcm_free(&gcm);
#endif
 if(!rc)plain=String((char*)decoded,n-28);memset(decoded,0,capacity-27);memset(bytes,0,capacity);return !rc;
}

bool xtouch_vault_selftest(){
 if(!xtouch_vault_begin())return false;
 String input="vault-test-0123456789",plain;String encrypted=xtouch_vault_seal(input,"test");
 bool valid=xtouch_vault_open(encrypted,"test",plain)&&plain==input;
 valid=valid&&!xtouch_vault_open(encrypted,"other",plain);
 String corrupt=encrypted;corrupt.setCharAt(45,corrupt[45]=='A'?'B':'A');
 valid=valid&&!xtouch_vault_open(corrupt,"test",plain);
 uint8_t previous[32];memcpy(previous,key,32);esp_fill_random(key,32);
 valid=valid&&!xtouch_vault_open(encrypted,"test",plain);memcpy(key,previous,32);memset(previous,0,32);
 String maximum;for(unsigned i=0;i<2048;++i)maximum+='x';
 valid=valid&&xtouch_vault_open(xtouch_vault_seal(maximum,"test"),"test",plain)&&plain==maximum;
 return valid&&xtouch_vault_seal(maximum+"x","test").isEmpty()&&xtouch_vault_open(encrypted,"test",plain);
}
