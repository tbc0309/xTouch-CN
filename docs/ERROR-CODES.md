# 故障码 / Error codes

屏幕保留打印机上报的原始代码；S3／P4／ES3C35P 另带英文说明，CYD 仅显示代码。文档保留中英完整资料，未收录的新代码仍可显示。
Original codes, including unknown ones, remain visible. S3/P4/ES3C35P include English descriptions; CYD shows codes only. Documentation retains the bilingual catalog.

HMS 是健康监测报告，包含警告与故障，不需要等打印错误出现。HMS 与当前打印错误在同一个弹窗汇总；先后上报会更新列表，相同代码不反复弹出。具体触发、确认及清除规则见[提示与故障弹窗](USER-GUIDE.md#notifications)。
HMS includes warnings and faults independently of print errors. Both share one dialog; repeated codes do not repeatedly notify. See [Notifications](USER-GUIDE.en.md#notifications).

| 类型 / Type | 格式 / Format | 对照 / Reference |
| --- | --- | --- |
| HMS 健康状态 | `HMS 0500-0500-0001-000F` | [3,267 条 / entries](HMS-CODES.md) |
| 打印错误 / Print error | `0x07018004` | [1,117 条 / entries](PRINT-ERROR-CODES.md) |

在对应文档中搜索屏幕代码。同码随机型变化的说明保存在原始数据的 `variants` 中；缺少中文时保留英文，不推测翻译。
Search by the displayed code. Model-specific descriptions are retained in `variants`; English is kept when Chinese text is unavailable.

**[在线故障码查询 / Online code lookup](https://tbc0309.github.io/xTouch-CN/errors.html)**：支持前缀自动匹配、HMS 格式、十六进制和十进制命令错误码。

## 数据与操作规则 / Data and actions

故障库覆盖十组设备的HMS、打印错误与操作规则。相同代码的机型差异保留为文案变体。
The catalog covers HMS, print errors and action rules for ten device groups. Model-specific descriptions remain separate variants.

重试与完成按钮按通用规则及设备序列号前缀匹配动作表。动作7对应完成，8／9对应恢复；未知规则只显示关闭。控制仍受打印机权限约束。
Retry and Done actions follow common rules and model-specific tables selected by serial-number prefix. Action 7 means Done; 8/9 mean Resume. Unknown rules show Close. Printer permissions still apply.

## 更新 / Update

```sh
python scripts/refresh_error_reference.py
python scripts/generate_error_docs.py
```

原始数据为 `docs/data/error-codes.json`，记录资料版本、设备来源及文案变体。更新失败不会覆盖原文件；普通编译不联网抓取故障库。
The JSON records device sources, data versions and description variants. Failed refreshes preserve the existing file. Normal builds do not download this database.
