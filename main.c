/*
 * Arquivo: main.c
 * Descrição: Loop principal e máquina de estados do Computador de Bordo.
 * Microcontrolador: PIC16F84 (4 MHz)
 * Funcionalidade: Controle de RPM (PWM Bit-Banging), Odometria, Piloto Automático (Opção A) e Telemetria.
 */

// CONFIGURAÇÃO DOS FUSE BITS DO PIC16F84
#pragma config FOSC = XT   // Oscilador a Cristal (XT é usado para 4 MHz)
#pragma config WDTE = OFF  // Watchdog Timer (Desabilitado para não resetar o laço PWM)
#pragma config PWRTE = ON  // Power-up Timer (Habilitado para estabilizar tensão no boot)
#pragma config CP = OFF    // Code Protection (Desabilitado)

#include <xc.h>
#include "config.h"

// DEFINIÇÕES DE ESTADO DO SISTEMA
typedef enum {
    MODO_MANUAL,
    MODO_AUTONOMO,    // Piloto Automático ativado (< 300ms acelerador)
    MODO_EMERGENCIA   // Temperatura crítica (>50°C por 2 min) ou Obstáculo
} EstadoVeiculo;

EstadoVeiculo estado_atual = MODO_MANUAL;

// VARIÁVEIS GLOBAIS DE CONTROLE
// Compartilhadas com isr.c, pwm_soft.c, display_mux.c e sensores.c
volatile unsigned char duty_cycle = 0; // 0 a 100%
volatile unsigned int rpm_atual = 0;
volatile unsigned long odometria_pulsos = 0;
volatile unsigned int tempo_acelerador_pressionado = 0;

// Flags de tempo atualizadas pela ISR do Timer0 (isr.c)
extern volatile unsigned char timer_50ms_flag;
extern volatile unsigned char timer_05s_flag;
extern volatile unsigned char timer_1s_flag;

// DECLARAÇÃO DAS FUNÇÕES EXTERNAS
extern void executa_pwm_burst(unsigned char duty);
extern void atualiza_interface_visual(void);
extern void monitora_volante_e_setas(void);
extern void monitora_temperatura(void);

// CONFIGURAÇÃO INICIAL (HARDWARE)
void setup_inicial() {
    // Configuração de I/O
    TRISA = 0xFF; // Todas como entradas (Pedais, Botões, Sensor Proximidade, Temp, Volante)
    TRISB = 0x01; // RB0 como entrada (Tacômetro/INT), resto como Saídas (Display, LED, PWM, Relé)
    
    // Configuração do TMR0 (Usado para multiplexação e base de tempo)
    // Prescaler 1:64, Clock Interno (Fosc/4)
    OPTION_REG = 0b00000101; 
    
    // Habilita Interrupções Globais (GIE), Timer0 (T0IE) e Externa RB0 (INTE)
    INTCON = 0b11110000;     
    
    // Inicialização segura das saídas
    PORTB = 0x00;
}

// LÓGICA DE CONTROLE: PEDAIS E PILOTO AUTÔNOMO
void controle_pedais() {
    if (estado_atual == MODO_EMERGENCIA) return; // Trava comandos se em emergência

    if (PEDAL_ACELERADOR) {
        tempo_acelerador_pressionado++;
        
        // Se manteve pressionado por 0,5s
        if (tempo_acelerador_pressionado >= TEMPO_500MS) {
            estado_atual = MODO_MANUAL; // Cancela modo autônomo
            
            // Incrementa 4% de duty cycle (limitado a 100%)
            if (timer_05s_flag) { // Sincroniza o incremento com a base de tempo de 0.5s
                if (duty_cycle <= 96) duty_cycle += 4;
                else duty_cycle = 100;
            }
        }
    } else {
        // Lógica da Opção A: Acelerador solto rapidamente (< 300ms) ativa o piloto automático
        if (tempo_acelerador_pressionado > 0 && tempo_acelerador_pressionado < TEMPO_300MS) {
            estado_atual = MODO_AUTONOMO;
            // O duty cycle atual é mantido constante
        }
        tempo_acelerador_pressionado = 0; // Zera o contador ao soltar o pedal
    }

    if (PEDAL_FREIO) {
        estado_atual = MODO_MANUAL; // Tocar no freio desativa o piloto automático imediatamente
        
        if (timer_05s_flag) {
            if (duty_cycle >= 20) duty_cycle -= 20; 
            else duty_cycle = 0;
        }
    }
}

// LÓGICA DE CONTROLE: SEGURANÇA (OPÇÃO A)
void sistema_anti_colisao() {
    // Reduz 20% a cada 0.5s se detectar obstáculo frontal
    if (SENSOR_OBSTACULO == 1) {
        if (timer_05s_flag) {
            if (duty_cycle >= 20) duty_cycle -= 20;
            else duty_cycle = 0;
        }
    }
}

// LÓGICA DE CONTROLE: ATUAÇÃO DE POTÊNCIA
void controle_rele_potencia() {
    // O ventilador não desliga via PWM a 0%. Abaixo de 19%, cortamos a alimentação via relé.
    if (duty_cycle < 19) {
        RELE_MOTOR = 0; 
    } else {
        RELE_MOTOR = 1; 
    }
}

// LOOP PRINCIPAL (FOREGROUND SCHEDULER)
void main() {
    setup_inicial();
    
    while(1) {
        // 1. DOMÍNIO DE POTÊNCIA (Bloqueante por ~5ms)
        // Mantém a estabilidade do motor operando no range de 25kHz exigido pela Intel
        executa_pwm_burst(duty_cycle);
        
        // 2. DOMÍNIO DE TELEMETRIA E IHM (Execução em < 1ms)
        
        // Verificações que rodam a cada ciclo (aprox. a cada 5ms devido ao PWM)
        monitora_volante_e_setas();
        
        // Verificações baseadas na flag de 50ms
        if (timer_50ms_flag) {
            monitora_temperatura();
            timer_50ms_flag = 0; // Consome a flag
        }
        
        // Lógica de decisão
        controle_pedais();
        sistema_anti_colisao();
        controle_rele_potencia();
        
        // Trata eventos baseados em 1 segundo (Freio motor inercial)
        if (timer_1s_flag) {
            // Se nenhum pedal acionado e não estiver em piloto automático
            if (!PEDAL_ACELERADOR && !PEDAL_FREIO && estado_atual != MODO_AUTONOMO) {
                if (duty_cycle >= 4) duty_cycle -= 4;
                else duty_cycle = 0;
            }
            timer_1s_flag = 0; // Consome a flag
        }

        // Consome a flag de 0.5s após todas as rotinas que dependem dela terem executado
        if (timer_05s_flag) {
            timer_05s_flag = 0; 
        }
        
        // Atualiza a preparação do buffer do Display (A multiplexação real roda na ISR)
        atualiza_interface_visual(); 
    }
}
