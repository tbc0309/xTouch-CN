# 32×32 像素屏画面

[简体中文](PIXEL_DISPLAYS.md) · [English](PIXEL_DISPLAYS.en.md)

两种布局仅切换打印中的主画面，准备、换料及结果状态共用。底部显示打印进度和剩余时间，模型随层数增长。

## 打印中显示方式

|  |  |
|:---:|:---:|
| **彩色打印机**<br><img src="images/pixel-screen/printing.gif" width="192" height="192" alt="彩色打印机"> | **进度视图**<br><img src="images/pixel-screen/progress-layout.gif" width="192" height="192" alt="进度视图"> |

进度视图：已完成部分彩色向右流动，未完成部分为当前耗材颜色。

## 准备阶段

|  |  |  |
|:---:|:---:|:---:|
| **准备等待**<br><img src="images/pixel-screen/preparing-unknown.gif" width="160" height="160" alt="准备等待"> | **加热**<br><img src="images/pixel-screen/heating.gif" width="160" height="160" alt="加热"> | **回零**<br><img src="images/pixel-screen/homing.gif" width="160" height="160" alt="回零"> |
| **调平**<br><img src="images/pixel-screen/leveling.gif" width="160" height="160" alt="调平"> | **清洁喷嘴**<br><img src="images/pixel-screen/cleaning.gif" width="160" height="160" alt="清洁喷嘴"> | **进退料**<br><img src="images/pixel-screen/loading.gif" width="160" height="160" alt="进退料"> |
| **校准**<br><img src="images/pixel-screen/calibration.gif" width="160" height="160" alt="校准"> | **检查**<br><img src="images/pixel-screen/checking.gif" width="160" height="160" alt="检查"> | **腔温加热**<br><img src="images/pixel-screen/chamber-heating.gif" width="160" height="160" alt="腔温加热"> |

## 打印与耗材

|  |  |  |
|:---:|:---:|:---:|
| **首层打印**<br><img src="images/pixel-screen/first-layer.gif" width="160" height="160" alt="首层打印"> | **打印中换料**<br><img src="images/pixel-screen/changing-filament.gif" width="160" height="160" alt="打印中换料"> | **收尾等待**<br><img src="images/pixel-screen/finishing.gif" width="160" height="160" alt="收尾等待"> |
| **最后阶段**<br><img src="images/pixel-screen/printing-final.gif" width="160" height="160" alt="最后阶段"> | **打印中腔温加热**<br><img src="images/pixel-screen/printing-chamber-heating.gif" width="160" height="160" alt="打印中腔温加热"> | **黑色耗材**<br><img src="images/pixel-screen/black-filament.gif" width="160" height="160" alt="黑色耗材"> |
| **白色耗材**<br><img src="images/pixel-screen/white-filament.gif" width="160" height="160" alt="白色耗材"> | **未知耗材颜色**<br><img src="images/pixel-screen/unknown-filament.gif" width="160" height="160" alt="未知耗材颜色"> |  |

## 暂停与结果

|  |  |  |
|:---:|:---:|:---:|
| **暂停**<br><img src="images/pixel-screen/paused.gif" width="160" height="160" alt="暂停"> | **完成**<br><img src="images/pixel-screen/complete.gif" width="160" height="160" alt="完成"> | **取消**<br><img src="images/pixel-screen/cancelled.gif" width="160" height="160" alt="取消"> |
| **故障**<br><img src="images/pixel-screen/error.gif" width="160" height="160" alt="故障"> | **离线**<br><img src="images/pixel-screen/offline.gif" width="160" height="160" alt="离线"> |  |

## 待机

|  |  |  |
|:---:|:---:|:---:|
| **待机时钟预览**<br><img src="images/pixel-screen/idle-clock.gif" width="160" height="160" alt="待机时钟预览"> | **等待校时**<br><img src="images/pixel-screen/waiting-clock.gif" width="160" height="160" alt="等待校时"> | **温湿度**<br><img src="images/pixel-screen/environment.gif" width="160" height="160" alt="温湿度"> |

## 长参数滚动

|  |  |  |
|:---:|:---:|:---:|
| **长层数滚动**<br><img src="images/pixel-screen/scroll-print-layers.gif" width="160" height="160" alt="长层数滚动"> | **准备阶段长参数**<br><img src="images/pixel-screen/scroll-prep-layers.gif" width="160" height="160" alt="准备阶段长参数"> | **超长剩余时间**<br><img src="images/pixel-screen/scroll-print-time.gif" width="160" height="160" alt="超长剩余时间"> |

实际待机使用 iDotMatrix 内置时钟，字形以设备为准。勾选待机温湿度后，有有效环境读数时每 10 秒交替；无有效读数时保留时钟。

[返回项目首页](../README.md)
