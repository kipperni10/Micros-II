#include <xc.h>
#include "config.h"

// Importação do estado principal do veículo (definido no main.c)
extern EstadoVeiculo estado_atual;

// Variáveis de controle de tempo e estado
unsigned int contador_temp_alta = 0;
unsigned int contador_temp_critica = 0;

unsigned char seta_ativada = 0;
unsigned int timer_pisca_seta = 0;
unsigned int posicao_volante_anterior = 0;
unsigned int timer_movimento_volante = 0;

// Limiar de variação do ADC para considerar "movimento rápido"
#define LIMIAR_MOVIMENTO_RAPIDO 50 

/*
 * Função de abstração de leitura analógica.
 * Deve ser adaptada para o ADC externo ou circuito RC da sua placa.
 */
unsigned int le_adc(unsigned char canal) {
    // Retorna valor simulado
    return 25; 
}

/*
 * Executada a cada 50ms (gatilho pelo timer_50ms_flag no main)
 */
void monitora_temperatura() {
    // A leitura da temperatura do motor deve ocorrer a cada 50ms[cite: 238, 239].
    unsigned int temp_atual = le_adc(0); // Canal 0 para temperatura
    
    // Verificação de faixa ideal
    if (temp_atual > 40) {
        contador_temp_alta++;
        // Se estiver fora da faixa ideal (> 40°C) por 0,1s (duas leituras seguidas de 50ms) deve ativar LED de alerta[cite: 240].
        if (contador_temp_alta >= 2) {
            LED_ALERTA = 1;
        }
    } else {
        contador_temp_alta = 0;
        LED_ALERTA = 0;
    }
    
    // Verificação de falha crítica
    if (temp_atual >= 50) {
        contador_temp_critica++;
        // Se ficar mais de 2 minutos nesta faixa (2400 leituras x 50ms = 120s), o veículo deve parar[cite: 247, 248].
        if (contador_temp_critica >= 2400) {
            estado_atual = MODO_EMERGENCIA; // Corta o acelerador no main.c e freia o carro
        }
    } else {
        contador_temp_critica = 0;
    }
}


// Executada a cada ~1ms para capturar movimentos dinâmicos 
void monitora_volante_e_setas() {
    // Movimento do volante simulado por potenciômetro[cite: 250].
    unsigned int posicao_volante = le_adc(1); // Canal 1 para volante
    
    // Calcula a variação absoluta para detectar movimento brusco
    int variacao = posicao_volante - posicao_volante_anterior;
    if (variacao < 0) variacao = -variacao; 
    
    // Movimento rápido do volante (ida e volta por cerca de 300ms) sinaliza intenção de dobrar[cite: 255, 256].
    if (variacao > LIMIAR_MOVIMENTO_RAPIDO) {
        if (timer_movimento_volante < TEMPO_300MS) {
            // Aciona a seta ou desaciona a seta após concluir a curva/novo movimento[cite: 257, 259].
            seta_ativada = !seta_ativada;
        }
        timer_movimento_volante = 0; // Reinicia a janela de amostragem
    }
    
    posicao_volante_anterior = posicao_volante;
    
    // Incrementa o timer (evita overflow)
    if (timer_movimento_volante < 1000) {
        timer_movimento_volante++;
    }
    
    // Lógica de piscar a seta
    if (seta_ativada) {
        timer_pisca_seta++;
        // Aciona seta piscando a 1Hz (500ms ligada, 500ms desligada)[cite: 257].
        if (timer_pisca_seta < 500) {
            // LIGA PINO DA SETA (Ex: PORTAbits.RA5 = 1)
        } else if (timer_pisca_seta < 1000) {
            // DESLIGA PINO DA SETA
        } else {
            timer_pisca_seta = 0;
        }
    } else {
        timer_pisca_seta = 0;
        // GARANTE PINO DA SETA DESLIGADO
    }
}
