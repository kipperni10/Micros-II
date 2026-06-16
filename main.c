#include <xc.h>
#include "config.h"

// Definições de Estado do Sistema
typedef enum {
    MODO_MANUAL,
    MODO_AUTONOMO,    // Piloto Automático ativado (< 300ms acelerador)
    MODO_EMERGENCIA   // Temperatura crítica (>50°C por 2 min) ou Obstáculo
} EstadoVeiculo;

EstadoVeiculo estado_atual = MODO_MANUAL;

// Variáveis Globais de Controle (compartilhadas com isr.c e pwm_soft.c)
volatile unsigned char duty_cycle = 0; // 0 a 100%
volatile unsigned int rpm_atual = 0;
volatile unsigned long odometria_pulsos = 0;
volatile unsigned int tempo_acelerador_pressionado = 0;

// Flags de tempo atualizadas pela ISR do Timer0
extern volatile unsigned char timer_50ms_flag;
extern volatile unsigned char timer_05s_flag;
extern volatile unsigned char timer_1s_flag;

// Declaração das funções externas
extern void executa_pwm_burst(unsigned char duty);
extern void atualiza_interface_visual(void);

void setup_inicial() {
    // Configuração de I/O
    TRISA = 0xFF; // Entradas (Pedais, Botões, Sensor Proximidade, Temp)
    TRISB = 0x01; // RB0 entrada (Tacômetro), resto Saídas (Display, LED, PWM, Relé)
    
    // Configuração do TMR0 (Usado para multiplexação e ticks de tempo gerais)
    OPTION_REG = 0b00000101; // Prescaler ajustado para interrupts regulares
    INTCON = 0b11110000;     // Habilita GIE, T0IE, INTE
}

void controle_pedais() {
    if (estado_atual == MODO_EMERGENCIA) return;

    if (PEDAL_ACELERADOR) {
        tempo_acelerador_pressionado++;
        // Se manteve pressionado por 0,5s
        if (tempo_acelerador_pressionado >= TEMPO_500MS) {
            estado_atual = MODO_MANUAL; // Cancela autônomo
            if (duty_cycle <= 96) duty_cycle += 4;
            tempo_acelerador_pressionado = 0;
        }
    } else {
        // Lógica da Opção A: Acelerador solto rapidamente (< 300ms)
        if (tempo_acelerador_pressionado > 0 && tempo_acelerador_pressionado < TEMPO_300MS) {
            estado_atual = MODO_AUTONOMO;
            // O duty cycle é mantido no valor atual pelo piloto automático
        }
        tempo_acelerador_pressionado = 0;
    }

    if (PEDAL_FREIO) {
        estado_atual = MODO_MANUAL; // Freio exige assumir o controle
        if (timer_05s_flag) {
            if (duty_cycle >= 20) duty_cycle -= 20; 
            else duty_cycle = 0;
            // Não zeramos a flag aqui pois ela pode ser usada por outra rotina no mesmo ciclo
        }
    }
}

void sistema_anti_colisao() {
    // Opção A: Sensor de proximidade frontal
    if (SENSOR_OBSTACULO == 1) {
        if (timer_05s_flag) {
            if (duty_cycle >= 20) duty_cycle -= 20;
            else duty_cycle = 0;
        }
    }
}

void controle_rele_potencia() {
    // O cooler Intel não desliga no PWM 0%, então cortamos a alimentação
    if (duty_cycle < 19) {
        RELE_MOTOR = 0; // Desliga relé
    } else {
        RELE_MOTOR = 1; // Liga relé
    }
}

void main() {
    setup_inicial();
    
    while(1) {
        // 1. DOMÍNIO DO PWM (Bloqueante por ~5ms)
        // Garante a frequência de 25kHz exigida pelo cooler
        executa_pwm_burst(duty_cycle);
        
        // 2. DOMÍNIO LÓGICO E IHM (Execução rápida < 1ms)
        controle_pedais();
        sistema_anti_colisao();
        controle_rele_potencia();
        
        // Trata eventos baseados em 1 segundo (Freio motor)
        if (timer_1s_flag) {
            if (!PEDAL_ACELERADOR && !PEDAL_FREIO && estado_atual != MODO_AUTONOMO) {
                if (duty_cycle >= 4) duty_cycle -= 4;
            }
            timer_1s_flag = 0; // Consome a flag
        }

        // Consome a flag de 0.5s após todas as rotinas que dependem dela terem executado
        if (timer_05s_flag) timer_05s_flag = 0; 
        
        // Atualiza a IHM
        atualiza_interface_visual(); 
    }
}
