/**
 * \file            lwesp_opts_template.h
 * \brief           Template config file
 */

/*
 * Copyright (c) 2024 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.1.2-dev
 */
#ifndef LWESP_OPTS_HDR_H
#define LWESP_OPTS_HDR_H

#define LWESP_CFG_DBG LWESP_DBG_ON
#define LWESP_CFG_DBG_TYPES_ON LWESP_DBG_TYPE_ALL

/* USART */
#define LWESP_USART USART1
#define LWESP_USART_CLK LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1)
#define LWESP_USART_IRQ USART1_IRQn
#define LWESP_USART_IRQHANDLER USART1_IRQHandler

/* DMA settings */
#define LWESP_USART_DMA DMA2
#define LWESP_USART_DMA_CLK LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2)
#define LWESP_USART_DMA_RX_CH LL_DMA_CHANNEL_7
#define LWESP_USART_DMA_RX_REQ_NUM LL_DMA_REQUEST_2
#define LWESP_USART_DMA_RX_IRQ DMA2_Channel7_IRQn
#define LWESP_USART_DMA_RX_IRQHANDLER DMA2_Channel7_IRQHandler

/* DMA flags management */
#define LWESP_USART_DMA_RX_IS_TC LL_DMA_IsActiveFlag_TC7(LWESP_USART_DMA)
#define LWESP_USART_DMA_RX_IS_HT LL_DMA_IsActiveFlag_HT7(LWESP_USART_DMA)
#define LWESP_USART_DMA_RX_CLEAR_TC LL_DMA_ClearFlag_TC7(LWESP_USART_DMA)
#define LWESP_USART_DMA_RX_CLEAR_HT LL_DMA_ClearFlag_HT7(LWESP_USART_DMA)

/* USART TX PIN */
#define LWESP_USART_TX_PORT_CLK LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define LWESP_USART_TX_PORT GPIOA
#define LWESP_USART_TX_PIN LL_GPIO_PIN_9
#define LWESP_USART_TX_PIN_AF LL_GPIO_AF_7

/* USART RX PIN */
#define LWESP_USART_RX_PORT_CLK LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)
#define LWESP_USART_RX_PORT GPIOA
#define LWESP_USART_RX_PIN LL_GPIO_PIN_10
#define LWESP_USART_RX_PIN_AF LL_GPIO_AF_7

/* RESET PIN */
#define LWESP_RESET_PORT_CLK LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB)
#define LWESP_RESET_PORT GPIOB
#define LWESP_RESET_PIN LL_GPIO_PIN_5

#define LWESP_USART_DMA_RX_BUFF_SIZE 0x200
#define LWESP_MEM_SIZE 0x1000
#define LWESP_CFG_MAX_SSID_LENGTH 33

#endif /* LWESP_OPTS_HDR_H */
