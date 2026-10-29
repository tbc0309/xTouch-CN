const languageMetadata=[{"code":"zh","name":"简体中文","website":true},{"code":"zh-TW","name":"繁體中文","website":false},{"code":"en","name":"English","website":true},{"code":"fr","name":"Français","website":false},{"code":"de","name":"Deutsch","website":false},{"code":"ru","name":"Русский","website":false},{"code":"ja","name":"日本語","website":false},{"code":"ko","name":"한국어","website":false}];
const languageCodes=languageMetadata.map(row=>row.code);
// Normalize a saved language value; display and portal preferences are independent.
function configuredLanguage(value) {
  return languageCodes.includes(value) ? value : 'en';
}

const websiteCodes=languageMetadata.filter(row=>row.website).map(row=>row.code);
const websiteLanguageKey='xTouch-CN.website.language';
function chineseWebsiteLanguage(value){return /^(?:zh|cmn|yue)(?:[-_]|$)/i.test(value||'');}
function websiteChoice(value){return chineseWebsiteLanguage(value)?'zh':value==='en'?'en':null;}
function websiteLanguage(){
  const explicit=websiteChoice(new URLSearchParams(location.search).get('lang'));
  if(explicit)return explicit;
  try {const saved=websiteChoice(localStorage.getItem(websiteLanguageKey));if(saved)return saved;} catch {}
  const preferred=navigator.languages?.[0]||navigator.language||'en';
  return chineseWebsiteLanguage(preferred)?'zh':'en';
}
function saveWebsiteLanguage(value){
  const chosen=websiteChoice(value);if(!chosen)return;
  try {localStorage.setItem(websiteLanguageKey,chosen);} catch {}
  const url=new URL(location.href);url.searchParams.set('lang',chosen);history.replaceState(null,'',url);
}
export {languageMetadata,languageCodes,websiteLanguage,saveWebsiteLanguage};
