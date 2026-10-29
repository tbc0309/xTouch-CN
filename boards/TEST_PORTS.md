# 新主板测试目标

这些目标使用与主线相同的原生 ESP-IDF、LVGL、网络协议和界面实现，以 `1.0.1-test` 预发布固件提供。编译通过表示生成了固件并通过构建审计，不能代替屏幕、触摸、声音、联网和待机的实物验收。现有四款正式主板继续使用正式版固件。

## 分类

| 主控 | 屏幕接口 | 新增构建目标数 |
| --- | --- | ---: |
| ESP32-S3 | SPI | 9 |
| ESP32-S3 | QSPI | 1 |
| ESP32-S3 | RGB | 3 |
| ESP32-P4 | SPI | 2 |
| ESP32-P4 | MIPI DSI | 14 |

板级引脚和控制器参数集中在各目录的 `profile.cpp`，共用驱动位于 `boards/common/port_*.cpp`。独立的电源芯片、音频芯片、扩展芯片及面板初始化表仍按硬件区分；屏幕尺寸相同不表示固件可以互刷。目标身份和打包参数由 `boards/port_targets.json` 管理。

新增目标按 S3、P4 分别共用一份依赖锁，存放在 `boards/port-project-s3/` 和 `boards/port-project-p4/`。单板构建与批量构建使用同一组锁定的组件版本。

## 构建

激活 ESP-IDF 6.1 环境后，在工程根目录执行：

```sh
python boards/build.py lcdwiki-es3c28p --jobs 4
```

可通过 `--assets small` 或 `--assets large` 选择资源规格。默认按目标分辨率选择资源；该选项不会改变显示引脚、控制器或分辨率。应用、完整刷机包、摘要和清单位于对应 `boards/build-<目标>/artifacts/`。

每个新目标使用独立的应用项目身份、固件文件名及 OTA 清单路径。P4 的早期芯片和 v3.x 芯片分别构建，不能混用。实物 Flash 容量、PSRAM 类型、显示模组及接线必须与构建参数一致；未核对的硬件参数不作为已确认规格。

普通 BOOT 按键、联网待机与深睡唤醒分别配置。BOOT 位于非 RTC 引脚、或引脚已经被显示/音频占用时，不启用深睡模块；不能将普通按键可读等同于能够从深睡唤醒。支持的目标在进入深睡前检查并设置唤醒源，失败时保留运行状态。

SD 接口尚未映射的目标明确关闭 SD，避免沿用其它主板的引脚。它们仍可使用网页上传或网络 OTA；SD 更新及卡上文件功能需要完成对应引脚配置和实物验收。没有 RGB 灯，或仅有普通单色灯的目标，关闭当前 RGB 状态灯模块；不能因共用驱动存在就视为支持完整状态配色。

## 候选配置预检

候选板资料在 `boards/candidate_targets.json`，逐板说明在 `boards/candidates/`。
原候选 15 项继续保留资料记录；确认无坐标触摸屏的型号从构建目录中排除；明确配置触摸的屏幕组合保留，屏幕是否另购不影响筛选，排除依据见 `target_exclusions.json`。明确的触摸型号与无触摸型号不能混用。准确触摸配置仍待核实的目标不计入已支持列表。

当前进入原生编译的候选为 BOX、BOX-3、Tab5 两种芯片修订及 ESP32-Touch-LCD-3.5，以及 Korvo-2 两个变体、CAM 2.8/3.5 两个屏幕组合，共 9 个目标。候选通过构建审计后仍需实物验收，不自动成为正式发布目标。候选固件需要显式指定 `--allow-candidate-build`，未声明该选项时不会启动编译。

```sh
python boards/check_candidates.py
python boards/build.py espressif-s3-box --allow-candidate-build
```

预检只检查资料、构建输入和目标对应关系，不调用 IDF 编译。实际固件编译另行执行，不能将预检通过当成编译通过。被排除的型号即使指定候选开关也不可构建。

## 测试分支 OTA

`resources/ota_channels.json` 管理通道。测试分支按本板身份探测 `main` 和 `test` 清单，校验文件名、文件大小、MD5、版本和附件标签后选择较新版本；同一数字版本的正式版高于 `-test` 预发布版，相同版本优先 `main`。缺失或无效的清单不会转用其它主板固件。

固件描述、屏幕版本、网页版本、OTA 清单和 GitHub 发布标签统一为 `1.0.1-test`。OTA 附件使用 `releases/download/1.0.1-test/<主板固件>.bin`；清单仍来自对应主板的 `main`／`test` 分支路径。预发布与正式版独立存放。

主线禁止携带测试通道。合并准备工具先移除测试通道，再进行构建与发布检查：

```sh
python scripts/prepare_main_merge.py
python scripts/prepare_main_merge.py --apply
```

第一条只检查，第二条在 `test` 分支应用修改：移除测试 OTA 地址及版本后缀，并使旧测试清单失效。随后须重新构建并生成正式版清单。

主线有修复后，先提交主线修复；保持测试工作树干净，再在 `test` 执行：

```sh
python scripts/sync_main_to_test.py
```

该工具合并本地 `main`，不抓取远端、不推送。合并后重新构建受影响目标并补对应回归；不能用重置工作树代替同步而丢失新板实现。
