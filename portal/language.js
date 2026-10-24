// Normalize a saved language value; display and portal preferences are independent.
function configuredLanguage(value) {
  return languageCodes.includes(value) ? value : 'en';
}
