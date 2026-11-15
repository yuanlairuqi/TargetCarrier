# TargetCarrier 靶车工程

本工程包含 X 轴、靶车旋转机构、ESP32 无线通信和电脑上位机的源码与相关资料。

| 文件夹 | 作用 |
| --- | --- |
| `00_工程导航与汇总` | 工程结构、历史说明和录波相关资料 |
| `01_TargetCarrier -geek-UART` | ESP32 与 STM32 使用 UART 的工程 |
| `02_TargetCarrier -geek-I2C` | ESP32 与 STM32 使用 I²C 的工程 |
| `03_TargetCarrier -geek-SPI` | ESP32 与 STM32 使用 SPI 的工程 |
| `05_上位机` | 各版本上位机程序、源码和运行日志 |

各通信工程内的 `x轴`、`靶车` 和 `ESP32-S3-GEEK` 分别对应 X 轴控制、靶车控制和 GEEK 总机。
I²C、SPI 工程中的 `wired_transport` 保存板间通信适配源码。

## 工程树状图

树状图包含本机交付目录。GitHub 提供源码和文档，图中的固件镜像、Windows 程序、快捷方式与现场日志随本地交付管理。

```text
TargetCarrier -geek6.9
├─ README.md
├─ 00_工程导航与汇总
│  ├─ 01_当前工程导航与汇总.md
│  └─ 录波源码和测试文件recorder_core
├─ 01_TargetCarrier -geek-UART
├─ 02_TargetCarrier -geek-I2C
│  ├─ 当前交付.md
│  ├─ ESP32-S3-GEEK
│  ├─ wired_transport
│  ├─ x轴
│  │  ├─ 野火stm                          STM32 源码
│  │  ├─ esp_无线从机_20261022             ESP 源码
│  │  └─ 交付文件
│  │     ├─ 当前版                         X4000，STM32 与 ESP
│  │     └─ 历史/原交付
│  └─ 靶车
│     ├─ firmware                          STM32 与 ESP 源码
│     └─ 交付文件
│        ├─ 当前版                         ±900°/s，STM32 与 ESP
│        └─ 历史/20261114-再使能修复版
├─ 03_TargetCarrier -geek-SPI
└─ 05_上位机
   ├─ 启动靶车上位机.lnk                   X4000 + ±900°/s
   ├─ 启动通用调参.lnk
   ├─ 00_上位机合集说明.md
   └─ 项目资料
      ├─ 程序
      │  ├─ 当前版
      │  └─ 历史
      │     ├─ 20261112-装甲板灯控版
      │     ├─ 20261113-侧栏灯控版
      │     ├─ 20261114-再使能修复版
      │     └─ 20261114-900度每秒版
      ├─ 源码
      │  ├─ 当前版
      │  └─ 历史
      ├─ 历史日志
      ├─ assets
      ├─ firmware
      └─ verification
```

## 使用上位机

使用[当前上位机源码](05_上位机/项目资料/源码/当前版/)中的 `build-unified.ps1` 构建 Windows 上位机。本机已部署交付时，可在 `05_上位机` 根目录双击「启动靶车上位机」。当前上位机支持 X 轴 4000 Hz 和靶车手动 ±900°/s。固件位置和历史版本见 [I²C 当前交付](02_TargetCarrier%20-geek-I2C/当前交付.md)。

工程内的部分历史文档仍使用整理前的目录名称，查找时请以当前文件夹为准。

不同分支的固件、接线和上位机版本应配套使用。
