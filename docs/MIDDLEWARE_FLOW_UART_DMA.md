# UART-DMA 中间件流程文档

## 1. 分层结构
- SMC 驱动层：`src/smc_gen/*`
- 端口抽象层：`src/app/uart_dma/uart_dma_port.*`
- 示例业务层：`src/app/uart_dma/uart_dma_demo.*`
- 入口层：`src/RX26TFADFM_UART_DMA_Motor.c`

## 2. 初始化流程
1. `main()` 调用 `uart_dma_demo_run_forever()`。
2. `uart_dma_port_init()`：
   - 清零 `g_sci5_rx_byte/g_sci5_rx_flag/g_sci5_tx_done`
   - 绑定 DMAC0/DMAC1 地址
3. `uart_dma_port_start()`：
   - 开启 SCI5 接收
   - 启动 DMAC0
   - 启动 SCI5 中断

## 3. RX 流程（DMAC0）
1. SCI5 收到字节后触发 DMAC0 搬运。
2. DMAC0 将 `SCI5.RDR` 写入 `g_sci5_rx_byte`。
3. DMAC0 中断回调调用 `uart_dma_port_on_rx_end_isr()`：
   - `g_sci5_rx_flag = 1U`
   - `DMAC0.DMCRA = 1U`
   - `R_Config_DMAC0_Start()` 重新使能
4. 业务层通过 `uart_dma_port_try_get_rx_byte()` 轮询取数。

## 4. TX 流程（DMAC1 软件触发）
1. 业务层调用 `uart_dma_port_tx_async(buf, len)`。
2. 对每个字节：
   - 清 `g_sci5_tx_done`
   - 设置 `DMAC1.DMSAR` 与 `DMAC1.DMCRA = 1`
   - 调用 `R_Config_DMAC1_Set_SoftwareTrigger()`
   - 调用 `R_Config_SCI5_Serial_Send(NULL, 1U)`
3. TEI5 回调置位 `g_sci5_tx_done`。
4. 轮询等待完成后继续下一个字节。

## 5. 异常流程
- SCI5 ERI5 触发后调用 `uart_dma_port_on_rx_error_isr()`，清理 RX 有效标志。

## 6. 后续扩展方向
- 增加帧协议（SOF/LEN/PAYLOAD/CRC）。
- TX 从字节搬运扩展为块搬运。
- 引入接收环形缓冲和解析状态机。
