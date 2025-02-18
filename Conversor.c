#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_PB 22
#define Botao_A 5
#define LED_R 13
#define LED_G 11
#define LED_B 12

#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

uint16_t map(uint16_t value, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main() {
    stdio_init_all();

    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB);

    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);

    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    gpio_set_function(LED_B, GPIO_FUNC_PWM);
    uint slice_R = pwm_gpio_to_slice_num(LED_R);
    uint slice_G = pwm_gpio_to_slice_num(LED_G);
    uint slice_B = pwm_gpio_to_slice_num(LED_B);
    pwm_set_enabled(slice_R, true);
    pwm_set_enabled(slice_G, true);
    pwm_set_enabled(slice_B, true);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    
    uint16_t adc_value_x, adc_value_y;
    int square_x = 50, square_y = 100;
    bool led_enable = true;
    int border_style = 0;

    while (true) {
        adc_select_input(0);
        adc_value_x = adc_read();
        adc_select_input(1);
        adc_value_y = adc_read();

        if (led_enable) {
            pwm_set_gpio_level(LED_R, map(abs(2048 - adc_value_x), 0, 2048, 0, 65535));
            pwm_set_gpio_level(LED_B, map(abs(2048 - adc_value_y), 0, 2048, 0, 65535));
        }

        square_x = map(adc_value_x, 0, 4095, 0, 118);
        square_y = map(adc_value_y, 0, 4095, 0, 54);

        static bool last_joystick_state = true;
        bool joystick_pressed = !gpio_get(JOYSTICK_PB);
        if (joystick_pressed && last_joystick_state) {
            border_style = (border_style + 1) % 3;
            pwm_set_gpio_level(LED_G, border_style == 1 ? 65535 : 0);
        }
        last_joystick_state = joystick_pressed;

        if (!gpio_get(Botao_A)) {
            led_enable = !led_enable;
            if (!led_enable) {
                pwm_set_gpio_level(LED_R, 0);
                pwm_set_gpio_level(LED_G, 0);
                pwm_set_gpio_level(LED_B, 0);
            }
        }

        ssd1306_fill(&ssd, false);
        if (border_style == 0) {
            ssd1306_rect(&ssd, 2, 2, 123, 59, true, false);
        } else if (border_style == 1) {
            ssd1306_rect(&ssd, 0, 0, 127, 8, true, false);
        } else {
            ssd1306_rect(&ssd, 4, 4, 118, 54, true, false);
        }
        ssd1306_rect(&ssd, square_x, square_y, 8, 8, true, true);
        ssd1306_send_data(&ssd);

        sleep_ms(100);
    }
}