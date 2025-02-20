#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "game/game.h"

#define X_PIN 27
#define Y_PIN 26

#define BUTTON_A 5
#define BUTTON_B 6

#define I2C_SDA 14
#define I2C_SCL 15

#define JOYSTICK_LEFT_THRESHOLD   1500
#define JOYSTICK_RIGHT_THRESHOLD  2500
#define JOYSTICK_UP_THRESHOLD     1500
#define JOYSTICK_DOWN_THRESHOLD   2500

// Implementação da função getInput()
char getInput() {
    char input;
    bool done = false;

    while(!done) {
        bool button_a_state = gpio_get(BUTTON_A);
        bool button_b_state = gpio_get(BUTTON_B);

        adc_select_input(1);
        uint16_t adc_x = adc_read();

        adc_select_input(0);
        uint16_t adc_y = adc_read();

        if(!button_a_state && !done) {
            input = 'A';
            done = true;
        }
        else if(!button_b_state && !done) {
            input = 'B';
            done = true;
        }

        if (adc_x < JOYSTICK_LEFT_THRESHOLD && !done) {
            input = 'L';
            done = true;
        } 
        else if (adc_x > JOYSTICK_RIGHT_THRESHOLD && !done) {
            input = 'R';
            done = true;
        }

        if (adc_y < JOYSTICK_UP_THRESHOLD && !done) {
            input = 'D';
            done = true;
        } else if (adc_y > JOYSTICK_DOWN_THRESHOLD && !done) {
            input = 'U';
            done = true;
        }

        // Delay para evitar problemas de debounce
        sleep_ms(100);
    }

    return input;
}

// Inicialização dos botões
void initButtons() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

// Inicialização do joystick
void initJoystick() {
    adc_init();
    adc_gpio_init(Y_PIN);
    adc_gpio_init(X_PIN);
}

// Inicialização do display
void initDisplay() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();
}

int main() {
    // Inicialização
    stdio_init_all();
    initButtons();
    initJoystick();
    sleep_ms(1000);
    initDisplay();

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // Zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    while (1) {
        char text[1] = {'a'};
        text[0] = getInput();
        int y=0;
        ssd1306_draw_string(ssd, 5, y, text);

        render_on_display(ssd, &frame_area);

        sleep_ms(250);
        memset(ssd, 0, ssd1306_buffer_length);
        render_on_display(ssd, &frame_area);
    }
}