/*
* Esse programa cria sinais analógicos e digitais para serem verificados por um osciloscópio
*   
*                                  |EN          ________     GPIO 23|
*                                  |GPI 36     |        |    GPIO 22|
*                                  |GPI 39     | ESP32  |    GPIO  1|
*                                  |GPIO 34    |        |    GPIO  3|
*                                  |GPIO 35    |        |    GPIO 21|
*                                  |GPI 32     |        |    GPIO 19| - PWM RAMPA
*                                  |GPI 33     |________|    GPIO 18| - PWM 25% 
*                        ALEATOR - |GPIO 25                  GPIO  5|
*                        SENOIDE - |GPIO 26                  GPIO 17| - UART "OFF"
*                                  |GPIO 27                  GPIO 16|
*                                  |GPIO 14                  GPIO  4| - MORSE
*                                  |GPIO 12                  GPIO  2| - TOGGLE
*                                  |GPIO 13                  GPIO 15|
*                                  |GND                          GND| - Ponta de referência
*                                  |VIN                         3.3V| - Medição de tensão
*
*/


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_system.h"
#include <time.h>

#define GPIO_TOGGLE     2
#define GPIO_MORSE      4
#define GPIO_PWM_25     18
#define GPIO_PWM_RAMPA  19

#define UART_TXD 17
#define UART_RXD 16

#define PI 3.14159265

/* =========================================================
   TAREFA 1 – GPIO alternando a cada 0,1 s (onda quadrada 10 Hz)
   ========================================================= */
void task_toggle(void *arg)
{
    gpio_set_direction(GPIO_TOGGLE, GPIO_MODE_OUTPUT);
    int level = 0;

    while (1) {
        level = !level;
        gpio_set_level(GPIO_TOGGLE, level);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* =========================================================
   TAREFA 2 – Morse SOS (. . . _ _ _ . . .)
   ========================================================= */
void morse_dot()
{
    gpio_set_level(GPIO_MORSE, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(GPIO_MORSE, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
}

void morse_dash()
{
    gpio_set_level(GPIO_MORSE, 1);
    vTaskDelay(pdMS_TO_TICKS(600));
    gpio_set_level(GPIO_MORSE, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
}

void task_morse(void *arg)
{
    gpio_set_direction(GPIO_MORSE, GPIO_MODE_OUTPUT);

    while (1) {
        // S
        morse_dot(); morse_dot(); morse_dot();
        vTaskDelay(pdMS_TO_TICKS(600));

        // O
        morse_dash(); morse_dash(); morse_dash();
        vTaskDelay(pdMS_TO_TICKS(600));

        // S
        morse_dot(); morse_dot(); morse_dot();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* =========================================================
   TAREFA 3 – UART enviando "OFF" em ASCII
   ========================================================= */
void task_uart(void *arg)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,//115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM_2, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);

    while (1) {
        uart_write_bytes(UART_NUM_2, "OFF", 5);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* =========================================================
   TAREFA 4 – DAC com valores aleatórios
   ========================================================= */
void task_dac_random(void *arg)
{
    dac_output_enable(DAC_CHANNEL_1); // GPIO 25
    srand(time(NULL)); 
    while (1) {
        uint8_t value = rand() % 256;
        dac_output_voltage(DAC_CHANNEL_1, value);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* =========================================================
   TAREFA 5 – DAC gerando senoide
   ========================================================= */
void task_dac_sine(void *arg)
{
    dac_output_enable(DAC_CHANNEL_2); // GPIO 26
    int i = 0;

    while (1) {
        float sine = (sinf(2 * PI * i / 100) + 1) * 127;
        dac_output_voltage(DAC_CHANNEL_2, (uint8_t)sine);
        i++;
        if (i >= 100) i = 0;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* =========================================================
   TAREFA 6 – PWM fixo em 25%
   ========================================================= */
void pwm_init()
{
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 1000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t pwm25 = {
        .gpio_num   = GPIO_PWM_25,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 256, // 25% de 1023
        .hpoint     = 0
    };

    ledc_channel_config(&pwm25);
}

/* =========================================================
   TAREFA 7 – PWM em rampa
   ========================================================= */
void task_pwm_rampa(void *arg)
{
    uint32_t duty = 0;
    int step = 10;

    while (1) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

        duty += step;
        if (duty >= 1023) duty = 0;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* =========================================================
   MAIN
   ========================================================= */
void app_main(void)
{
    pwm_init();

    ledc_channel_config_t pwm_rampa = {
        .gpio_num   = GPIO_PWM_RAMPA,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&pwm_rampa);

    xTaskCreate(task_toggle,     "toggle",     2048, NULL, 1, NULL);
    xTaskCreate(task_morse,      "morse",      2048, NULL, 1, NULL);
    xTaskCreate(task_uart,       "uart",       2048, NULL, 1, NULL);
    xTaskCreate(task_dac_random, "dac_rand",   2048, NULL, 1, NULL);
    xTaskCreate(task_dac_sine,   "dac_sine",   2048, NULL, 1, NULL);
    xTaskCreate(task_pwm_rampa,  "pwm_rampa",  2048, NULL, 1, NULL);
}
