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
function accountLoginFields(body, fields, a) {
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
}
