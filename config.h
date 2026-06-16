/*
 * Arquivo: config.h
 * Descrição: Definições de pinagem e constantes para NUCLEO-F446RE.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f4xx_hal.h"

// MAPEAMENTO DE ENTRADAS (Conectores Analógicos/Digitais Arduino)
#define PORT_PEDAIS          GPIOA
#define PINO_ACELERADOR      GPIO_PIN_0  // Pino A0 (Entrada digital/ADC)
#define PINO_FREIO           GPIO_PIN_1  // Pino A1 (Entrada digital/ADC)

#define PORT_SENSORES        GPIOA
#define PINO_OBSTACULO       GPIO_PIN_4  // Pino A2 (Sensor ultrassônico/ótico - Opção A)

#define PORT_BOTOES          GPIOC
#define PINO_BOTAO_FUNCAO    GPIO_PIN_1  // Pino PC1 (Botão de Função)
#define PINO_BOTAO_SELECIONA GPIO_PIN_0  // Pino PC0 (Botão de Seleção)

// MAPEAMENTO DE SAÍDAS E ATUADORES
// O PWM por hardware utilizará o TIM2 (Canal 2), mapeado no pino PB3
#define PORT_PWM             GPIOB
#define PINO_PWM             GPIO_PIN_3  // Pino D3 (Saída PWM do Motor)

#define PORT_RELE            GPIOB
#define PINO_RELE            GPIO_PIN_5  // Pino D4 (Controle do Relé de Alimentação)

#define PORT_LEDS            GPIOB
#define PINO_LED_ALERTA      GPIO_PIN_4  // Pino D5 (LED de Alerta de Temperatura)

// Pinos de controle de multiplexação do Display de 7 segmentos
#define PORT_DISP_SEL        GPIOA
#define PINO_SEL_DISPLAY_0   GPIO_PIN_10 // Pino D6
#define PINO_SEL_DISPLAY_1   GPIO_PIN_8  // Pino D7
#define PORT_DISP_SEL_2      GPIOB
#define PINO_SEL_DISPLAY_2   GPIO_PIN_10 // Pino D8

// CONSTANTES DE TEMPO (Baseadas no clock de 1ms SysTick)
#define TEMPO_300MS          300
#define TEMPO_500MS          500

#endif
