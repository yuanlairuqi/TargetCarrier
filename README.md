# TargetCarrier geek7.0

**Windows 上位机 7.0 · [从 GitHub Release 下载安装程序](https://github.com/yuanlairuqi/TargetCarrier/releases/download/v7.0.0/TargetCarrier-Setup.exe) · [仓库安装程序](05_windows/TargetCarrier-Setup.exe)**

从 **Code → Download ZIP** 下载整套工程后，完整解压并双击 `05_windows/TargetCarrier-Setup.exe`。安装面向当前用户，默认路径为 `%LOCALAPPDATA%\Programs\TargetCarrier`。在开始菜单的 **TargetCarrier** 文件夹中打开主控制台或 TC/2 调参工具。

TargetCarrier 统一控制 X 轴和靶车旋转机构，提供装甲灯控、并行程序和 SD 录波。**日常使用推荐 I²C 版。**

| 目录 | 作用 |
| --- | --- |
| [工程导航](00_工程导航与汇总/01_当前工程导航与汇总.md) | 查看完整目录和模块入口 |
| [I²C 工程](<02_TargetCarrier -geek-I2C/README.md>) | 推荐使用，内含 X 轴、靶车和 GEEK 总机 |
| [UART 工程](<01_TargetCarrier -geek-UART/README.md>)、[SPI 工程](<03_TargetCarrier -geek-SPI/README.md>) | 用于对应板间接口的设备 |
| [机械结构与图纸](04_机械结构与图纸/README.md) | 靶车 SolidWorks 总装、零件模型与开源技术文档 |
| [Windows 上位机](05_windows/windows-guide.md) | 控制台、源码和通用调参 |

## 上位机简明操作手册

1. **启动和连接**　安装后从开始菜单打开 TargetCarrier 7.0。按 [I²C 工程](<02_TargetCarrier -geek-I2C/README.md>) 核对接线及 [配套固件](<02_TargetCarrier -geek-I2C/当前交付.md>)。在控制页搜索 BLE 设备，或选择 GEEK 的 USB 端口。到「设备总览」查看连接状态。
2. **操作 X 轴**　进入「X 轴控制」，确认停止且限位已释放。设置速度和加减速度，选择点动、持续正反转或交替运行。普通停机可用「减速停止」。
3. **操作靶车**　进入「靶车控制」，确认处于运动模式。读取电机配置，在失能状态应用增益，再点击「解除急停」「使能靶车」。收到使能反馈后，设置偏航速度并点击「连续旋转 / 更新转速」。
4. **操作灯光**　在侧栏切换「灯控模式」，等待靶车停止并失能，再选择颜色或红蓝循环。切回运动模式后，重新手动使能才可运动。
5. **停止设备**　按 `Esc` 或点击「全部停止」，查看两台设备各自的停止反馈。

并行程序、SD 录波和连接诊断见 [上位机使用说明](05_windows/project/user-guide.md)。

## Windows 安装与日志

安装包自带 Python 运行组件和 TC/2。Windows 缺少 WebView2 时，安装程序通过微软官方 Evergreen Bootstrapper 联网安装。运行日志保存在 `%LOCALAPPDATA%\TargetCarrier\logs`。安装包为未签名构建，SmartScreen 提示取决于本机策略。

原 X4000 程序在 `05_windows/project/programs/legacy-x4000`，用于历史版本维护。[安装包构建说明](05_windows/README.md)

## 固件与配对

公开交付包含 STM32 固件、ESP 引导程序和分区表、Android 安装包及各模块源码。ESP 应用请按对应分支的构建说明生成，并使用自己的设备配对参数。
