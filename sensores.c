/*
 * Arquivo: sensores.c
 * Descrição: Processamento analógico (ADC de 12-bits), telemetria térmica e intenção de manobra.
 */

#include "main.h"
#include "config.h"
#include <stdlib.h> // Para a função abs()

extern EstadoVeiculo estado_atual;
extern ADC_HandleTypeDef hadc1; // Handler do ADC1 configurado pelo CubeMX

// Controles de Temperatura
uint32_t contador_temp_alta = 0;
uint32_t contador_temp_critica = 0;

// Controles de Direção
uint8_t seta_ativada = 0;
uint32_t ultimo_tick_seta = 0;
uint32_t posicao_volante_anterior = 0;
uint32_t ultimo_movimento_rapido = 0;

// Limiar de variação do ADC (Ajustar de acordo com o potenciômetro 0-4095)
#define LIMIAR_MOVIMENTO_RAPIDO 400 

// Função de Abstração para Polling do ADC no STM32
uint32_t Le_ADC_Canal(uint32_t canal) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = canal;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        return HAL_ADC_GetValue(&hadc1);
    }
    return 0; // Retorna 0 em caso de falha timeout
}

void Monitora_Temperatura() {
    // Leitura simulada mapeada. No hardware real, aplique a fórmula do termistor/LM35
    uint32_t leitura_adc = Le_ADC_Canal(ADC_CHANNEL_0); 
    uint32_t temp_atual = (leitura_adc * 100) / 4095; // Mapeamento grosseiro para escala de graus
    
    if (temp_atual > 40) {
        contador_temp_alta++;
        // Permanece acima de 40°C por mais de 2 leituras de 50ms (0.1s)
        if (contador_temp_alta >= 2) {
            HAL_GPIO_WritePin(PORT_LEDS, PINO_LED_ALERTA, GPIO_PIN_SET);
        }
    } else {
        contador_temp_alta = 0;
        HAL_GPIO_WritePin(PORT_LEDS, PINO_LED_ALERTA, GPIO_PIN_RESET);
    }
    
    if (temp_atual >= 50) {
        contador_temp_critica++;
        // Permanece acima de 50°C por 2400 leituras (2 minutos)
        if (contador_temp_critica >= 2400) {
            estado_atual = MODO_EMERGENCIA; // Força parada no main
        }
    } else {
        contador_temp_critica = 0;
    }
}

void Monitora_Volante_e_Setas() {
    uint32_t tick_atual = HAL_GetTick();
    uint32_t posicao_volante = Le_ADC_Canal(ADC_CHANNEL_1); 
    
    uint32_t variacao = abs((int)(posicao_volante - posicao_volante_anterior));
    
    // Detecta intenção de curva
    if (variacao > LIMIAR_MOVIMENTO_RAPIDO) {
        if (tick_atual - ultimo_movimento_rapido < TEMPO_300MS) {
            seta_ativada = !seta_ativada; // Alterna estado (liga ou desliga após concluir)
        }
        ultimo_movimento_rapido = tick_atual;
    }
    
    posicao_volante_anterior = posicao_volante;
    
    // Lógica da Seta Piscante (1 Hz -> 500ms ligada, 500ms desligada)
    if (seta_ativada) {
        if (tick_atual - ultimo_tick_seta >= 500) {
            HAL_GPIO_TogglePin(PORT_LEDS, PINO_LED_ALERTA); // Ajustar para o pino da seta real
            ultimo_tick_seta = tick_atual;
        }
    } else {
        // Garante que a seta fique desligada quando desativada
        // HAL_GPIO_WritePin(PORT_LEDS, PINO_SETA, GPIO_PIN_RESET); 
    }
}
