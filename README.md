/computador-de-bordo-pic
│
├── /docs                 # Datasheets, especificações do Intel Fan, esquemáticos
├── /hardware             # Diagramas do painel e pinagem
│
├── /src                  # Firmware em C
│   ├── main.c            # Loop principal e máquina de estados
│   ├── isr.c             # Rotinas de Serviço de Interrupção (TMR0 e INT)
│   ├── pwm_soft.c        # Geração de PWM via software (25kHz)
│   ├── display_mux.c     # Varredura do display de 7 segmentos (60Hz)
│   ├── sensores.c        # Leitura dos pedais, botões e sensor de proximidade
│   └── config.h          # Mapeamento de pinos e macros de configuração
│
└── README.md             # Instruções de compilação e deploy
