#include "stm32f30x.h"
#include "stm32f3_discovery.h"

#include <string.h>
#include "nixie_font.h"

RCC_ClocksTypeDef RCC_Clocks;
volatile uint32_t TimingDelay = 0;

void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);

void SysTick_Handler(void) {
    TimingDelay_Decrement();
}

void TimingDelay_Decrement(void) {
    if (TimingDelay != 0x00) { 
        TimingDelay--;
    }
}

void Delay(__IO uint32_t nTime) {
    TimingDelay = nTime;
    while(TimingDelay != 0);
}


int initSerial() {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = 2;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_7);

    USART_InitStructure.USART_BaudRate = 115200; 
    USART_InitStructure.USART_WordLength = USART_WordLength_8b ;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 

    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);

    return 0;
}

int initIo() {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 |
        GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    return 0;
}

void putSerialChar(char c) {
    USART_SendData(USART1, c);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
    }
}

void putSerialString(char *str) {
    while(*str) {
        putSerialChar(*str);
        str++;
    }
}

void putSerialHex(uint32_t val) {
    int offset;
    putSerialString("0x");
    for (offset = 28; offset >= 0; offset -= 4) {
        int digit = (val >> offset) & 0xf;
        if (digit < 0xa) {
             putSerialChar(digit + '0');
        } else {
             putSerialChar(digit - 0xa + 'a');
        }
    }
}

uint16_t flipShift(uint16_t value) {
    value = ((value & 0x00ff) << 8) | ((value & 0xff00) >> 8);
    value = ((value & 0x0f0f) << 4) | ((value & 0xf0f0) >> 4);
    value = ((value & 0x3333) << 2) | ((value & 0xcccc) >> 2);
    value = ((value & 0x5555) << 1) | ((value & 0xaaaa) >> 1);
    return value >> 1;
}

uint16_t ones(uint16_t value) {
    int i;
    int cnt = 0;
    for (i = 0; i < 16; i++) {
        if (value & (1 << i)) {
            cnt++;
        }
    }
    return cnt;
}

int main(void) {
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 256);

    initSerial();
    initIo();

    GPIO_Write(GPIOA, 0x7f);
    GPIO_Write(GPIOD, 0x00);

    uint32_t x;
    uint32_t y;
    uint32_t cnt = 0;
    uint32_t i = 0;
    uint32_t slow = 3;
    volatile uint32_t d = 0;

    putSerialString("hi!\r\n");

    while (1) {
        for (x = 0; x < 15; x++) {
            uint8_t digits[3];
            uint8_t j;

            digits[0] = cnt / 100 % 10;
            digits[1] = cnt / 10 % 10;
            digits[2] = cnt % 10;

            uint8_t cd = x / 5;
            uint8_t cc = x % 5;

            uint8_t data = 0;

            for (j = 0; j < 7; j++) {
                data |= !!(nixieFontData[digits[cd] * 7 + (6 - j)] & (1 << (4 - cc)));
                data <<= 1;
            }
            data >>= 1;

            GPIO_Write(GPIOD, (1 << x));
            GPIO_Write(GPIOA, ~data);
            for (d = 0; d < 100 * slow; d++) {
            }
        }
        for (x = 0; x < 15; x++) {
            GPIO_Write(GPIOD, (1 << x));
            GPIO_Write(GPIOA, x & 1 ? 0x55 : 0xaa);
            for (d = 0; d < 5 * slow; d++) {
            }
        }
        for (x = 0; x < 15; x++) {
            GPIO_Write(GPIOD, (1 << x));
            GPIO_Write(GPIOA, x & 1 ? 0xaa : 0x55);
            for (d = 0; d < 5 * slow; d++) {
            }
        }
        if (i % 100 == 0) {
            cnt++;
        }
        i++;
    }
}
