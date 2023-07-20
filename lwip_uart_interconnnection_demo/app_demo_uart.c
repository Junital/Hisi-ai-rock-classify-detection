/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hi_stdlib.h>
#include <hisignalling_protocol.h>
#include <hi_uart.h>
#include <app_demo_uart.h>
#include <iot_uart.h>
#include <hi_gpio.h>
#include <hi_io.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_uart.h"
#include "hi_uart.h"
#include "iot_watchdog.h"
#include "iot_errno.h"
#include "net_params.h"
#include "wifi_connecter.h"
#include "lwip/sockets.h"

UartDefConfig uartDefConfig = {0};
unsigned char uartReadBuff[32] = {0};
#define U_SLEEP_TIME 100000
static void Uart1GpioCOnfig(void)
{
#ifdef ROBOT_BOARD
    IoSetFunc(HI_IO_NAME_GPIO_5, IOT_IO_FUNC_GPIO_5_UART1_RXD);
    IoSetFunc(HI_IO_NAME_GPIO_6, IOT_IO_FUNC_GPIO_6_UART1_TXD);
    /* IOT_BOARD */
#elif defined(EXPANSION_BOARD)
    IoSetFunc(HI_IO_NAME_GPIO_0, IOT_IO_FUNC_GPIO_0_UART1_TXD);
    IoSetFunc(HI_IO_NAME_GPIO_1, IOT_IO_FUNC_GPIO_1_UART1_RXD);
#endif
}

int SetUartRecvFlag(UartRecvDef def)
{
    if (def == UART_RECV_TRUE)
    {
        uartDefConfig.g_uartReceiveFlag = HI_TRUE;
    }
    else
    {
        uartDefConfig.g_uartReceiveFlag = HI_FALSE;
    }

    return uartDefConfig.g_uartReceiveFlag;
}

int GetUartConfig(UartDefType type)
{
    int receive = 0;

    switch (type)
    {
    case UART_RECEIVE_FLAG:
        receive = uartDefConfig.g_uartReceiveFlag;
        break;
    case UART_RECVIVE_LEN:
        receive = uartDefConfig.g_uartLen;
        break;
    default:
        break;
    }
    return receive;
}

void ResetUartReceiveMsg(void)
{
    (void)memset_s(uartDefConfig.g_receiveUartBuff, sizeof(uartDefConfig.g_receiveUartBuff),
                   0x0, sizeof(uartDefConfig.g_receiveUartBuff));
}

unsigned char *GetUartReceiveMsg(void)
{
    return uartDefConfig.g_receiveUartBuff;
}

static hi_void *UartDemoTask(char *param)
{
    hi_u8 uartBuff[UART_BUFF_SIZE] = {0};
    hi_unref_param(param);
    printf("Initialize uart demo successfully, please enter some datas via DEMO_UART_NUM port...\n");
    Uart1GpioCOnfig();
    //
    int netId = ConnectToHotspot();

    int timeout = 3; /* timeout 10ms */
    while (timeout--)
    {
        printf("After %d seconds, I will start lwip test!\r\n", timeout);
        osDelay(100); /* 延时100ms */
    }

    printf("TcpClientTest start\r\n");
    printf("I will connect to %s\r\n", PARAM_SERVER_ADDR);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;                // AF_INET表示IPv4协议
    serverAddr.sin_port = htons(PARAM_SERVER_PORT); // 端口号，从主机字节序转为网络字节序
    if (inet_pton(AF_INET, PARAM_SERVER_ADDR, &serverAddr.sin_addr) <= 0)
    { // 将主机IP地址从“点分十进制”字符串 转化为 标准格式（32位整数）
        printf("inet_pton failed!\r\n");
        lwip_close(sockfd);
    }
    // 尝试和目标主机建立连接，连接成功会返回0 ，失败返回 -1
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("connect failed!\r\n");
        lwip_close(sockfd);
    }
    printf("connect to server %s success!\r\n", PARAM_SERVER_ADDR);

    for (;;)
    {
        uartDefConfig.g_uartLen = IoTUartRead(DEMO_UART_NUM, uartBuff, UART_BUFF_SIZE);
        if ((uartDefConfig.g_uartLen > 0) && (uartBuff[0] == 0xaa) && (uartBuff[1] == 0x55))
        {
            if (GetUartConfig(UART_RECEIVE_FLAG) == HI_FALSE)
            {
                (void)memcpy_s(uartReadBuff, uartDefConfig.g_uartLen,
                               uartBuff, uartDefConfig.g_uartLen);
                (void)SetUartRecvFlag(UART_RECV_TRUE);
                TaskMsleep(20); /* 20:sleep 20ms */
                uartReadBuff[uartDefConfig.g_uartLen] = '\n';
                ssize_t retval = send(sockfd, uartReadBuff, UART_BUFF_SIZE, 0);
                if (retval < 0)
                {
                    printf("send uartReadBuff failed!\r\n");
                    lwip_close(sockfd);
                }
                printf("send uartReadBuff to server done!\r\n");
                usleep(U_SLEEP_TIME * 5);
            }
        }
    }
    return HI_NULL;
}

/*
 * This demo simply shows how to read datas from UART2 port and then echo back.
 */
hi_void UartTransmit(hi_void)
{
    hi_u32 ret = 0;

    IotUartAttribute uartAttr = {
        .baudRate = 115200, /* baudRate: 115200 */
        .dataBits = 8,      /* dataBits: 8bits */
        .stopBits = 1,      /* stop bit */
        .parity = 0,
    };
    /* Initialize uart driver */
    ret = IoTUartInit(DEMO_UART_NUM, &uartAttr);
    if (ret != HI_ERR_SUCCESS)
    {
        printf("Failed to init uart! Err code = %d\n", ret);
        return;
    }
    /* Create a task to handle uart communication */
    osThreadAttr_t attr = {0};
    attr.stack_size = 20480;
    attr.priority = osPriorityNormal;
    attr.name = (hi_char *)"uart demo";
    if (osThreadNew((osThreadFunc_t)UartDemoTask, NULL, &attr) == NULL)
    {
        printf("Falied to create uart demo task!\n");
    }
}
SYS_RUN(UartTransmit);
