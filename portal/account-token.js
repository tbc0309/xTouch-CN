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
