# TargetCarrier 靶车工程

本工程包含 X 轴、靶车旋转机构、ESP32 无线通信和电脑上位机的源码与相关资料。


| 文件夹 | 作用 |
| `00_工程导航与汇总` | 工程结构、历史说明和录波相关资料 |
| `01_TargetCarrier -geek-UART` | ESP32 与 STM32 使用 UART 的工程 |
| `02_TargetCarrier -geek-I2C` | ESP32 与 STM32 使用 I²C 的工程 |
| `03_TargetCarrier -geek-SPI` | ESP32 与 STM32 使用 SPI 的工程 |
| `05_上位机` | 各版本上位机程序、源码和运行日志 |

各通信工程内的 `x轴`、`靶车` 和 `ESP32-S3-GEEK` 分别对应 X 轴控制、靶车控制和 GEEK 总机。
I²C、SPI 工程中的 `wired_transport` 保存板间通信适配源码。

## 工程树状图


TargetCarrier -geek
├─ 00_工程导航与汇总       工程结构与历史资料
├─ 01_TargetCarrier -geek-UART
│  ├─ x轴                  X 轴控制工程
│  ├─ 靶车                 靶车控制工程
│  └─ ESP32-S3-GEEK        GEEK 总机工程
├─ 02_TargetCarrier -geek-I2C
│  ├─ x轴 / 靶车           两套执行端工程
│  ├─ ESP32-S3-GEEK        GEEK 总机工程
│  ├─ wired_transport      I²C 通信适配源码
│  └─ 任务                 接线、烧录与验证记录
├─ 03_TargetCarrier -geek-SPI
│  ├─ x轴 / 靶车           两套执行端工程
│  ├─ ESP32-S3-GEEK        GEEK 总机工程
│  └─ wired_transport      SPI 通信适配源码
├─ 05_上位机
│  └─ 项目资料/源码        上位机源码
└─ README.md
```

## 使用上位机

在 `05_上位机/项目资料/源码/900度每秒版` 中执行 `build-unified.ps1`，构建 Windows 上位机。打包后应保留程序和同目录的运行库。

工程内的部分历史文档仍使用整理前的目录名称，查找时请以当前文件夹为准。

不同分支的固件、接线和上位机版本应配套使用。
