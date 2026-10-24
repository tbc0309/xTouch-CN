const languageMetadata=[{"code":"zh","name":"简体中文","website":true},{"code":"zh-TW","name":"繁體中文","website":false},{"code":"en","name":"English","website":true},{"code":"fr","name":"Français","website":false},{"code":"de","name":"Deutsch","website":false},{"code":"ru","name":"Русский","website":false},{"code":"ja","name":"日本語","website":false},{"code":"ko","name":"한국어","website":false}];
const languageCodes=languageMetadata.map(row=>row.code);
// Normalize a saved language value; display and portal preferences are independent.
function configuredLanguage(value) {
  return languageCodes.includes(value) ? value : 'en';
}

const websiteCodes=languageMetadata.filter(row=>row.website).map(row=>row.code);
function websiteLanguage(){const value=new URLSearchParams(location.search).get('lang');return websiteCodes.includes(value)?value:'en';}
function saveWebsiteLanguage(value){const url=new URL(location.href);url.searchParams.set('lang',value);history.replaceState(null,'',url);}
export {languageMetadata,languageCodes,websiteLanguage,saveWebsiteLanguage};
