#include <xc.h>
#include "config.h"

// Macros para otimização de ciclos (ajuste fino no osciloscópio)
#define NOP() asm("nop")

/*
 * O PWM precisa gerar um período de ~40us (25 kHz).
 * A função executa_pwm_burst() gera o sinal continuamente por ~5ms.
 * Após esses 5ms, ela devolve o controle para o main() atualizar o display
 * e processar a telemetria, garantindo que o PWM domine 90% da CPU.
 */
void executa_pwm_burst(unsigned char duty_cycle) {
    // Escala de 0 a 100 mapeada para ciclos de delay.
    // Como os saltos são de 4% (0, 4, 8... 100), mapeamos para um range
    // onde o compilador XC8 consiga executar dentro de ~40us totais.
    
    // Tratamento das extremidades para evitar processamento inútil
    if (duty_cycle == 0) {
        PINO_PWM = 0;
        __delay_ms(5); // Mantém em LOW pela duração do burst
        return;
    }
    
    if (duty_cycle >= 100) {
        PINO_PWM = 1;
        __delay_ms(5); // Mantém em HIGH pela duração do burst
        return;
    }

    // Calcula tempos ON e OFF proporcionais. 
    // O fator divisor é empírico e será ajustado conforme o overhead do laço 'for' gerado pelo XC8.
    unsigned char t_on = (duty_cycle * 4) / 10; 
    unsigned char t_off = 40 - t_on;

    // 125 iterações de 40us resulta em uma rajada (burst) de ~5ms.
    for(unsigned char burst = 0; burst < 125; burst++) {
        
        PINO_PWM = 1;
        for(unsigned char i = 0; i < t_on; i++) {
            NOP(); // Preenche o tempo ON
        }
        
        PINO_PWM = 0;
        for(unsigned char i = 0; i < t_off; i++) {
            NOP(); // Preenche o tempo OFF
        }
    }
}
