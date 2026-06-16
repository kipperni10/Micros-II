#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>

// Configurações do Oscilador (Necessário para a macro __delay_ms funcionar)
#define _XTAL_FREQ 4000000 // Cristal de 4 MHz

// MAPEAMENTO DE ENTRADAS (Sensores e IHM)
// Assumindo entradas conectadas ao PORTA e pinos restantes do PORTB
#define PEDAL_ACELERADOR PORTAbits.RA0
#define PEDAL_FREIO      PORTAbits.RA1
#define BOTAO_FUNCAO     PORTAbits.RA2
#define BOTAO_SELECIONA  PORTAbits.RA3
#define SENSOR_OBSTACULO PORTAbits.RA4 // Usado na Opção A

// MAPEAMENTO DE SAÍDAS (Atuadores e IHM)
// O Tacômetro (Tach) do Intel Fan DEVE obrigatoriamente estar no pino RB0/INT
// O pino de PWM será chaveado no software (bit-banging)
#define PINO_PWM         PORTBbits.RB1
#define RELE_MOTOR       PORTBbits.RB2
#define LED_ALERTA       PORTBbits.RB3

// Pinos de controle de multiplexação do Display de 7 segmentos
#define SEL_DISPLAY_0    PORTBbits.RB4
#define SEL_DISPLAY_1    PORTBbits.RB5
#define SEL_DISPLAY_2    PORTBbits.RB6

// Porta ou pinos designados para enviar o dado BCD ao display
#define PORTA_DADOS_DISP PORTB // Ajuste conforme o roteamento real da placa

// MACROS DE TEMPO (Baseadas na interrupção do TMR0 de 1ms)
#define TEMPO_300MS 300 // Usado para detectar movimento rápido (piloto automático)
#define TEMPO_500MS 500 // Usado para resposta dos pedais

#endif
