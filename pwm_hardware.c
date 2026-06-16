/*
 * Arquivo: pwm_hardware.c
 * Descrição: Controle do sinal PWM de 25 kHz utilizando os Timers nativos do ARM.
 */

#include "main.h"
#include "config.h"

// Handler do Timer 2 configurado via CubeMX
extern TIM_HandleTypeDef htim2; 

/*
 * Modifica a velocidade do motor sem gerar overhead ou travar o processamento.
 * Recebe o duty_cycle de 0 a 100% e traduz para a resolução real do Timer (0 a 3599).
 */
void Set_PWM_Motor(uint8_t duty) {
    if (duty > 100) duty = 100;
    
    // Mapeia linearmente 0-100% para o range físico do registrador ARR (0 a 3599)
    uint32_t compare_value = (duty * 3599) / 100; // ARR = 90.000.000/25K - 1 = 3599  
    
    // Atualiza diretamente o registrador CCR2 (Canal 2 do TIM2, pino PB3/D3)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare_value);
}
