# STM32F103 标准外设库（SPL）基础外设工程

> 从一片 ST 官方示例工程开始，逐个把基础外设跑通，**并记录每一次卡住的原因和定位过程**。
>
> | 项 | 值 |
> |---|---|
> | **芯片** | STM32F103C8T6（**中容量 MD**，64KB Flash / 20KB SRAM） |
> | **开发板** | DShanMCU-F103（百问网"瑞士军刀"板） |
> | **工具链** | Keil MDK-ARM + **ARM Compiler 5** + ST 标准外设库包 `STM32F10x_StdPeriph_Lib_V3.6.0` |
> | **调试器** | ST-Link（SWD） |
> | **外部测量** | 逻辑分析仪（PulseView）+ 串口助手 |
> | ⚠️ | **不使用 STM32CubeMX / HAL** —— 全程手写或改官方库的初始化代码 |

---

## 仓库结构（ST 官方包的布局）

这个仓库的根目录就是 **ST 官方外设库包的根目录** —— 因为 `Project.uvprojx` 里的
源文件路径是 `..\..\..\Libraries\` 和 `..\..\..\Utilities\`（**三层相对路径**），
**只有把仓库根放在这里，clone 下来才能直接编译。**

```
（仓库根）
├── Libraries/                      ← ST 官方库：CMSIS + 标准外设驱动
│   ├── CMSIS/CM3/…                  （内核支持 + 启动文件）
│   └── STM32F10x_StdPeriph_Driver/  （外设驱动源码）
├── Utilities/STM32_EVAL/           ← 评估板支持文件（工程仍引用，见下方说明）
├── Project/STM32F10x_StdPeriph_Template/   ⭐ 我的工程在这里
│   ├── main.c                      ⭐ 我写的（GPIO / 定时器 / 串口 / 时钟自检）
│   ├── stm32f10x_it.c              ⭐ 我写的（TIM3 中断服务函数）
│   ├── stm32f10x_conf.h            （库配置，原版）
│   ├── system_stm32f10x.c          （原版）
│   └── MDK-ARM/
│       └── Project.uvprojx         ⭐ Keil 工程文件（用这个打开）
├── Package_license.md              ← ST 的许可证（再分发必须保留）
└── README.md                       ← 本文件
```

### ⭐ 怎么编译 / 烧录

1. 用 **Keil µVision** 打开 `Project/STM32F10x_StdPeriph_Template/MDK-ARM/Project.uvprojx`
2. ⚠️ 确认 `Options for Target → Target` 里的 **ARM Compiler 选 Version 5**（见踩坑 002）
3. ⚠️ 确认 `Options for Target → C/C++ → Define` 是 `USE_STDPERIPH_DRIVER, STM32F10X_MD`（**只留一个器件宏**，见踩坑 001）
4. Rebuild → Download（ST-Link，SWD：SWDIO / SWCLK / GND / 3V3）

> ⚠️ **工程里保留了 ST 官方示例的 6 个 Target**（`STM32100E-EVAL`、`STM3210E-EVAL`、`STM3210B-EVAL` 等），
> **只有 `STM32100E-EVAL` 这个被改成了 F103C8**（就是我在用的那个）。
> **Target 名字沿用 ST 评估板的名字是历史遗留**，不影响编译。
> ⚠️ 另外工程树里仍能看到 `Utilities/STM32_EVAL/` 的文件 —— 它们属于**其他 Target**，
> 我的 Target 不编译它们（这就是 `Utilities/` 不能删的原因）。

---

## 已完成的外设

| # | 外设 | 做了什么 | 状态 |
|---|---|---|---|
| 1 | **GPIO** | 直接操作 `BSRR` / `BRR` 寄存器翻转 LED 电平（不走库函数包装） | ✅ |
| 2 | **RCC · 时钟树** | 把工程从 **24MHz 修到 72MHz**，串口波特率随之从 4800 修正到 **9600** | ✅ 见下方 |
| 3 | **TIM + NVIC** | TIM3 定时中断驱动 LED，`while(1)` 里全空 | ✅ |
| 4 | **USART** | `USART2` 收发，用 printf 打印时钟自检信息 | ✅ |
| 5 | **逻辑分析仪** | PulseView 的 UART 解码器抓 TX 线，**实测解出 9600 / 8 数据位 / 1 停止位 / 无校验（8N1）** | ✅ |

> ⭐ **第 4、5 条的价值不止"跑通"**：串口自检代码是发现下面那个 bug 的**探测器**，
> 逻辑分析仪则是它的**外部确认**。

---

## ⭐ 最有价值的一条：一个追到寄存器层的时钟 Bug

### 症状

串口通信本身正常，但**波特率正好慢一半** —— 代码里写 9600，串口助手必须设 **4800** 才能读到正确字符。

### 第一步：先发现"库自己矛盾"

用一段自检代码，把库对同一个时钟给出的两个答案并排打出来：

```c
#include "stm32f10x_rcc.h"

RCC_ClocksTypeDef c;
RCC_GetClocksFreq(&c);
printf("SystemCoreClock=%u  RCC_GetClocksFreq.SYSCLK=%u  PCLK1=%u  "
       "CFGR=0x%08lX  CFGR2=0x%08lX  BRR=0x%04X\r\n",
       (unsigned)SystemCoreClock, (unsigned)c.SYSCLK_Frequency, (unsigned)c.PCLK1_Frequency,
       (unsigned long)RCC->CFGR, (unsigned long)RCC->CFGR2, (unsigned)USART2->BRR);
```

输出：

```
SystemCoreClock=24000000       RCC_GetClocksFreq.SYSCLK=48000000
PCLK1=48000000                 CFGR=0x0013000A
CFGR2=0x00000000               BRR=0x1388
```

⭐ **一个库，对同一个时钟给了两个答案 —— 这时不需要查任何外部资料，库自己就证明了其中一个是错的。**

### 第二步：对比"写分频"和"读分频"的路径

| 角色 | 位置 | 用哪个寄存器 | 含义 |
|---|---|---|---|
| **写** | `system_stm32f10x.c:645`（`SetSysClockTo24` 值线分支） | **`CFGR` bit17** = `RCC_CFGR_PLLXTPRE_PREDIV1_Div2` | PREDIV1 = **÷2** |
| **读** | `stm32f10x_rcc.c:947`（`RCC_GetClocksFreq`） | **`CFGR2` [3:0]** = `CFGR2_PREDIV1` | PREDIV1 = ÷(1..16) |
| **读** | `system_stm32f10x.c:345`（`SystemCoreClockUpdate`） | 同上 | **同一个 bug 的第二份拷贝** |

**而 STM32F103C8T6 上根本没有 `CFGR2` 这个寄存器** —— 读回来永远是 0。

### 第三步：逐位解码 `CFGR`，确认硬件究竟做了什么

`CFGR = 0x0013000A`：

| 位 | 名字 | 值 | 含义 |
|---|---|---|---|
| [1:0] | **SW** | `10` | 选 PLL 作系统时钟源 |
| [3:2] | **SWS** | `10` | ⭐ **硬件回报：现在实际跑的就是 PLL** |
| [10:8] | **PPRE1** | `000` | APB1 ÷1 → PCLK1 = SYSCLK |
| 16 | **PLLSRC** | `1` | HSE 进 PLL |
| **17** | **PLLXTPRE** | **`1`** | ⭐⭐ **HSE 先 ÷2 —— 硬件真的除 2 了** |
| [21:18] | **PLLMULL** | `0100` = 4 | 倍数 = 4 + 2 = **×6** |

→ **真实 SYSCLK = 8MHz ÷ 2 × 6 = 24MHz** ✅（与 `SystemCoreClock` 完全一致）

**而库去读 `CFGR2`（= 0）** → 以为 `PREDIV1 = ÷1` → 算出 `8MHz × 6 = 48MHz` ❌

→ ⭐ **库错、硬件对。板子一直跑在合规的 24MHz 上，是库把频率算错了一倍。**

### 为什么官方库会这么写（不是抄错）

- **值线（VL = Value Line）** 器件的合法用法是"HSE 先除 N 再乘 M"，**N ∈ [1, 16]**
  → 只能用 **4 位**的 `CFGR2.PREDIV1` 来表达
- 而 `CFGR` bit17 只有 **1 位**，只能表达 ÷1 / ÷2
- 所以 ST 为了让函数能算出任意频率，**写用 `CFGR.bit17`、读用 `CFGR2`** ——
  **它赌的是"值线上这两个位是同一个位的别名，硬件会自动同步"**

⭐ 这个赌注在 **"器件宏 = 真实芯片"** 时成立；**器件宏选错时就不成立**。

**根本原因**：工程用的是值线宏 `STM32F10X_HD_VL`，而芯片是 **F103C8T6（中容量 `STM32F10X_MD`）**。

### 修复

把器件宏切到真实档位（连带启动文件、Device、评估板文件、`main.c` 里的评测板分支）：

```
Define   : USE_STDPERIPH_DRIVER, STM32F10X_MD      ← 替换，不是追加
启动文件 : startup_stm32f10x_md.s（旧的 hd_vl.s 取消 Include in Target Build）
Device   : STM32F103C8
移除文件 : stm32_eval.c / stm32100e_eval_*.c 等评估板支持文件
main.c   : 注释掉 #include "stm32_eval.h" 和 #ifdef USE_STM32xx_EVAL 相关调用
```

**修复后实测**：

```
SystemCoreClock=72000000     RCC_GetClocksFreq.SYSCLK=72000000
PCLK1=36000000               CFGR=0x001D040A     BRR=0x0EA6 (=3750)
```

| 项 | 修复前 | 修复后 |
|---|---|---|
| SYSCLK | 24MHz | **72MHz** |
| CFGR **bit17**（PLLXTPRE） | `1`（÷2） | **`0`（÷1）** |
| CFGR **[21:18]**（PLLMULL） | `4`（×6） | **`7`（×9）** |
| BRR | 5000 → 实际 **4800bps** | **3750 → 真 9600bps** |

⭐ **修复前后的每一个值，都是先预测、再上板实测确认的** —— 六个值全部命中。

### ⭐⭐ 最后一步：用逻辑分析仪做「外部确认」

**寄存器读数是"内部视角"（软件说的），逻辑分析仪是"外部视角"（硬件真实行为）。**

| 视角 | 依据 | 结论 |
|---|---|---|
| **内部**（读寄存器） | `BRR = 0x0EA6 = 3750` → `36000000 / 3750 = 9600` | PCLK1 = 36MHz → SYSCLK = 72MHz |
| **外部**（逻辑分析仪） | PulseView 的 UART 解码器在 **9600** 下解出**正确帧格式**（8N1） | **硬件真的在 9600** |

⭐ **逻辑分析仪不信任何寄存器，只看线上的电平变化 —— 两个独立视角给出同一个答案，才算真的确认。**

**修复前后三个数字的变化（注意倍数各不相同）：**

| | 修复前 | 修复后 | 倍数 |
|---|---|---|---|
| PCLK1（硬件实际） | 24MHz | 36MHz | 1.5× |
| BRR | 5000 | 3750 | 0.75× |
| **实际波特率** | **4800** | **9600** | **2×** |

**验算：`1.5 ÷ 0.75 = 2`** ✅ —— "波特率正好差一倍"这个症状，从头到尾都对得上。

### 从这条 bug 里得到的经验

1. ⭐⭐ **当"库算出来的值"和"库自己的常量"打架时，先怀疑那个「读寄存器反推」的函数，不要怀疑常量。**
2. ⭐ **症状"正好差一倍 / 一半"时，优先查分频器，不要先查晶振。**
3. ⭐ **同一个功能有两个名字、两个位置（别名寄存器）时，"谁写谁读"必须对齐** ——
   写的用 A、读的用 B，只有在**硬件保证两者同步**时才成立；芯片不对，这个保证就没了。
4. ⭐ **库的 `@note` 要读**：`stm32f10x_rcc.c:903` 明确写着这个反推函数**有前提条件**。
5. ⭐ **一个库给两个答案时，不用查外部资料** —— 库自己就已经证明其中一个是错的。

---

## 踩坑清单

| # | 坑 | 根因 | 一句话教训 |
|---|---|---|---|
| 001 | 编译官方库文件报 **36 个错** | 工程里**同时存在两个器件宏** | 器件宏是**替换**，不是追加；报错里那个枚举成员名能反推走了哪个分支 |
| 002 | 编译一片红（另一类） | **ARM Compiler 6** 编不了 SPL | ST 的 SPL 必须用 **ARM Compiler 5** |
| 003 | `TIM_TimeBaseInit()` 执行完就直接进中断 | 它最后往 `EGR` 写一位、**主动产生一次更新事件** | `TIM_TimeBaseInit` 之后、开中断之前，先 `TIM_ClearFlag(x, TIM_FLAG_Update)` |
| 004 | 中断里两个断点"有时候不跳" | 一行 C 代码对应**多条汇编指令**，而 Keil 断点按**地址**下 | 打开 **Disassembly** 窗口看 PC 在哪；或用单步 F10/F11 |
| 005 | 波特率正好慢一半 | 写/读分频用了两个寄存器，而芯片上没有后者 | 见上面那一整节 |
| 006 | 改主频后定时器周期变了 3 倍 | APB 预分频 ≠1 时，**定时器时钟 = PCLK × 2** | 改主频后要重算所有"按旧主频标定"的东西 |

---

## 环境配置速查

| 项 | 值 |
|---|---|
| IDE | Keil MDK-ARM（µVision） |
| **编译器** | ⚠️ **ARM Compiler 5**（`Options → Target → ARM Compiler`） |
| 库 | ST 标准外设库包 `STM32F10x_StdPeriph_Lib_V3.6.0` |
| 器件宏 | `USE_STDPERIPH_DRIVER, STM32F10X_MD`（**只留一个**） |
| 芯片 | STM32F103C8T6 |
| 调试器 | ST-Link V2（SWD：SWDIO / SWCLK / GND / 3V3） |
| 串口 | **9600 8N1**（改主频后请重新核对 BRR） |
| 逻辑分析仪 | 8 通道 + PulseView（UART 解码器：9600 / 8 / 1 / None） |

---

## 启动流程（读代码笔记）

```
上电复位
  → 硬件从向量表取 MSP（0x00000000）和 PC（0x00000004）
  → Reset_Handler            （startup_stm32f10x_md.s 里的第一段汇编）
      → SystemInit()          // 厂商提供，配置系统时钟
      → __main                // ⭐ C 库入口，不是你的 main
          → 初始化 .data / .bss
          → main()            // 你写的代码
```

⭐ **关键词：`__main` ≠ `main`** —— 前者是 C 库入口（负责 `.data` 拷贝、`.bss` 清零、堆栈初始化），
**最后**才调用你写的 `main()`。

---

## 后续计划

- ⬜ **TIM3 在 72MHz 下重新标定**（`PSC = 7199`、`ARR = 9999` → 1s）
- ⬜ 外部中断 EXTI / ADC / 看门狗
- ⬜ FreeRTOS 移植（下一步主线）

---

## 许可证

- **ST 的库代码**（`Libraries/`、`Utilities/`）版权归 STMicroelectronics，
  按 **MCD-ST Liberty SW License Agreement V2** 分发 —— 见 `Package_license.md` 与
  `Project/STM32F10x_StdPeriph_Template/LICENSE.txt`。
- **本仓库中我自己写的部分**（`main.c` / `stm32f10x_it.c` 的修改、`README.md`）可自由参考。

---

## 关于这份记录

所有"踩坑"条目都来自实际调试过程，格式统一为：
**现象 → 当时以为 → 实际原因 → 怎么定位到的 → 怎么解决 → 可迁移的规律**。

⭐ 其中 **005** 是先提出预测、再上板实测验证的 —— 所以它的每个数字都可以被复核。
⭐ 并且它有**两个独立视角**的确认：**寄存器读数（内部）＋ 逻辑分析仪（外部）**。
