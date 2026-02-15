#include "uart_dma_demo.h"

#include "uart_dma_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void demo_send_text(const char *text);
static void demo_send_help(void);
static void demo_send_stats(void);
static void demo_run_burst_test(void);
static void demo_handle_command(const char *cmd_line);
static void demo_autosweep_service(void);
static void demo_autosweep_stop(void);
static int demo_parse_u32_list4(const char *src, uint32_t *v0, uint32_t *v1, uint32_t *v2, uint32_t *v3);

static volatile uint8_t g_autosweep_active = 0U;
static uint32_t g_autosweep_start = 0UL;
static uint32_t g_autosweep_end = 0UL;
static uint32_t g_autosweep_step = 0UL;
static uint32_t g_autosweep_interval_ms = 0UL;
static uint32_t g_autosweep_next_baud = 0UL;
static uint8_t g_autosweep_dir_up = 1U;

void uart_dma_demo_run_forever(void)
{
    uint8_t rx_byte;
    uint8_t echo_enabled = 1U;
    char cmd_buf[80];
    uint8_t cmd_len = 0U;
    uint8_t in_cmd = 0U;

    uart_dma_port_init();
    uart_dma_port_start();
    demo_send_help();
    demo_send_text("ready@115200\r\n");

    while (1)
    {
        if (uart_dma_port_try_get_rx_byte(&rx_byte))
        {
            if (in_cmd != 0U)
            {
                if ((rx_byte == '\r') || (rx_byte == '\n'))
                {
                    cmd_buf[cmd_len] = '\0';
                    demo_handle_command(cmd_buf);
                    cmd_len = 0U;
                    in_cmd = 0U;
                    continue;
                }

                if (cmd_len < (sizeof(cmd_buf) - 1U))
                {
                    cmd_buf[cmd_len] = (char)rx_byte;
                    cmd_len++;
                }
                continue;
            }

            if (rx_byte == '@')
            {
                in_cmd = 1U;
                cmd_len = 0U;
                continue;
            }

            if (echo_enabled != 0U)
            {
                (void)uart_dma_port_tx_async(&rx_byte, 1U);
            }

            switch ((char)rx_byte)
            {
                case 'h':
                case 'H':
                    demo_send_help();
                    break;
                case 's':
                case 'S':
                    demo_send_stats();
                    break;
                case 't':
                case 'T':
                    demo_send_text("UART_DMA_TEST_OK\r\n");
                    break;
                case 'b':
                case 'B':
                    demo_run_burst_test();
                    break;
                case 'c':
                case 'C':
                    uart_dma_port_reset_diag();
                    demo_send_text("diag counters reset\r\n");
                    break;
                case 'e':
                case 'E':
                    echo_enabled = (uint8_t)(1U - echo_enabled);
                    demo_send_text((echo_enabled != 0U) ? "echo on\r\n" : "echo off\r\n");
                    break;
                default:
                    break;
            }
        }
        demo_autosweep_service();
    }
}

static void demo_send_text(const char *text)
{
    if (text == NULL)
    {
        return;
    }
    (void)uart_dma_port_tx_async((const uint8_t *)text, (uint16_t)strlen(text));
}

static void demo_send_help(void)
{
    demo_send_text("\r\n=== UART DMA TEST IF ===\r\n");
    demo_send_text("h: help\r\n");
    demo_send_text("s: stats\r\n");
    demo_send_text("t: send test token\r\n");
    demo_send_text("b: burst 256-byte tx test\r\n");
    demo_send_text("c: clear diag counters\r\n");
    demo_send_text("e: toggle echo on/off\r\n");
    demo_send_text("@AUTOSWEEP:START,END,STEP,MS\r\n");
    demo_send_text("@AUTOSWEEP:STOP\r\n");
    demo_send_text("@AUTOSWEEP:STATUS\r\n");
}

static void demo_send_stats(void)
{
    char line[128];
    const uart_dma_diag_t *diag = uart_dma_port_get_diag();

    (void)snprintf(line, sizeof(line),
                   "rx=%lu rx_ovr=%lu tx=%lu tx_to=%lu rx_err=%lu\r\n",
                   (unsigned long)diag->rx_count,
                   (unsigned long)diag->rx_overrun_count,
                   (unsigned long)diag->tx_count,
                   (unsigned long)diag->tx_timeout_count,
                   (unsigned long)diag->rx_error_count);
    demo_send_text(line);
}

static void demo_run_burst_test(void)
{
    uint8_t i;
    uint8_t buf[256];
    for (i = 0U; i < 255U; i++)
    {
        buf[i] = (uint8_t)('A' + (i % 26U));
    }
    buf[255] = '\n';
    (void)uart_dma_port_tx_async(buf, (uint16_t)sizeof(buf));
    demo_send_text("burst done\r\n");
}

static void demo_handle_command(const char *cmd_line)
{
    char rsp[80];
    uint32_t baud;
    uint32_t start_baud;
    uint32_t end_baud;
    uint32_t step_baud;
    uint32_t interval_ms;
    uart_dma_baud_status_t rc;
    char *endp;

    if (cmd_line == NULL)
    {
        return;
    }

    if (strcmp(cmd_line, "PING") == 0)
    {
        demo_send_text("@PONG\r\n");
        return;
    }

    if (strncmp(cmd_line, "SETBAUD:", 8) == 0)
    {
        baud = strtoul(&cmd_line[8], &endp, 10);
        if ((*endp != '\0') || (baud == 0UL))
        {
            demo_send_text("@ERR:BAD_BAUD\r\n");
            return;
        }

        (void)snprintf(rsp, sizeof(rsp), "@ACK:SETBAUD:%lu\r\n", (unsigned long)baud);
        demo_send_text(rsp);
        rc = uart_dma_port_set_baud(baud);
        if (rc == UART_DMA_BAUD_OK)
        {
            (void)snprintf(rsp, sizeof(rsp), "@READY:%lu\r\n", (unsigned long)baud);
            demo_send_text(rsp);
        }
        else if (rc == UART_DMA_BAUD_UNSUPPORTED)
        {
            demo_send_text("@ERR:UNSUPPORTED\r\n");
        }
        else
        {
            demo_send_text("@ERR:BAD_BAUD\r\n");
        }
        return;
    }

    if (strcmp(cmd_line, "GETBAUD") == 0)
    {
        (void)snprintf(rsp, sizeof(rsp), "@BAUD:%lu\r\n", (unsigned long)uart_dma_port_get_baud());
        demo_send_text(rsp);
        return;
    }

    if (strcmp(cmd_line, "AUTOSWEEP:STOP") == 0)
    {
        demo_autosweep_stop();
        demo_send_text("@AUTOSWEEP:STOPPED\r\n");
        return;
    }

    if (strcmp(cmd_line, "AUTOSWEEP:STATUS") == 0)
    {
        (void)snprintf(rsp, sizeof(rsp),
                       "@AUTOSWEEP:%s,%lu,%lu,%lu,%lu,NEXT=%lu\r\n",
                       (g_autosweep_active != 0U) ? "ON" : "OFF",
                       (unsigned long)g_autosweep_start,
                       (unsigned long)g_autosweep_end,
                       (unsigned long)g_autosweep_step,
                       (unsigned long)g_autosweep_interval_ms,
                       (unsigned long)g_autosweep_next_baud);
        demo_send_text(rsp);
        return;
    }

    if (strncmp(cmd_line, "AUTOSWEEP:", 10) == 0)
    {
        if (demo_parse_u32_list4(&cmd_line[10], &start_baud, &end_baud, &step_baud, &interval_ms) == 0)
        {
            demo_send_text("@ERR:BAD_SWEEP_ARG\r\n");
            return;
        }
        if ((step_baud == 0UL) || (interval_ms < 10UL))
        {
            demo_send_text("@ERR:BAD_SWEEP_ARG\r\n");
            return;
        }

        g_autosweep_start = start_baud;
        g_autosweep_end = end_baud;
        g_autosweep_step = step_baud;
        g_autosweep_interval_ms = interval_ms;
        g_autosweep_next_baud = start_baud;
        g_autosweep_dir_up = (start_baud <= end_baud) ? 1U : 0U;
        g_autosweep_active = 1U;
        demo_send_text("@AUTOSWEEP:STARTED\r\n");
        return;
    }

    demo_send_text("@ERR:UNKNOWN_CMD\r\n");
}

static void demo_autosweep_stop(void)
{
    g_autosweep_active = 0U;
}

static void demo_autosweep_service(void)
{
    char rsp[80];
    uint32_t target;
    uart_dma_baud_status_t rc;

    if (g_autosweep_active == 0U)
    {
        return;
    }

    target = g_autosweep_next_baud;
    (void)snprintf(rsp, sizeof(rsp), "@SWEEP:SET:%lu\r\n", (unsigned long)target);
    demo_send_text(rsp);

    rc = uart_dma_port_set_baud(target);
    if (rc != UART_DMA_BAUD_OK)
    {
        (void)snprintf(rsp, sizeof(rsp), "@SWEEP:ERR:%lu\r\n", (unsigned long)target);
        demo_send_text(rsp);
        demo_autosweep_stop();
        return;
    }

    (void)snprintf(rsp, sizeof(rsp), "@SWEEP:OK:%lu\r\n", (unsigned long)target);
    demo_send_text(rsp);

    if (g_autosweep_dir_up != 0U)
    {
        if ((g_autosweep_next_baud + g_autosweep_step) > g_autosweep_end)
        {
            demo_send_text("@SWEEP:DONE\r\n");
            demo_autosweep_stop();
            return;
        }
        g_autosweep_next_baud += g_autosweep_step;
    }
    else
    {
        if ((g_autosweep_next_baud < g_autosweep_step) ||
            ((g_autosweep_next_baud - g_autosweep_step) < g_autosweep_end))
        {
            demo_send_text("@SWEEP:DONE\r\n");
            demo_autosweep_stop();
            return;
        }
        g_autosweep_next_baud -= g_autosweep_step;
    }

    (void)R_BSP_SoftwareDelay(g_autosweep_interval_ms, BSP_DELAY_MILLISECS);
}

static int demo_parse_u32_list4(const char *src, uint32_t *v0, uint32_t *v1, uint32_t *v2, uint32_t *v3)
{
    char buf[64];
    char *p;
    char *t0;
    char *t1;
    char *t2;
    char *t3;
    char *endp;

    if ((src == NULL) || (v0 == NULL) || (v1 == NULL) || (v2 == NULL) || (v3 == NULL))
    {
        return 0;
    }

    if (strlen(src) >= sizeof(buf))
    {
        return 0;
    }
    (void)strcpy(buf, src);

    p = buf;
    t0 = strtok(p, ",");
    t1 = strtok(NULL, ",");
    t2 = strtok(NULL, ",");
    t3 = strtok(NULL, ",");
    if ((t0 == NULL) || (t1 == NULL) || (t2 == NULL) || (t3 == NULL))
    {
        return 0;
    }
    if (strtok(NULL, ",") != NULL)
    {
        return 0;
    }

    *v0 = strtoul(t0, &endp, 10);
    if (*endp != '\0')
    {
        return 0;
    }
    *v1 = strtoul(t1, &endp, 10);
    if (*endp != '\0')
    {
        return 0;
    }
    *v2 = strtoul(t2, &endp, 10);
    if (*endp != '\0')
    {
        return 0;
    }
    *v3 = strtoul(t3, &endp, 10);
    if (*endp != '\0')
    {
        return 0;
    }

    return 1;
}
