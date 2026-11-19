# TC/2 通用通信

TC/2 供独立通信和调参。电脑通过 GEEK 和 ESP 接收端访问 STM32，板间接口由固件配置选择 UART、I²C 或 SPI。

`core/` 提供协议，`firmware/` 放板端工程，`pc/` 放调参工具。使用 [通用调参入口](../../05_windows/project/README-TC2.md) 连接配套固件。
