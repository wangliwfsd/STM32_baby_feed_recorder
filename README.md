# F746 TouchLog

基于 **STM32F746G-DISCO** 的触摸式奶量记录器。项目使用板载 4.3 英寸触摸屏显示和录入记录，通过 microSD + FatFs 持久化保存 CSV 数据，并支持板载以太网 DHCP/SNTP 自动校时。

## 主要功能

- 触摸按钮快速记录 `30 / 50 / 90 / 100 / 110 / 120 ml`
- 实时显示当前 RTC 时间
- 显示当天总奶量、次数和最近一次记录
- 最新记录显示在列表顶部
- 上下滑动浏览今日及历史记录
- 单条记录选择和删除
- 带二次确认的全部历史记录清除
- CSV 数据写入后立即同步到 SD 卡
- SD 写入失败时以橙色显示待保存记录，并每 0.5 秒自动恢复和重试
- 每条新记录带唯一编号，便于离线识别和检查记录
- 断电重启后从 SD 卡恢复记录
- RTC 有效时跳过时间设置；备份域失效时要求重新设置
- 板载 RJ45 + LwIP + DHCP + DNS + SNTP 网络校时
- 网络同步过程中可取消
- SD、网线、DHCP 和 IP 状态显示

## 硬件

- STM32F746G-DISCO / 32F746GDISCOVERY
- STM32F746NGH6 Cortex-M7
- 板载 480 × 272 TFT LCD 和电容触摸屏
- 板载 microSD 卡槽
- 板载 LAN8742A 10/100 Mbps Ethernet PHY 和 RJ45
- ST-LINK/V2-1

注意：开发板没有独立 RTC 备用电池，`VBAT` 与主电源相关。完全断电后 RTC 时间不能可靠保留；可通过开机手动设置或联网 SNTP 重新校时。

## 软件组件

- STM32 HAL
- FreeRTOS / CMSIS-RTOS v1
- FatFs
- LwIP
- DHCP、DNS、SNTP
- LAN8742 PHY 驱动
- STM32746G-Discovery BSP LCD/Touch/SDRAM 驱动

所需 LwIP、SNTP、LAN8742 和 `ethernetif` 源码已经放入工程，构建时不依赖外部 STM32Cube Repository，也不包含本机绝对路径。

## 构建要求

- CMake 3.22 或更高版本
- Ninja
- Arm GNU Toolchain，命令前缀为 `arm-none-eabi-`
- STM32CubeIDE 也可用于下载和调试

### Debug 构建

```sh
cmake --preset Debug
cmake --build --preset Debug
```

生成文件：

```text
build/Debug/F746_TouchLog.elf
```

### Release 构建

```sh
cmake --preset Release
cmake --build --preset Release
```

生成文件：

```text
build/Release/F746_TouchLog.elf
```

## 烧录

可使用 STM32CubeIDE、STM32CubeProgrammer 或 GDB/ST-LINK 下载生成的 ELF。

在 STM32CubeIDE 调试模式下，程序可能停在 `main()` 临时断点。此时点击 **Resume** 或按 `F8` 继续运行。

## 开机流程

1. 初始化 LCD 和触摸屏。
2. 检查 RTC 备份寄存器和时间有效性。
3. RTC 无效时显示时间设置页面。
4. 初始化并测试 SD 卡。
5. SD 正常时直接进入功能菜单；异常时先显示诊断页面。
6. 用户选择功能或进入主记录界面。

## 开机菜单

### START

进入奶量记录主界面。

### SET TIME

手动设置年、月、日、时、分、秒。保存后会写入 RTC 和备份有效标记。

### SYNC TIME

通过板载 RJ45 自动校时：

```text
LAN8742 → DHCP → DNS → pool.ntp.org → SNTP → UTC+8 → RTC
```

同步页面提供 `CANCEL` 按钮。状态可能包括：

- `NET: OFF`：网络尚未初始化
- `NET: NO CABLE`：没有检测到网线
- `NET: DHCP...`：正在获取地址
- `NET: 192.168.x.x`：已经获取 IP
- `TIME SYNCED`：同步成功
- `SYNC FAILED`：超时或服务器访问失败
- `SYNC CANCELLED`：用户取消

当前时区固定为 **Australia/Perth（UTC+8，无夏令时）**。

### CLEAR HISTORY

删除全部历史记录。必须在确认页面再次点击 `DELETE` 才会执行，随后 CSV 文件会被重建并立即同步。

## 主界面

左侧显示：

- 当前查看日期
- 日期累计奶量
- 记录次数
- 最近一次记录
- 今日或历史记录列表

右侧显示：

- 当前时间 `HH:MM:SS`
- SD 状态
- 待保存记录数 `P:n`
- 六个奶量快捷按钮
- 单条记录删除按钮

## 历史记录操作

- 默认页面为 `TODAY RECORDS`
- 最新记录显示在最上方
- 在左侧列表向上滑动查看更早记录
- 向下滑动返回较新记录
- 每页最多显示 8 条
- 内存中保留最近 256 条记录用于快速浏览
- SD 卡 CSV 文件保存完整记录，不受 256 条显示缓存限制
- 橙色并带 `*` 的记录表示仍在 RAM 中等待写入 SD
- `WAIT P:n` 表示 SD 故障：每 0.5 秒直接重试；检测到拔插卡或连续失败 3 次时执行完整恢复
- SD 恢复且记录确认写入后，该行自动恢复为正常颜色

### 删除单条记录

1. 点击左侧某条记录进行选择。
2. 选中行高亮，右下角 `DELETE` 按钮变红。
3. 点击 `DELETE`。
4. 程序生成临时 CSV，替换原文件，然后重新加载记录和摘要。

存在待保存记录或 SD 正在恢复时，删除按钮暂时禁用，避免与文件系统重新挂载并发。

## SD 卡与 FatFs

日志文件：

```text
MILKLOG.CSV
```

格式：

```csv
date,time,amount_ml,record_id
2026-07-12,08:15:23,90,0
2026-07-12,11:42:08,120,1
```

程序兼容原来的三列 CSV，并且不会为了修改表头而重写整份历史。旧文件保持三列表头，新追加的数据行可以带第四个 `record_id` 字段；旧记录读取时编号视为 `0`，新记录使用递增的非零编号。新建或清空的日志使用四列表头。

每次新增记录后执行：

1. 新记录立即压入 RAM FIFO 队列并显示为橙色
2. 独立存储线程按先进先出顺序每次取一条
3. 打开 CSV、定位到文件末尾、追加一行并关闭；`f_close()` 负责同步缓存
4. 成功后通知界面将记录恢复为正常颜色，再处理下一条
5. 失败的当前记录不丢弃，显示 `WAIT P:n`；每 0.5 秒直接重试，连续失败 3 次才重新初始化和挂载

相邻两条 SD 操作之间保留 500 ms 安静时间，避免部分 SD 卡刚完成内部编程时立即收到下一条命令。RAM FIFO 仍可继续接收按键记录，不会阻塞触摸界面。

单条记录的 `f_open`、`f_lseek`、`f_write`、`f_close` 步骤之间各保留 20 ms，让 SD 卡和 FatFs 缓存有稳定时间。所有延时都位于独立存储线程，不阻塞触摸和页面滚动。

拔卡后，失败记录继续保留在 FIFO 中。检测到卡重新插入后，程序先等待检测状态稳定 500 ms，再重新初始化 SDMMC、确认 HAL 已进入 `READY`、更新磁盘状态并挂载 FatFs；恢复完成后再等待 200 ms，才执行下一次 `f_open()`。如果卡检测开关漏掉拔卡事件，连续 3 次文件操作失败也会强制进入同一恢复流程。故障捕获只读取 HAL 状态和错误码，不再额外发送 CMD13。

存储线程使用 `osPriorityAboveNormal`。轮询式 SDMMC 传输期间不会被普通优先级的触摸和 LCD 任务抢占；步骤间的 `osDelay()` 仍会主动让出 CPU，因此界面保持响应。

`record_id` 仍写入 CSV，便于离线检查和后续数据处理。运行时不会为了去重额外读取 CSV，每次保存或重试都直接追加当前 FIFO 记录。

待保存队列最多 32 条，仅保存在 RAM 中。按照当前设计，SD 故障期间如果复位或完全断电，尚未写入 SD 的橙色记录会丢失。

### SDMMC 稳定性设置

当前配置采用：

- 1-bit SD 总线
- 约 8 MHz SD 时钟
- 阻塞式扇区读写

该配置为插卡座、走线和不同 SD 卡保留了更大的信号裕量。对于少量 CSV 日志，当前吞吐量完全足够。

### 本次 SD/FatFs 故障修复总结

旧版本在写入失败时可能直接丢弃记录，因此表面上很少停在错误状态，但记录本身并未得到可靠保存。加入待写 FIFO 后，问题变得可见，也暴露出以下几个因素：

- 阻塞式 SDMMC 轮询传输曾运行在较低优先级，触摸和 LCD 任务抢占后可能触发 `RX_OVERRUN`、`TX_UNDERRUN` 或命令超时
- 热插拔后 FatFs 和 SDMMC 可能仍保留旧状态，单纯反复调用 `f_open()` 无法恢复
- PC13 卡检测开关偶尔会漏掉一次拔卡过程，使程序没有进入重新初始化分支
- 调试阶段额外发送的 CMD13 会改变随后读到的 HAL 错误状态，掩盖最初失败原因

最终处理方式是：按键记录先进入 RAM FIFO；存储线程以较高优先级逐条写入；正常失败每 0.5 秒重试；检测到拔插卡，或连续 3 次文件操作失败时，强制卸载 FatFs、重新初始化 SDMMC、检查卡状态并重新挂载。恢复后等待 200 ms 再打开日志文件。这样即使卡检测漏报，待保存记录也能在重新插卡后自动补写。

## 网络实现

网络相关源码位于：

```text
Middlewares/Third_Party/LwIP/
Drivers/BSP/Components/lan8742/
Network/Target/ethernetif.c
```

主要配置：

```text
Core/Inc/lwipopts.h
Core/Inc/ethernetif.h
```

SNTP 使用 UTC Unix 时间戳，收到后转换为 UTC+8，再写入 RTC。网络初始化使用官方 STM32746G-Discovery Ethernet 接口实现。

## 目录结构

```text
Core/                         应用、外设和中断代码
FATFS/                        FatFs 应用和 SD 磁盘接口
BSP/                          LCD、触摸、SDRAM 和字体
Drivers/                      HAL、CMSIS 和 LAN8742
Middlewares/                  FreeRTOS、FatFs、USB Host、LwIP
Network/Target/               板级 Ethernet 网络接口
USB_HOST/                     USB Host
cmake/                        Arm GCC 和 CubeMX CMake 配置
build/                        构建输出
F746_TouchLog.ioc             STM32CubeMX 工程配置
STM32F746xx_FLASH.ld          GNU linker script
```

## 常见问题

### `FatFs: 1`

`1` 对应 `FR_DISK_ERR`，表示底层扇区读写失败。检查页面上的 `Stage` 和 `HAL error`。当前工程已使用稳定的 1-bit、8 MHz 阻塞式 SDMMC 配置。

### `HAL error: 0x00000004`

表示 SDMMC 命令响应超时。它可能发生在 `f_open()`、`f_close()` 内部访问文件系统扇区时，也可能来自状态查询，并不等同于 FatFs 挂载失败。当前程序连续失败 3 次后会自动执行完整的 SDMMC/FatFs 恢复流程。

### `HAL error: 0x00000010`

表示 SDMMC 发送欠载（`TX_UNDERRUN`）。不要随意恢复为 4-bit、24 MHz，除非已经验证信号完整性和 SD 卡兼容性。

### `HAL error: 0x00000020`

表示 SDMMC 接收溢出（`RX_OVERRUN`）。存储线程目前使用 `osPriorityAboveNormal`，避免普通优先级的界面任务在轮询式块传输期间抢占；运行时也不会为去重扫描 CSV，重试时只追加 FIFO 当前记录。

### CMD13 是什么

CMD13（`SEND_STATUS`）用于查询 SD 卡状态，`BSP_SD_GetCardState()` 会使用它。它不是 FatFs 的挂载、读取或写入操作，CMD13 成功也不能说明 `f_mount()` 已经成功。当前故障捕获不再为了诊断额外发送 CMD13，以免覆盖原始 HAL 错误码。

### 显示 `NET: NO CABLE`

- 检查 RJ45 网线
- 检查路由器端口指示灯
- 确认路由器端口已启用

### 长时间停在 DHCP

- 确认网络提供 DHCP
- 尝试更换路由器端口或网线
- 检查局域网是否限制未知 MAC 地址

### 获取 IP 但时间同步失败

- 检查网络是否能访问互联网
- 检查 DNS 和 UDP 123 端口是否被阻止
- 检查 `pool.ntp.org` 是否可解析

### 完全断电后要求设置时间

这是板卡硬件限制。板载 RTC 没有独立备用电池；可使用 SNTP、外接 DS3231，或增加 ESP32/手机同步方案。

## 已知事项

- RTC 时区目前固定为 Perth UTC+8。
- 网络校时需要支持 DHCP、DNS 和 UDP 123 的网络。
- LwIP、FreeRTOS 和显示功能会增加 RAM 占用，但当前仍有充足余量。
- 工程仍可能有 CubeMX 生成的未使用 RTC 变量和未调用 MPU 配置警告，不影响固件链接。

## 数据安全建议

- 写入过程中不要拔出 SD 卡。
- 确认记录已从橙色恢复为正常颜色、顶部不再显示 `WAIT` 或 `SAVE` 后再断电。
- 定期将 `MILKLOG.CSV` 复制到电脑备份。
- 清空全部历史记录不可撤销。
