#include <xc.h>
#include "config.h"

// Variáveis externas do sistema
extern volatile unsigned int rpm_atual;
extern volatile unsigned char duty_cycle;
extern volatile unsigned long odometria_pulsos;

// Controle interno do display
unsigned char digito_atual = 0;
unsigned char buffer_display[3] = {0, 0, 0}; // Armazena as centenas, dezenas e unidades

// Enumeração das telas possíveis pelo Botão Função
typedef enum {
    TELA_VELOCIDADE,
    TELA_RPM,
    TELA_HODOMETRO,
    TELA_ALERTA_TEMP
} TelaAtiva;

TelaAtiva tela_selecionada = TELA_VELOCIDADE;

// Função utilitária para converter um valor numérico em dígitos separados no buffer
void converte_para_buffer(unsigned int valor) {
    buffer_display[2] = (valor / 100) % 10; // Centena
    buffer_display[1] = (valor / 10) % 10;  // Dezena
    buffer_display[0] = valor % 10;         // Unidade
}

// Chamada no loop principal para formatar o que será exibido
void atualiza_interface_visual() {
    // A navegação entre as telas é feita pelo botão FUNÇÃO
    if (BOTAO_FUNCAO) {
        __delay_ms(50); // Debounce básico
        if (BOTAO_FUNCAO) {
            tela_selecionada++;
            if (tela_selecionada > TELA_ALERTA_TEMP) {
                tela_selecionada = TELA_VELOCIDADE;
            }
            while(BOTAO_FUNCAO); // Aguarda soltar o botão
        }
    }

    // Alimenta o buffer com o dado correto baseado na tela ativa
    switch (tela_selecionada) {
        case TELA_VELOCIDADE:
            // Regra de 3 simples: RPM Máximo (aprox. 1980) equivale a 120 km/h
            unsigned int velocidade = (rpm_atual * 120) / 1980;
            converte_para_buffer(velocidade);
            break;
            
        case TELA_RPM:
            // Requisito do projeto: exibir rotação / 3 devido ao display de 3 dígitos
            converte_para_buffer(rpm_atual / 3);
            break;
            
        case TELA_HODOMETRO:
            // Cálculo: (Pulsos Totais / Pulsos por Revolução) * Perímetro da roda
            // Assumindo pneu aro 15 com diâmetro ~62cm -> Perímetro ~1.94m
            unsigned long revolucoes = odometria_pulsos / 2;
            unsigned int distancia_metros = (revolucoes * 194) / 100;
            unsigned int distancia_km = distancia_metros / 1000;
            converte_para_buffer(distancia_km);
            break;
            
        case TELA_ALERTA_TEMP:
            // Se houver uma rotina de leitura do ADC gerando temperatura alta:
            // A matriz para formar "Aqu" dependerá da tabela do decodificador BCD ou 7seg.
            // Aqui representamos com códigos brutos fictícios para A, q, u
            buffer_display[2] = 0x0A; // 'A'
            buffer_display[1] = 0x0B; // 'q'
            buffer_display[0] = 0x0C; // 'u'
            break;
    }
}

// Rotina engatilhada pela ISR do Timer0 para chavear os transistores do display
void alterna_digito_display() {
    // Desliga todos os displays (evita sombreamento/ghosting)
    SEL_DISPLAY_0 = 0;
    SEL_DISPLAY_1 = 0;
    SEL_DISPLAY_2 = 0;
    
    // Joga o dado no barramento
    // O valor do buffer (0 a 9) deve ser enviado para os pinos corretos
    // Exemplo genérico enviando os 4 bits menos significativos para o PORTB
    PORTA_DADOS_DISP = (PORTA_DADOS_DISP & 0xF0) | (buffer_display[digito_atual] & 0x0F);
    
    // Liga apenas o display correspondente à iteração atual
    switch (digito_atual) {
        case 0: SEL_DISPLAY_0 = 1; break;
        case 1: SEL_DISPLAY_1 = 1; break;
        case 2: SEL_DISPLAY_2 = 1; break;
    }
    
    // Prepara para o próximo dígito na próxima chamada
    digito_atual++;
    if (digito_atual > 2) digito_atual = 0;
}
