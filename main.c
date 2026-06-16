/*
 * Arquivo: main.c (Apenas as secções de código do utilizador)
 * Instruções: Cole cada bloco dentro da respetiva secção 'USER CODE' 
 * gerada automaticamente pelo STM32CubeMX no seu main.c.
 */

/* USER CODE BEGIN Includes */
#include "config.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// VARIÁVEIS GLOBAIS DE CONTROLE
volatile uint8_t duty_cycle = 0;       // 0 a 100%
volatile uint32_t rpm_atual = 0;
volatile uint32_t odometria_pulsos = 0;
volatile uint32_t pulsos_taco = 0;     // CORREÇÃO 1: Variável definida fisicamente e instanciada
uint32_t tempo_acelerador_pressionado = 0;

// Variáveis para o Escalonador Não-Bloqueante (SysTick)
uint32_t tick_atual = 0;
uint32_t timer_50ms = 0;
uint32_t timer_500ms = 0;
uint32_t timer_1s = 0;

typedef enum {
    MODO_MANUAL,
    MODO_AUTONOMO,    // Piloto Automático ativado (< 300ms acelerador)
    MODO_EMERGENCIA   // Temperatura crítica (>50°C por 2 min) ou Obstáculo
} EstadoVeiculo;

EstadoVeiculo estado_atual = MODO_MANUAL;
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
// DECLARAÇÃO DAS FUNÇÕES EXTERNAS (Módulos)
extern void Atualiza_Interface_Visual(void);
extern void Monitora_Volante_e_Setas(void);
extern void Monitora_Temperatura(void);
extern void Set_PWM_Motor(uint8_t duty);

// LÓGICA DE CONTROLE: PEDAIS E PILOTO AUTÔNOMO
void Controle_Pedais() {
    if (estado_atual == MODO_EMERGENCIA) return;

    if (HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_ACELERADOR) == GPIO_PIN_SET) {
        tempo_acelerador_pressionado++; 
        
        if (tempo_acelerador_pressionado >= TEMPO_500MS) {
            estado_atual = MODO_MANUAL;
            if (tick_atual - timer_500ms >= 500) {
                if (duty_cycle <= 96) duty_cycle += 4;
                else duty_cycle = 100;
            }
        }
    } else {
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
        HAL_GPIO_WritePin(PORT_RELE, PINO_RELE, GPIO_PIN_RESET); 
    } else {
        HAL_GPIO_WritePin(PORT_RELE, PINO_RELE, GPIO_PIN_SET);   
    }
}
/* USER CODE END 0 */


// DENTRO DA FUNÇÃO main() QUE A IDE GERA:
// ... (inicializações da HAL geradas pelo sistema) ...

  /* USER CODE BEGIN 3 */
  while (1) {
        tick_atual = HAL_GetTick(); 
        
        // 1. ATUAÇÃO DO HARDWARE (Instantâneo)
        Set_PWM_Motor(duty_cycle); 
        
        // 2. TAREFAS DE ALTA FREQUÊNCIA (~1ms)
        Monitora_Volante_e_Setas();
        Atualiza_Interface_Visual(); 
        
        // 3. TAREFAS DE MÉDIA FREQUÊNCIA (50ms)
        if (tick_atual - timer_50ms >= 50) {
            Monitora_Temperatura();
            timer_50ms = tick_atual;
        }
        
        // 4. LÓGICA DE DECISÃO 
        Controle_Pedais();
        Sistema_Anti_Colisao();
        Controle_Rele_Potencia();
        
        if (tick_atual - timer_500ms >= 500) {
            timer_500ms = tick_atual;
        }
        
        // 5. TAREFAS DE BAIXA FREQUÊNCIA (1s)
        if (tick_atual - timer_1s >= 1000) {
            
            // --- CÁLCULO DE RPM E ODOMETRIA ---
            rpm_atual = (pulsos_taco * 60) / 2; 
            pulsos_taco = 0; // Zera a janela de leitura 
            
            // --- FREIO MOTOR INERCIAL ---
            if (HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_ACELERADOR) == GPIO_PIN_RESET && 
                HAL_GPIO_ReadPin(PORT_PEDAIS, PINO_FREIO) == GPIO_PIN_RESET && 
                estado_atual != MODO_AUTONOMO) {
                
                if (duty_cycle >= 4) duty_cycle -= 4;
                else duty_cycle = 0;
            }
            timer_1s = tick_atual;
        }
  }
  /* USER CODE END 3 */


/* USER CODE BEGIN 4 */
// Callback invocado automaticamente pelo hardware (EXTI)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == PINO_TACOMETRO) { 
        // As variáveis já são globais e visíveis pelo main.c
        pulsos_taco++;
        odometria_pulsos++;
    }
}
/* USER CODE END 4 */
