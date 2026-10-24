# 文档目录

简体中文 | [English](README.en.md)

主板功能对比、界面截图和快速配置见[项目 README](../README.md)。下列指南提供详细说明。

## 使用与开发指南

| 指南 | 查阅内容 |
|---|---|
| [安装、升级与账号](INSTALLATION.md) | 刷入固件、首次配网、网页登录、局域网/拓竹账号、Token |
| [日常操作与故障提示](USER-GUIDE.md) | 按键与确认规则、提示类型、HMS 去重与关闭行为 |
| [打印机、AMS 与控制兼容](PRINTERS.md) | 型号识别、耗材与烘干、双喷头、云端权限及限制 |
| [温湿度计](THERMOMETERS.md) | 支持型号、MAC/BLE Key 绑定、显示模式与上报时间 |
| [主板与外设](HARDWARE.md) | 三板硬件要求、CYD 引脚、DS18B20 外接仓温 |
| [配置、备份与安全](MAINTENANCE.md) | SD 同步、配置导入、恢复、凭据与账号密钥 |
| [开发、构建与发布](DEVELOPMENT.md) | 源码构建、功能编译、字体资源、打包及网页部署 |

## 按需求查找

- [安装与更新](INSTALLATION.md#install)
- [添加打印机或导入 Token](INSTALLATION.md#accounts)
- [暂停、取消和故障确认](USER-GUIDE.md#notifications)
- [AMS、双喷头与烘干](PRINTERS.md#ams)
- [拓竹账号在线但命令被拒绝](PRINTERS.md#authorization)
- [备份、换卡或恢复配置](MAINTENANCE.md#storage)
- [适配新主板或选择编译模块](DEVELOPMENT.md#options)
- [打包发布及 OTA 清单](DEVELOPMENT.md#release)
- [温湿度计支持列表与绑定配置](THERMOMETERS.md)

## 故障码查询

[在线搜索](https://tbc0309.github.io/xTouch-CN/errors.html) · [代码格式与数据来源](ERROR-CODES.md) · [HMS 索引](HMS-CODES.md) · [打印错误索引](PRINT-ERROR-CODES.md)

故障码资料为中英对照。`errors/` 保存自动生成的查询页，`data/` 保存源数据，`images/` 保存正文引用的图片，`licenses/` 保存授权记录。

版本记录统一见 [GitHub Releases](https://github.com/tbc0309/xTouch-CN/releases)。
