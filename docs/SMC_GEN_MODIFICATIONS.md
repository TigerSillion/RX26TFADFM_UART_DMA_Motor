# `smc_gen` 修改清单与边界

## 1. 修改原则
- 仅允许修改 `smc_gen` 用户保护区（`/* Start user code ... */`）。
- 业务逻辑统一放在 `src/app/*`。
- `smc_gen` 只承担驱动回调桥接和寄存器地址绑定，不承载业务状态机。

## 2. 本次已修改文件

### 2.1 `src/smc_gen/Config_DMAC0/Config_DMAC0_user.c`
- `R_Config_DMAC0_Create_UserInit()` 用户区：
  - `DMAC0.DMSAR = (void *)&SCI5.RDR;`
  - `DMAC0.DMDAR = (void *)&g_sci5_rx_byte;`
  - `g_sci5_rx_flag = 0U;`
- `r_dmac0_callback_transfer_end()` 用户区：
  - 调用 `uart_dma_port_on_rx_end_isr();`

### 2.2 `src/smc_gen/Config_DMAC1/Config_DMAC1_user.c`
- `R_Config_DMAC1_Create_UserInit()` 用户区：
  - `DMAC1.DMDAR = (void *)&SCI5.TDR;`

### 2.3 `src/smc_gen/Config_SCI5/Config_SCI5_user.c`
- `r_Config_SCI5_callback_transmitend()` 用户区：
  - 调用 `uart_dma_port_on_tx_end_isr();`
- `r_Config_SCI5_callback_receiveerror()` 用户区：
  - 调用 `uart_dma_port_on_rx_error_isr();`
- `r_Config_SCI5_callback_receiveend()` 用户区：
  - 保持空桥接（RX完成由 DMAC0 回调主导）。

### 2.4 `src/smc_gen/general/r_cg_userdefine.h`
- 用户 include 区：
  - 引入 `../../app/uart_dma/uart_dma_port.h`
- 用户 function 区：
  - 声明 `g_sci5_rx_byte/g_sci5_rx_flag/g_sci5_tx_done`

## 3. 严禁修改范围
- 严禁修改 `smc_gen` 非用户区自动生成代码。
- 严禁在 `smc_gen` 中写入业务协议、命令分发、参数管理逻辑。

## 4. 追溯要求
- 所有用户区新增代码必须带 `USER CODE SAFE` 注释。
- 每次修改后同步更新：
  - `CHANGELOG.md`
  - `docs/CHANGE_RECORDS.md`
  - `docs/SMC_REGEN_CHECKLIST.md`
