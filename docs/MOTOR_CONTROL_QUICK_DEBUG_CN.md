# RX26T 电机不转一页纸速查（10 分钟版）

## 1. 先看这 15 个变量

### 1.1 模式/状态
- `com_u1_system_mode`：命令侧（STOP=0/RUN=1/ERROR=2/RESET=3）
- `g_u1_system_mode`：系统侧镜像
- `g_st_sensorless_vector.st_stm.u1_status`：实际状态（STOP=0/RUN=1/ERROR=2）
- `g_st_sensorless_vector.u2_error_status`：错误位

### 1.2 采样/控制主量
- `g_st_sensorless_vector.f4_vdc_ad`
- `g_st_sensorless_vector.f4_iu_ad`
- `g_st_sensorless_vector.f4_iv_ad`
- `g_st_sensorless_vector.f4_iw_ad`
- `g_st_sensorless_vector.st_speed_output.f4_ref_speed_rad_ctrl`
- `g_st_sensorless_vector.st_speed_output.f4_speed_rad_lpf`
- `g_st_sensorless_vector.st_current_output.f4_modu`
- `g_st_sensorless_vector.st_current_output.f4_modv`
- `g_st_sensorless_vector.st_current_output.f4_modw`

### 1.3 启动门槛标志
- `g_st_cc.u1_flag_offset_calc`
- `g_st_cc.u1_flag_charge_bootstrap`
- `g_st_cc.u1_active`
- `g_st_sc.u1_active`

## 2. 10 分钟排障路径

### 第 1 步（1 分钟）：命令是否真的进系统
- 写 RUN 后检查：
  - `com_u1_system_mode == 1`
  - `g_u1_system_mode == 1`
  - `g_st_sensorless_vector.st_stm.u1_status` 变成 `1`
- 若失败：先确认 `g_u1_sw_userif == MAIN_UI_RMW(0)`，再查 ICS 命令链。

### 第 2 步（1 分钟）：是否马上掉 ERROR
- 若 `st_stm.u1_status == 2`，立刻读 `u2_error_status`。
- 常见错误位：
  - `0x0001` 硬件过流（POE）
  - `0x0100` 软件过流
  - `0x0002` 过压
  - `0x0080` 欠压
  - `0x0004` 超速
  - `0x0200` 堵转

### 第 3 步（2 分钟）：采样是否在跑
- `f4_vdc_ad/f4_iu_ad/f4_iw_ad` 应持续刷新。
- 若基本不变：
  - 查 `S12AD0` 中断是否进入（`Config_S12AD0_user.c`）
  - 查触发链（MTU -> ADC）

### 第 4 步（2 分钟）：是否卡在启动前置阶段
- `g_st_cc.u1_flag_offset_calc` 需从 `0 -> 1`
- `g_st_cc.u1_flag_charge_bootstrap` 需从 `0 -> 1`
- 若不置位：
  - 看计数器是否增长：`g_st_cc.u2_crnt_offset_cnt`、`g_st_cc.u2_charge_bootstrap_cnt`
  - 不增长通常是电流中断链未跑通

### 第 5 步（2 分钟）：PWM 是否真的输出
- 看 `f4_modu/modv/modw`：
  - 长期固定在 `0.5`：通常仍在 offset/bootstrap 或控制未激活
  - 有动态变化：算法已下发占空比
- 若有变化但不转：
  - 查 POE 标志：`POE.ICSR1.BIT.POE0F`、`POE.OCSR1.BIT.OSF1`
  - 查功率级门极使能/相序/母线供电

### 第 6 步（2 分钟）：速度环是否闭合
- `f4_ref_speed_rad_ctrl` 给定后应非零
- `f4_speed_rad_lpf` 转动后应跟随变化
- `g_st_sc.u1_active` 应为 1

## 3. 快速判定表
| 现象 | 最可能问题点 | 优先动作 |
|---|---|---|
| RUN 命令后仍 STOP | 命令链/UI 模式 | 查 `com/g_u1_system_mode` 与 `g_u1_sw_userif` |
| 刚 RUN 立刻 ERROR | 保护触发 | 先解码 `u2_error_status` |
| 电压有值但电流不刷新 | ADC/中断链断 | 查 S12AD0 ISR 与触发源 |
| 调制值长期 0.5 | 卡在前置阶段 | 查 offset/bootstrap 标志与计数器 |
| 调制值在动但电机不转 | 功率级或机械侧 | 查 POE 标志、相序、驱动/负载 |

## 4. 默认关键阈值（当前工程）
- 载波：`20kHz`
- 电流环周期：`50us`
- 速度环周期：`500us`
- 过压/欠压：`60V / 8V`
- 默认超速：`4500rpm`

## 5. 建议的最短抓包/观测序列
- 序列 A（基础）：`status` -> `u2_error_status` -> `vdc/iu/iw`
- 序列 B（启动门槛）：`offset_flag` -> `bootstrap_flag` -> `u1_active(current/speed)`
- 序列 C（闭环效果）：`ref_speed_rad_ctrl` -> `speed_rad_lpf` -> `modu/modv/modw`
