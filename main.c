/*
 * Arquivo: main.c
 * Placa: NUCLEO-F446RE (STM32F446RE)
 * Funcionalidade: Controle de RPM (Hardware PWM), Odometria (Hardware EXTI), 
 * Piloto Automático (Opção A) e Telemetria via ADC.
 */

#include "main.h" // Biblioteca HAL do STM32
#include "config.h"

// DEFINIÇÕES DE ESTADO DO SISTEMA
typedef enum {
    MODO_MANUAL,
    MODO_AUTONOMO,    // Piloto Automático ativado (< 300ms acelerador)
    MODO_EMERGENCIA   // Temperatura crítica (>50°C por 2 min) ou Obstáculo
} EstadoVeiculo;

EstadoVeiculo estado_atual = MODO_MANUAL;

// VARIÁVEIS GLOBAIS DE CONTROLE
volatile uint8_t duty_cycle = 0;       // 0 a 100%
volatile uint32_t rpm_atual = 0;
volatile uint32_t odometria_pulsos = 0;
volatile uint32_t pulsos_taco = 0;
uint32_t tempo_acelerador_pressionado = 0;

// Variáveis para o Escalonador Não-Bloqueante (SysTick)
uint32_t tick_atual = 0;
uint32_t timer_50ms = 0;
uint32_t timer_500ms = 0;
uint32_t timer_1s = 0;

// DECLARAÇÃO DAS FUNÇÕES EXTERNAS (Módulos)
extern void Hardware_Init(void);           // Inicializa Clocks, GPIOs, Timers e ADCs
extern void Atualiza_Interface_Visual(void);
extern void Monitora_Volante_e_Setas(void);
extern void Monitora_Temperatura(void);
extern void Set_PWM_Motor(uint8_t duty);   // Agora atua direto no registrador CCR do Timer do STM32

// LÓGICA DE CONTROLE: PEDAIS E PILOTO AUTÔNOMO
void Controle_Pedais() {
    if (estado_atual == MODO_EMERGENCIA) return;

    // Leitura das GPIOs via HAL
    if (HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_ACELERADOR) == GPIO_PIN_SET) {
        tempo_acelerador_pressionado++; // Incrementado a cada 1ms no loop
        
        if (tempo_acelerador_pressionado >= TEMPO_500MS) {
            estado_atual = MODO_MANUAL;
            
            // Incremento atrelado ao tick de 500ms
            if (tick_atual - timer_500ms >= 500) {
                if (duty_cycle <= 96) duty_cycle += 4;
                else duty_cycle = 100;
            }
        }
    } else {
        // Acelerador solto rapidamente (< 300ms) ativa piloto automático
        if (tempo_acelerador_pressionado > 0 && tempo_acelerador_pressionado < TEMPO_300MS) {
            estado_atual = MODO_AUTONOMO;
        }
        tempo_acelerador_pressionado = 0;
    }

    if (HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_FREIO) == GPIO_PIN_SET) {
        estado_atual = MODO_MANUAL; 
        
        if (tick_atual - timer_500ms >= 500) {
            if (duty_cycle >= 20) duty_cycle -= 20; 
            else duty_cycle = 0;
        }
    }
}

// LÓGICA DE CONTROLE: SEGURANÇA (OPÇÃO A)
void Sistema_Anti_Colisao() {
    if (HAL_GPIO_ReadPin(PORT_SENSORES, PINO_OBSTACULO) == GPIO_PIN_SET) {
        if (tick_atual - timer_500ms >= 500) {
            if (duty_cycle >= 20) duty_cycle -= 20;
            else duty_cycle = 0;
        }
    }
}

// LÓGICA DE CONTROLE: ATUAÇÃO DE POTÊNCIA
void Controle_Rele_Potencia() {
    if (duty_cycle < 19) {
        HAL_GPIO_WritePin(PORT_RELE, PINO_RELE, GPIO_PIN_RESET); // Desliga 12V
    } else {
        HAL_GPIO_WritePin(PORT_RELE, PINO_RELE, GPIO_PIN_SET);   // Liga 12V
    }
}

// LOOP PRINCIPAL (FOREGROUND SCHEDULER)
int main(void) {
    // Inicialização da HAL e do Hardware interno do STM32 (Clocks, Timers, ADC)
    Hardware_Init();
    
    while (1) {
        tick_atual = HAL_GetTick(); // Captura o tempo atual do sistema em milissegundos
        
        // 1. ATUAÇÃO DO HARDWARE (Instantâneo)
        Set_PWM_Motor(duty_cycle); // Altera o registrador de hardware (CCR). O sinal contínuo de 25kHz é mantido pelo Timer do STM32.
        
        // 2. TAREFAS DE ALTA FREQUÊNCIA (~1ms)
        Monitora_Volante_e_Setas();
        Atualiza_Interface_Visual(); // A multiplexação pode continuar aqui ou ir para um Timer Básico (TIM6)
        
        // 3. TAREFAS DE MÉDIA FREQUÊNCIA (50ms)
        if (tick_atual - timer_50ms >= 50) {
            Monitora_Temperatura();
            timer_50ms = tick_atual;
        }
        
        // 4. LÓGICA DE DECISÃO (Requer avaliação contínua)
        Controle_Pedais();
        Sistema_Anti_Colisao();
        Controle_Rele_Potencia();
        
        // Sincroniza o timer de 500ms usado nos pedais e colisão
        if (tick_atual - timer_500ms >= 500) {
            timer_500ms = tick_atual;
        }
        
        // 5. TAREFAS DE BAIXA FREQUÊNCIA (1s)
        if (tick_atual - timer_1s >= 1000) {
            
            // --- CÁLCULO DE RPM E ODOMETRIA ---
            extern volatile uint32_t pulsos_taco;
            
            // O fan gera 2 pulsos por revolução. RPM = (pulsos em 1s * 60) / 2
            rpm_atual = (pulsos_taco * 60) / 2; 
            pulsos_taco = 0; // Zera a janela de leitura para o próximo segundo
            
            // FREIO MOTOR INERCIAL
            if (HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_ACELERADOR) == GPIO_PIN_RESET && 
                HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_FREIO) == GPIO_PIN_RESET && 
                estado_atual != MODO_AUTONOMO) {
                
                if (duty_cycle >= 4) duty_cycle -= 4;
                else duty_cycle = 0;
            }
            timer_1s = tick_atual;
        }
    }
}

// Callback invocado automaticamente pelo hardware quando o pino do tacômetro muda de estado
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == PINO_TACOMETRO) { // Pino configurado no CubeMX
        extern volatile uint32_t pulsos_taco;
        extern volatile uint32_t odometria_pulsos;
        
        pulsos_taco++;
        odometria_pulsos++;
    }
}
