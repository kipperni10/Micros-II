#include <xc.h>
#include "config.h"

// Definições de Estado do Sistema
typedef enum {
    MODO_MANUAL,
    MODO_AUTONOMO,    // Piloto Automático ativado (< 300ms acelerador)
    MODO_EMERGENCIA   // Temperatura crítica (>50°C por 2 min) ou Obstáculo
} EstadoVeiculo;

EstadoVeiculo estado_atual = MODO_MANUAL;

// Variáveis Globais de Controle
volatile unsigned char duty_cycle = 0; // 0 a 100%
volatile unsigned int rpm_atual = 0;
volatile unsigned long odometria_pulsos = 0;
volatile unsigned int tempo_acelerador_pressionado = 0;

void setup_inicial() {
    // Configuração de I/O
    TRISA = 0xFF; // Entradas (Pedais, Botões, Sensor Proximidade, Temp)
    TRISB = 0x01; // RB0 entrada (Tacômetro), resto Saídas (Display, LED, PWM, Relé)
    
    // Configuração do TMR0 (Usado para base de tempo e multiplexação)
    OPTION_REG = 0b00000101; // Prescaler ajustado para interrupts regulares
    INTCON = 0b11110000;     // Habilita GIE, T0IE, INTE
}

void controle_pedais() {
    if (estado_atual == MODO_EMERGENCIA) return;

    if (PEDAL_ACELERADOR) {
        tempo_acelerador_pressionado++;
        // Se manteve pressionado por 0,5s (ajustar contador p/ base de tempo)
        if (tempo_acelerador_pressionado >= TEMPO_500MS) {
            estado_atual = MODO_MANUAL; // Cancela autônomo
            if (duty_cycle <= 96) duty_cycle += 4; [cite: 149]
            tempo_acelerador_pressionado = 0;
        }
    } else {
        // Lógica da Opção A: Acelerador solto rapidamente (< 300ms)
        if (tempo_acelerador_pressionado > 0 && tempo_acelerador_pressionado < TEMPO_300MS) {
            estado_atual = MODO_AUTONOMO; [cite: 264]
            // Duty cycle é mantido no valor atual
        }
        tempo_acelerador_pressionado = 0;
    }

    if (PEDAL_FREIO) {
        estado_atual = MODO_MANUAL; // Freio sempre cancela o autônomo [cite: 267]
        // Lógica de redução de 20% a cada 0,5s [cite: 150]
        if (duty_cycle >= 20) duty_cycle -= 20; 
        else duty_cycle = 0;
    }
}

void sistema_anti_colisao() {
    // Opção A: Sensor de proximidade [cite: 270]
    if (SENSOR_OBSTACULO == 1) {
        // Reduz o duty cycle em 20% a cada 0,5 s [cite: 271]
        if (timer_05s_flag) {
            if (duty_cycle >= 20) duty_cycle -= 20;
            else duty_cycle = 0;
            timer_05s_flag = 0;
        }
    }
}

void controle_rele_potencia() {
    if (duty_cycle < 19) {
        RELE_MOTOR = 0; // Desliga alimentação do cooler 
    } else {
        RELE_MOTOR = 1;
    }
}

void main() {
    setup_inicial();
    
    while(1) {
        // A geração do PWM e a multiplexação do display a 60Hz 
        // rodam em background via interrupções. [cite: 212]
        
        controle_pedais();
        sistema_anti_colisao();
        controle_rele_potencia();
        
        // Freio motor: Se nenhum pedal pressionado e não está autônomo
        if (!PEDAL_ACELERADOR && !PEDAL_FREIO && estado_atual != MODO_AUTONOMO) {
            if (timer_1s_flag) {
                if (duty_cycle >= 4) duty_cycle -= 4; [cite: 153]
                timer_1s_flag = 0;
            }
        }
        
        // Atualiza buffers do display (RPM/3, Velocidade, Alertas) [cite: 224, 240]
        atualiza_interface_visual(); 
    }
}
