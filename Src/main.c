/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Projeto 2 - Henrique, Bianca, Nicolas
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
  TELA_VELOCIDADE = 0,
  TELA_RPM,
  TELA_TEMP,
  TELA_ODO,
  TELA_MARCHA,
  TELA_DUTY,
  TELA_TOTAL
} Tela_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// sinais do display
#define SEG_ON                 GPIO_PIN_RESET
#define SEG_OFF                GPIO_PIN_SET
#define DIG_ON                 GPIO_PIN_RESET
#define DIG_OFF                GPIO_PIN_SET

// botoes ligados com pull-up interno
#define BTN_PRESSED            GPIO_PIN_RESET

// transistor externo da alimentacao do cooler
#define POWER_ON               GPIO_PIN_SET
#define POWER_OFF              GPIO_PIN_RESET

// canal PWM usado no cooler
#define FAN_PWM_CHANNEL        TIM_CHANNEL_3

// parametros de controle do cooler
#define DUTY_MIN_ON            19U
#define DUTY_MAX               100U
#define DUTY_ACCEL_STEP        4U
#define DUTY_BRAKE_STEP        20U
#define DUTY_OBSTACLE_STEP     20U
#define DUTY_NATURAL_STEP      4U

#define ACCEL_INTERVAL_MS      500U
#define BRAKE_INTERVAL_MS      500U
#define OBSTACLE_INTERVAL_MS   500U
#define NATURAL_INTERVAL_MS    1000U
#define AUTO_INTERVAL_MS       500U

#define FAN_START_KICK_MS      1500U

// leitura do tacometro do cooler
#define TACH_PULSES_PER_REV    2U
#define RPM_MAX_FOR_120_KMH    3000U

// limites de temperatura
#define TEMP_READ_INTERVAL_MS  200U
#define TEMP_ALERT_C           40
#define TEMP_CRITICAL_C        50
#define TEMP_CRITICAL_MS       120000UL

// limites do potenciometro usado como volante
#define STEER_CENTER           2048
#define STEER_CURVE_LIMIT      900
#define STEER_FAST_DELTA       1200
#define STEER_RETURN_BAND      350
#define STEER_FAST_TIME_MS     300U
#define CURVE_DUTY_LIMIT       100U
#define CURVE_SPEED_LIMIT      90U

// sensor de obstaculo
#define OBSTACLE_DETECTED      GPIO_PIN_RESET

// leds indicadores
#define LED_ON                 GPIO_PIN_SET
#define LED_OFF                GPIO_PIN_RESET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// tabela dos numeros no display de 7 segmentos
static const uint8_t digit_map[10] =
{
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};

#define PATTERN_BLANK 0x00
#define PATTERN_C     0x39
#define PATTERN_A     0x77
#define PATTERN_Q     0x67
#define PATTERN_U     0x1C

static uint8_t disp[3] = {0x00, 0x00, 0x3F};
static uint8_t ativo = 0;

static Tela_t tela = TELA_VELOCIDADE;

// variaveis principais
static uint8_t duty = 0;
static uint8_t marcha = 0;
static uint8_t auto_on = 0;
static uint8_t auto_target_duty = 0;
static uint16_t auto_target_speed = 0;

// variaveis de medicao
static int16_t temp_val = 0;
static uint8_t temp_alert = 0;
static uint8_t temp_critical = 0;
static uint8_t critical_lock = 0;
static uint32_t temp_critical_start = 0;

static volatile uint32_t tach_pulses = 0;
static uint16_t tach_per_sec = 0;
static uint16_t rpm = 0;
static uint16_t velocidade = 0;

static uint32_t odometro_dm = 0;

static uint16_t centro_volante = STEER_CENTER;
static uint16_t steer_now = STEER_CENTER;
static uint16_t steer_prev = STEER_CENTER;
static uint8_t curva = 0;
static uint8_t seta_on = 0;
static uint8_t seta_blink = 0;
static uint8_t movimento_volante = 0;
static int8_t lado_movimento = 0;
static uint32_t inicio_movimento = 0;
static uint8_t curva_feita = 0;

// estados anteriores dos botoes
static uint8_t func_prev = 0;
static uint8_t sel_prev = 0;

// controle de partida do cooler
static uint8_t fan_ligado = 0;
static uint32_t fan_kick_until = 0;

// marcadores de tempo das tarefas
static uint32_t t_display = 0;
static uint32_t t_inputs = 0;
static uint32_t t_accel = 0;
static uint32_t t_brake = 0;
static uint32_t t_obstacle = 0;
static uint32_t t_natural = 0;
static uint32_t t_auto = 0;
static uint32_t t_adc = 0;
static uint32_t t_temp = 0;
static uint32_t t_tach = 0;
static uint32_t t_odo = 0;
static uint32_t t_blink = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

static void configurar_pinos(void);

static void escrever_segmentos(uint8_t v);
static void desligar_digitos(void);
static void mostrar_numero(uint16_t n, uint8_t blank_left);
static void mostrar_temperatura(uint16_t t);
static void mostrar_texto(uint8_t a, uint8_t b, uint8_t c);
static void atualizar_display(void);
static void atualizar_conteudo_display(void);

static uint16_t ler_adc(uint32_t channel);
static int16_t ler_temperatura(void);

static void ajustar_duty_cooler(uint8_t percent);
static void aplicar_saida_cooler(void);

static uint8_t botao_pressionado(GPIO_TypeDef *port, uint16_t pin);
static void aumentar_duty(uint8_t step);
static void reduzir_duty(uint8_t step);
static void atualizar_marcha(void);
static uint16_t velocidade_para_painel(void);

static void calibrar_volante(void);

static void tarefa_botoes(void);
static void tarefa_volante(void);
static void tarefa_temperatura(void);
static void tarefa_tach(void);
static void tarefa_logica(void);
static void tarefa_odometro(void);
static void tarefa_leds(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void configurar_pinos(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  // configura os segmentos
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_HIGH;

  g.Pin = GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_10 | GPIO_PIN_6;
  HAL_GPIO_Init(GPIOB, &g);

  g.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  HAL_GPIO_Init(GPIOA, &g);

  g.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOC, &g);

  // configura os digitos em open-drain
  g.Mode = GPIO_MODE_OUTPUT_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_HIGH;

  g.Pin = GPIO_PIN_7 | GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &g);

  g.Pin = GPIO_PIN_9;
  HAL_GPIO_Init(GPIOB, &g);

  desligar_digitos();
  escrever_segmentos(0x00);

  // configura os botoes
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_LOW;

  // PC1 = funcao, PC0 = seleciona
  g.Pin = GPIO_PIN_1 | GPIO_PIN_0;
  HAL_GPIO_Init(GPIOC, &g);

  // PA4 = acelerador
  g.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOA, &g);

  // PB0 = freio
  g.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOB, &g);

  // sensor de obstaculo PA1
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  g.Pin = GPIO_PIN_1;
  HAL_GPIO_Init(GPIOA, &g);

  // leds indicadores PA5 temp, PA2 seta, PA3 auto
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Pin = GPIO_PIN_5 | GPIO_PIN_2 | GPIO_PIN_3;
  HAL_GPIO_Init(GPIOA, &g);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, LED_OFF);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, LED_OFF);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, LED_OFF);

  // leitura do tacometro do cooler PB3 EXTI - igual ao codigo que funcionou: pull-up interno
  g.Mode = GPIO_MODE_IT_FALLING;
  g.Pull = GPIO_PULLUP;
  g.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOB, &g);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  // PB8 aciona o transistor externo
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Pin = GPIO_PIN_8;
  HAL_GPIO_Init(GPIOB, &g);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, POWER_OFF);

  // PA10 gera o PWM do cooler
  g.Mode = GPIO_MODE_AF_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  g.Alternate = GPIO_AF1_TIM1;
  g.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOA, &g);
}

static void escrever_segmentos(uint8_t v)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  (v & 0x01) ? SEG_ON : SEG_OFF);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  (v & 0x02) ? SEG_ON : SEG_OFF);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, (v & 0x04) ? SEG_ON : SEG_OFF);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,  (v & 0x08) ? SEG_ON : SEG_OFF);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9,  (v & 0x10) ? SEG_ON : SEG_OFF);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7,  (v & 0x20) ? SEG_ON : SEG_OFF);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6,  (v & 0x40) ? SEG_ON : SEG_OFF);
}

static void desligar_digitos(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, DIG_OFF);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, DIG_OFF);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, DIG_OFF);
}

static void mostrar_texto(uint8_t a, uint8_t b, uint8_t c)
{
  disp[0] = a;
  disp[1] = b;
  disp[2] = c;
}

static void mostrar_numero(uint16_t n, uint8_t blank_left)
{
  if (n > 999)
  {
    n = 999;
  }

  uint8_t h = n / 100;
  uint8_t t = (n / 10) % 10;
  uint8_t u = n % 10;

  if (blank_left && n < 100)
  {
    disp[0] = PATTERN_BLANK;
  }
  else
  {
    disp[0] = digit_map[h];
  }

  if (blank_left && n < 10)
  {
    disp[1] = PATTERN_BLANK;
  }
  else
  {
    disp[1] = digit_map[t];
  }

  disp[2] = digit_map[u];
}

static void mostrar_temperatura(uint16_t t)
{
  if (t > 99)
  {
    t = 99;
  }

  disp[0] = digit_map[t / 10];
  disp[1] = digit_map[t % 10];
  disp[2] = PATTERN_C;
}

static void atualizar_display(void)
{
  desligar_digitos();
  escrever_segmentos(0x00);

  for (volatile uint16_t i = 0; i < 80; i++)
  {
  }

  escrever_segmentos(disp[ativo]);

  if (ativo == 0)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, DIG_ON);
  }
  else if (ativo == 1)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, DIG_ON);
  }
  else
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, DIG_ON);
  }

  ativo++;
  if (ativo >= 3)
  {
    ativo = 0;
  }
}

static void atualizar_conteudo_display(void)
{
  // alerta de temperatura tem prioridade no display
  if (temp_critical)
  {
    mostrar_texto(PATTERN_A, PATTERN_Q, PATTERN_U); // Aqu
    return;
  }

  if (temp_alert)
  {
    mostrar_temperatura((uint16_t)(temp_val < 0 ? 0 : temp_val));
    return;
  }

  switch (tela)
  {
    case TELA_VELOCIDADE:
      // mostra a velocidade calculada
      mostrar_numero(velocidade_para_painel(), 1);
      break;

    case TELA_RPM:
      // mostra RPM dividido por 10
      // exemplo: 245 representa 2450 rpm
      mostrar_numero(rpm / 10U, 1);
      break;

    case TELA_TEMP:
      // limites de temperatura com C no terceiro digito.
      mostrar_temperatura((uint16_t)(temp_val < 0 ? 0 : temp_val));
      break;

    case TELA_ODO:
      // mostra hodometro em metros
      mostrar_numero((odometro_dm / 10U) % 1000U, 1);
      break;

    case TELA_MARCHA:
      mostrar_numero(marcha, 1);
      break;

    case TELA_DUTY:
      // mostra o duty do PWM
      mostrar_numero(duty, 1);
      break;

    default:
      mostrar_numero(velocidade_para_painel(), 1);
      break;
  }
}

static uint16_t ler_adc(uint32_t channel)
{
  ADC_ChannelConfTypeDef c = {0};
  uint16_t v = 0;

  c.Channel = channel;
  c.Rank = 1;
  c.SamplingTime = ADC_SAMPLETIME_480CYCLES;

  HAL_ADC_ConfigChannel(&hadc1, &c);

  HAL_ADC_Start(&hadc1);

  if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK)
  {
    v = HAL_ADC_GetValue(&hadc1);
  }

  HAL_ADC_Stop(&hadc1);

  return v;
}

static int16_t ler_temperatura(void)
{
  uint16_t raw;
  int32_t mv;
  int32_t temp;

  ADC->CCR |= ADC_CCR_TSVREFE;

  raw = ler_adc(ADC_CHANNEL_TEMPSENSOR);

  mv = ((int32_t)raw * 3300) / 4095;
  temp = ((mv - 760) * 10) / 25 + 25;

  return (int16_t)temp;
}

static void ajustar_duty_cooler(uint8_t percent)
{
  uint32_t period;
  uint32_t pulse;

  if (percent > 100)
  {
    percent = 100;
  }

  period = __HAL_TIM_GET_AUTORELOAD(&htim1);

  pulse = ((uint32_t)percent * (period + 1U)) / 100U;

  if (pulse > (period + 1U))
  {
    pulse = period + 1U;
  }

  __HAL_TIM_SET_COMPARE(&htim1, FAN_PWM_CHANNEL, pulse);
}

static void aplicar_saida_cooler(void)
{
  uint32_t now = HAL_GetTick();

  if (duty < DUTY_MIN_ON || critical_lock)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, POWER_OFF);
    ajustar_duty_cooler(0);
    fan_ligado = 0;
    fan_kick_until = 0;
    return;
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, POWER_ON);

  if (fan_ligado == 0)
  {
    fan_ligado = 1;
    fan_kick_until = now + FAN_START_KICK_MS;
    ajustar_duty_cooler(100);
    return;
  }

  if ((int32_t)(now - fan_kick_until) < 0)
  {
    ajustar_duty_cooler(100);
  }
  else
  {
    ajustar_duty_cooler(duty);
  }
}

static uint8_t botao_pressionado(GPIO_TypeDef *port, uint16_t pin)
{
  return (HAL_GPIO_ReadPin(port, pin) == BTN_PRESSED) ? 1U : 0U;
}

static void calibrar_volante(void)
{
  uint32_t soma = 0;

  // le algumas vezes no inicio e assume que esta posicao eh o centro
  for (uint8_t i = 0; i < 20; i++)
  {
    soma += ler_adc(ADC_CHANNEL_0);
    HAL_Delay(5);
  }

  centro_volante = (uint16_t)(soma / 20U);
  steer_now = centro_volante;
  steer_prev = centro_volante;
}

static uint16_t velocidade_para_painel(void)
{
  uint16_t v = velocidade;

  // se o tacometro ainda nao respondeu, estima a velocidade pelo duty
  if (v == 0U && duty >= DUTY_MIN_ON)
  {
    v = (uint16_t)(((uint32_t)duty * 120U) / 100U);
  }

  // em curva, a velocidade maxima exibida e considerada eh 90 km/h
  if (curva && v > CURVE_SPEED_LIMIT)
  {
    v = CURVE_SPEED_LIMIT;
  }

  return v;
}

static void aumentar_duty(uint8_t step)
{
  if (duty > (DUTY_MAX - step))
  {
    duty = DUTY_MAX;
  }
  else
  {
    duty += step;
  }
}

static void reduzir_duty(uint8_t step)
{
  if (duty < step)
  {
    duty = 0;
  }
  else
  {
    duty -= step;
  }
}

static void atualizar_marcha(void)
{
  if (duty < DUTY_MIN_ON)
  {
    marcha = 0;
  }
  else if (duty <= 35)
  {
    marcha = 1;
  }
  else if (duty <= 50)
  {
    marcha = 2;
  }
  else if (duty <= 65)
  {
    marcha = 3;
  }
  else if (duty <= 80)
  {
    marcha = 4;
  }
  else
  {
    marcha = 5;
  }
}

static void tarefa_botoes(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - t_inputs) < 20U)
  {
    return;
  }

  t_inputs = now;

  uint8_t func_now = botao_pressionado(GPIOC, GPIO_PIN_1);
  uint8_t sel_now = botao_pressionado(GPIOC, GPIO_PIN_0);

  if (func_now && !func_prev)
  {
    tela = (Tela_t)((tela + 1) % TELA_TOTAL);
  }

  // seleciona liga ou desliga velocidade autonoma
  if (sel_now && !sel_prev)
  {
    if (auto_on)
    {
      auto_on = 0;
    }
    else if (!critical_lock && duty >= DUTY_MIN_ON)
    {
      auto_on = 1;
      auto_target_duty = duty;
      auto_target_speed = velocidade;
    }
  }

  func_prev = func_now;
  sel_prev = sel_now;
}

static void tarefa_volante(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - t_adc) < 100U)
  {
    return;
  }

  t_adc = now;

  steer_prev = steer_now;
  steer_now = ler_adc(ADC_CHANNEL_0);

  int32_t diff = (int32_t)steer_now - (int32_t)centro_volante;

  if (diff > STEER_CURVE_LIMIT || diff < -STEER_CURVE_LIMIT)
  {
    curva = 1;
  }
  else
  {
    curva = 0;
  }

  // detecta movimento rapido: sai do centro e volta em ate 300 ms
  int8_t lado_atual = 0;

  if (diff > STEER_FAST_DELTA)
  {
    lado_atual = 1;
  }
  else if (diff < -STEER_FAST_DELTA)
  {
    lado_atual = -1;
  }

  if (movimento_volante == 0)
  {
    if (lado_atual != 0)
    {
      movimento_volante = 1;
      lado_movimento = lado_atual;
      inicio_movimento = now;
    }
  }
  else
  {
    uint8_t voltou_centro = 0;

    if (diff < STEER_RETURN_BAND && diff > -STEER_RETURN_BAND)
    {
      voltou_centro = 1;
    }

    if ((now - inicio_movimento) > STEER_FAST_TIME_MS)
    {
      movimento_volante = 0;
      lado_movimento = 0;
    }
    else if (voltou_centro)
    {
      // novo movimento rapido alterna a seta
      seta_on = !seta_on;
      curva_feita = 0;
      movimento_volante = 0;
      lado_movimento = 0;
    }
  }

  // depois da curva, ao voltar para o centro, a seta desliga
  if (seta_on)
  {
    if (curva)
    {
      curva_feita = 1;
    }

    if (curva_feita && diff < STEER_RETURN_BAND && diff > -STEER_RETURN_BAND)
    {
      seta_on = 0;
      curva_feita = 0;
    }
  }
  else
  {
    curva_feita = 0;
  }
}


static void tarefa_temperatura(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - t_temp) < TEMP_READ_INTERVAL_MS)
  {
    return;
  }

  t_temp = now;

  temp_val = ler_temperatura();

  temp_alert = (temp_val > TEMP_ALERT_C) ? 1U : 0U;

  if (temp_val >= TEMP_CRITICAL_C)
  {
    temp_critical = 1;

    if (temp_critical_start == 0)
    {
      temp_critical_start = now;
    }

    if ((now - temp_critical_start) >= TEMP_CRITICAL_MS)
    {
      critical_lock = 1;
      auto_on = 0;
      duty = 0;
    }
  }
  else
  {
    temp_critical = 0;
    temp_critical_start = 0;
    critical_lock = 0;
  }
}

static void tarefa_tach(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - t_tach) < 1000U)
  {
    return;
  }

  t_tach = now;

  uint32_t pulses;

  __disable_irq();
  pulses = tach_pulses;
  tach_pulses = 0;
  __enable_irq();

  tach_per_sec = (uint16_t)pulses;

  rpm = (uint16_t)((pulses * 60U) / TACH_PULSES_PER_REV);

  if (rpm >= RPM_MAX_FOR_120_KMH)
  {
    velocidade = 120;
  }
  else
  {
    velocidade = (uint16_t)(((uint32_t)rpm * 120U) / RPM_MAX_FOR_120_KMH);
  }
}

static void tarefa_logica(void)
{
  uint32_t now = HAL_GetTick();

  uint8_t accel_now = botao_pressionado(GPIOA, GPIO_PIN_4);
  uint8_t brake_now = botao_pressionado(GPIOB, GPIO_PIN_0);
  uint8_t obstacle_now = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == OBSTACLE_DETECTED) ? 1U : 0U;

  // freio tem prioridade
  if (brake_now)
  {
    auto_on = 0;

    if ((now - t_brake) >= BRAKE_INTERVAL_MS)
    {
      t_brake = now;
      reduzir_duty(DUTY_BRAKE_STEP);
    }
  }
  else
  {
    t_brake = now;
  }

  // obstaculo reduz a velocidade
  if (obstacle_now)
  {
    auto_on = 0;

    if ((now - t_obstacle) >= OBSTACLE_INTERVAL_MS)
    {
      t_obstacle = now;
      reduzir_duty(DUTY_OBSTACLE_STEP);
    }
  }
  else
  {
    t_obstacle = now;
  }

  // acelerador aumenta o duty
  if (accel_now && !brake_now && !auto_on && !critical_lock)
  {
    if ((now - t_accel) >= ACCEL_INTERVAL_MS)
    {
      t_accel = now;
      aumentar_duty(DUTY_ACCEL_STEP);
    }
  }
  else
  {
    t_accel = now;
  }

  // controle autonomo tenta manter a velocidade
  if (auto_on && !brake_now && !critical_lock)
  {
    if ((now - t_auto) >= AUTO_INTERVAL_MS)
    {
      t_auto = now;

      if (velocidade == 0 && auto_target_duty >= DUTY_MIN_ON)
      {
        duty = auto_target_duty;
      }
      else if (velocidade + 3U < auto_target_speed)
      {
        aumentar_duty(2);
      }
      else if (velocidade > auto_target_speed + 3U)
      {
        reduzir_duty(2);
      }
    }
  }
  else
  {
    t_auto = now;
  }

  // sem acelerar, o carro desacelera
  if (!accel_now && !brake_now && !auto_on)
  {
    if ((now - t_natural) >= NATURAL_INTERVAL_MS)
    {
      t_natural = now;
      reduzir_duty(DUTY_NATURAL_STEP);
    }
  }
  else
  {
    t_natural = now;
  }

  // em curva, nao deixa passar da velocidade maxima definida
  if (curva && velocidade_para_painel() > CURVE_SPEED_LIMIT)
  {
    reduzir_duty(DUTY_NATURAL_STEP);
  }

  atualizar_marcha();
}

static void tarefa_odometro(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - t_odo) < 1000U)
  {
    return;
  }

  t_odo = now;

  // converte velocidade para distancia aproximada
  odometro_dm += ((uint32_t)velocidade_para_painel() * 25U) / 9U;
}

static void tarefa_leds(void)
{
  uint32_t now = HAL_GetTick();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (temp_alert || temp_critical) ? LED_ON : LED_OFF);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, auto_on ? LED_ON : LED_OFF);

  if (seta_on)
  {
    if ((now - t_blink) >= 500U)
    {
      t_blink = now;
      seta_blink = !seta_blink;
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, seta_blink ? LED_ON : LED_OFF);
  }
  else
  {
    seta_blink = 0;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, LED_OFF);
  }
}

void EXTI3_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_3)
  {
    tach_pulses++;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_TIM6_Init();
  MX_TIM1_Init();

  /* USER CODE BEGIN 2 */

  configurar_pinos();

  HAL_TIM_PWM_Start(&htim1, FAN_PWM_CHANNEL);
  __HAL_TIM_MOE_ENABLE(&htim1);
  __HAL_TIM_SET_COMPARE(&htim1, FAN_PWM_CHANNEL, 0);

  ADC->CCR |= ADC_CCR_TSVREFE;

  calibrar_volante();

  duty = 0;
  tela = TELA_VELOCIDADE;

  ajustar_duty_cooler(0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, POWER_OFF);

  mostrar_numero(0, 1);

  t_display = 0;
  t_inputs = 0;
  t_accel = HAL_GetTick();
  t_brake = HAL_GetTick();
  t_obstacle = HAL_GetTick();
  t_natural = HAL_GetTick();
  t_auto = HAL_GetTick();
  t_adc = 0;
  t_temp = 0;
  t_tach = 0;
  t_odo = HAL_GetTick();
  t_blink = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if ((HAL_GetTick() - t_display) >= 5U)
    {
      t_display = HAL_GetTick();
      atualizar_display();
    }

    tarefa_botoes();
    tarefa_volante();
    tarefa_temperatura();
    tarefa_tach();
    tarefa_logica();
    tarefa_odometro();
    tarefa_leds();

    aplicar_saida_cooler();
    atualizar_conteudo_display();
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
