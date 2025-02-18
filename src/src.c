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

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main() {
    stdio_init_all();
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    adc_gpio_init(27);

    sleep_ms(1000);
    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    restart:

    while (1) {
        adc_select_input(1);
        uint adc_x_raw = adc_read();
        adc_select_input(0);
        uint adc_y_raw = adc_read();

        // Display the joystick position something like this:
        // X: [            o             ]  Y: [              o         ]
        const uint bar_width = 15;
        const uint adc_max = (1 << 12) - 1;
        uint bar_x_pos = adc_x_raw * bar_width / adc_max;
        uint bar_y_pos = adc_y_raw * bar_width / adc_max;

        char text[2][15];

        for (uint i = 0; i < bar_width; ++i)
            text[0][i] = ( i == bar_x_pos ? 'o' : ' '); 

        for (uint i = 0; i < bar_width; ++i)
            text[1][i] = ( i == bar_y_pos ? 'o' : ' '); 

        int y = 0;
        for (uint i = 0; i < count_of(text); i++)
        {   
            ssd1306_draw_string(ssd, 5, y, text[i]);
            y += 8;
        }

        render_on_display(ssd, &frame_area);

        sleep_ms(100);
    }
}