/*
 * Arquivo: display_mux.c
 * Descrição: Multiplexação não-bloqueante de Display 7 Segmentos (3 dígitos).
 */

#include "main.h"
#include "config.h"

// Variáveis externas do sistema
extern volatile uint32_t rpm_atual;
extern volatile uint32_t odometria_pulsos;

// Controle interno do display
uint8_t digito_atual = 0;
uint8_t buffer_display[3] = {0, 0, 0}; // Centenas, dezenas e unidades
uint32_t ultimo_tick_display = 0;

typedef enum {
    TELA_VELOCIDADE,
    TELA_RPM,
    TELA_HODOMETRO,
    TELA_ALERTA_TEMP
} TelaAtiva;

TelaAtiva tela_selecionada = TELA_VELOCIDADE;

// Função utilitária para converter valor em dígitos
void Converte_Para_Buffer(uint32_t valor) {
    buffer_display[2] = (valor / 100) % 10; // Centena
    buffer_display[1] = (valor / 10) % 10;  // Dezena
    buffer_display[0] = valor % 10;         // Unidade
}

// Dependendo de como seu hardware está roteado, esta função joga os 4 bits do BCD
// ou os 7 bits do display para os pinos do STM32.
void Envia_Dado_Display(uint8_t valor) {
    // Exemplo genérico: você mapeará GPIO_PIN_x conforme sua fiação
    // HAL_GPIO_WritePin(PORTA_DADOS, PINO_DADOS_A, (valor & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Atualiza_Interface_Visual() {
    uint32_t tick_atual = HAL_GetTick();

    // 1. Lógica de Navegação (Botão Função) Não-Bloqueante
    static uint8_t botao_pressionado_anteriormente = 0; // Memória de estado

    if (HAL_GPIO_ReadPin(PORT_BOTOES, PINO_BOTAO_FUNCAO) == GPIO_PIN_SET) {
        if (!botao_pressionado_anteriormente) {
            // Registra a transição de borda de subida
            botao_pressionado_anteriormente = 1;
            
            tela_selecionada++;
            if (tela_selecionada > TELA_ALERTA_TEMP) tela_selecionada = TELA_VELOCIDADE;
        }
    } else {
        // Reseta o estado apenas quando o usuário soltar o botão fisicamente
        botao_pressionado_anteriormente = 0;
    }

    // 2. Formatação do Buffer de Memória
    switch (tela_selecionada) {
        case TELA_VELOCIDADE:
            Converte_Para_Buffer((rpm_atual * 120) / 1980);
            break;
        case TELA_RPM:
            Converte_Para_Buffer(rpm_atual / 3);
            break;
        case TELA_HODOMETRO:
            Converte_Para_Buffer(((odometria_pulsos / 2) * 194) / 100000); // Em km
            break;
        case TELA_ALERTA_TEMP:
            buffer_display[2] = 0x0A; // 'A'
            buffer_display[1] = 0x0B; // 'q'
            buffer_display[0] = 0x0C; // 'u'
            break;
    }

    // 3. Multiplexação Física (Executada a cada 5ms)
    if (tick_atual - ultimo_tick_display >= 5) {
        // Desliga todos os transistores de seleção (evita ghosting)
        HAL_GPIO_WritePin(PORT_DISP_SEL, PINO_SEL_DISPLAY_0, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORT_DISP_SEL, PINO_SEL_DISPLAY_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORT_DISP_SEL_2, PINO_SEL_DISPLAY_2, GPIO_PIN_RESET);
        
        // Envia o dado para o barramento
        Envia_Dado_Display(buffer_display[digito_atual]);
        
        // Liga apenas o dígito da vez
        switch (digito_atual) {
            case 0: HAL_GPIO_WritePin(PORT_DISP_SEL, PINO_SEL_DISPLAY_0, GPIO_PIN_SET); break;
            case 1: HAL_GPIO_WritePin(PORT_DISP_SEL, PINO_SEL_DISPLAY_1, GPIO_PIN_SET); break;
            case 2: HAL_GPIO_WritePin(PORT_DISP_SEL_2, PINO_SEL_DISPLAY_2, GPIO_PIN_SET); break;
        }
        
        digito_atual++;
        if (digito_atual > 2) digito_atual = 0;
        
        ultimo_tick_display = tick_atual;
    }
}
