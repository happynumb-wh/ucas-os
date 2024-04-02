/* Copyright 2018 Canaan Inc.
 *
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

#ifndef _BSP_PLATFORM_H
#define _BSP_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
/* Register base address */

/* Under Coreplex */
#define CLINT_BASE_ADDR     (0xffffffc002000000U)
#define PLIC_BASE_ADDR      (0xffffffc00C000000U)

/* Under TileLink */
#define UARTHS_BASE_ADDR    (0xffffffc038000000U)
#define GPIOHS_BASE_ADDR    (0xffffffc038001000U)

/* Under AXI 64 bit */
#define RAM_BASE_ADDR       (0xffffffc080000000U)
#define RAM_SIZE            (6 * 1024 * 1024U)

#define IO_BASE_ADDR        (0xffffffc040000000U)
#define IO_SIZE             (6 * 1024 * 1024U)

#define AI_RAM_BASE_ADDR    (0xffffffc080600000U)
#define AI_RAM_SIZE         (2 * 1024 * 1024U)

#define AI_IO_BASE_ADDR     (0xffffffc040600000U)
#define AI_IO_SIZE          (2 * 1024 * 1024U)

#define AI_BASE_ADDR        (0xffffffc040800000U)
#define AI_SIZE             (12 * 1024 * 1024U)

#define FFT_BASE_ADDR       (0xffffffc042000000U)
#define FFT_SIZE            (4 * 1024 * 1024U)

#define ROM_BASE_ADDR       (0xffffffc088000000U)
#define ROM_SIZE            (128 * 1024U)

/* Under AHB 32 bit */
#define DMAC_BASE_ADDR      (0xffffffc050000000U)

/* Under APB1 32 bit */
#define GPIO_BASE_ADDR      (0xffffffc050200000U)
#define UART1_BASE_ADDR     (0xffffffc050210000U)
#define UART2_BASE_ADDR     (0xffffffc050220000U)
#define UART3_BASE_ADDR     (0xffffffc050230000U)
#define SPI_SLAVE_BASE_ADDR (0xffffffc050240000U)
#define I2S0_BASE_ADDR      (0xffffffc050250000U)
#define I2S1_BASE_ADDR      (0xffffffc050260000U)
#define I2S2_BASE_ADDR      (0xffffffc050270000U)
#define I2C0_BASE_ADDR      (0xffffffc050280000U)
#define I2C1_BASE_ADDR      (0xffffffc050290000U)
#define I2C2_BASE_ADDR      (0xffffffc0502A0000U)
#define FPIOA_BASE_ADDR     (0xffffffc0502B0000U)
#define SHA256_BASE_ADDR    (0xffffffc0502C0000U)
#define TIMER0_BASE_ADDR    (0xffffffc0502D0000U)
#define TIMER1_BASE_ADDR    (0xffffffc0502E0000U)
#define TIMER2_BASE_ADDR    (0xffffffc0502F0000U)

/* Under APB2 32 bit */
#define WDT0_BASE_ADDR      (0xffffffc050400000U)
#define WDT1_BASE_ADDR      (0xffffffc050410000U)
#define OTP_BASE_ADDR       (0xffffffc050420000U)
#define DVP_BASE_ADDR       (0xffffffc050430000U)
#define SYSCTL_BASE_ADDR    (0xffffffc050440000U)
#define AES_BASE_ADDR       (0xffffffc050450000U)
#define RTC_BASE_ADDR       (0xffffffc050460000U)


/* Under APB3 32 bit */
#define SPI0_BASE_ADDR      (0xffffffc052000000U)
#define SPI1_BASE_ADDR      (0xffffffc053000000U)
#define SPI3_BASE_ADDR      (0xffffffc054000000U)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _BSP_PLATFORM_H */
