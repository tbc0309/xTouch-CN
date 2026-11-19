# 3D 打印外壳

简体中文 | [English](README.en.md) | [项目首页](../README.md)

下载对应压缩包并解压，在切片软件中打开 STL。外壳按完整主板型号设计，不能只按屏幕尺寸选择。

| 主板 | 下载 | 可选结构 |
|---|---|---|
| Waveshare ESP32-S3-Touch-LCD-2.8 V2 | [S3 外壳](case_esp32-s3-touch-lcd-2.8-v2.zip) | `1mm`、`2mm`、`2mm+speaker`；每组含三个按钮及 `case_tf`、`case_tf_usb` 壳体 |
| Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | [P4 外壳](case_esp32-p4-wifi6-touch-lcd-4.3.zip) | 无 40-pin 开口、带扬声器、带 40-pin 开口及两者兼有的四种版本；每组含壳体与三个按钮 |

CYD 与 LCDWIKI ES3C35P 暂无随项目提供的外壳模型。S3 外壳不适用于 ES3C35P。

先核对 USB、TF 卡、排针和扬声器的开口位置，再选择对应模型。打印方向、支撑、层高和材料请按所选结构及打印机调整；未提供统一切片参数或预切片任务。装配前断开电源，保留接口、按钮与散热空间，避免螺丝或壳体接触裸露电路。
