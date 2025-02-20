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

// ----- Macros -----
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
// ----- Fim dos macros -----

// Variáveis globais 
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// ----- Implementação das funções de entrada -----
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
// ----- Fim da implementação das funções de entrada -----

// ----- Funções auxiliares -----
void clearDisplay(GameDisplay *gameDisplay) {
    memset(gameDisplay->ssd , 0, 1024);
    render_on_display(gameDisplay->ssd, &frame_area);
}

void cardToString(const Card *card, char str[2]) {
    const char suits[] = {'H', 'D', 'C', 'S', 'M'};

    if(card->value == -1) 
        str[0] = 'M';
    else if(card->value == 0)
        str[0] = 'K';
    else if(card->value == 1)
        str[0] = 'A';
    else if(card->value == 10)
        str[0] = 'Z';
    else if(card->value == 11)
        str[0] = 'J';
    else if(card->value == 12)
        str[0] = 'Q';
    else
        str[0] = card->value + '0';
    
    str[1] = suits[card->suit];
}

// Fonte: https://www.geeksforgeeks.org/how-to-convert-an-integer-to-a-string-in-c/
void intToStr(int N, char *str) {
    int i = 0;
  
    // Save the copy of the number for sign
    int sign = N;

    // If the number is negative, make it positive
    if (N < 0)
        N = -N;

    // Extract digits from the number and add them to the
    // string
    while (N > 0) {
      
        // Convert integer digit to character and store
      	// it in the str
        str[i++] = N % 10 + '0';
      	N /= 10;
    } 

    // If the number was negative, add a minus sign to the
    // string
    if (sign < 0) {
        str[i++] = '-';
    }

    // Null-terminate the string
    str[i] = '\0';

    // Reverse the string to get the correct order
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}

void displayHand(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);

    for(uint8_t p=0; p<N_PLAYERS; p++) {
        PlayerDisplay *playerDisplay = &gameDisplay->playersDisplay[p];
        Player *player = &game->players[p];

        for(uint8_t row=0; row<HAND_SIZE; row++) {
            for(uint8_t col=0; col<HAND_SIZE; col++) {
                Card *card = &player->cards[row][col];
                bool isFaceUp = player->isFaceUp[row][col];
                if(row == player->selectedRow && col == player->selectedColumn && !player->isAi) {
                    ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xCards[col], playerDisplay->yCards[row], "OO");
                }     
                else if(isFaceUp) {
                    char str[2];
                    cardToString(card, str);
                    ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xCards[col], playerDisplay->yCards[row], str);
                }
                else {
                    ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xCards[col], playerDisplay->yCards[row], "WW");
                }
            }
        }
    }
}

void displayScore(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    for(uint8_t p=0; p<N_PLAYERS; p++) {
        PlayerDisplay *playerDisplay = &gameDisplay->playersDisplay[p];
        Player *player = &game->players[p];

        char str[3];
        intToStr(player->score, str);
        ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xScore, playerDisplay->yScore, str);
    }
}

void displayDiscardTop(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    uint8_t discardTop = game->discardTop;
    if(discardTop != 0) {
        Card *card = &game->discard[game->discardTop];
        char str[2];
        cardToString(card, str);
        ssd1306_draw_string(gameDisplay->ssd, gameDisplay->xDiscardTop, gameDisplay->yDiscardTop, str);
    }
}

void displayChosen(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    Card *card = &game->chosenCard;
    if(card->value != NULL_CARD.value || card->suit != NULL_CARD.suit) {
        char str[2];
        cardToString(card, str);
        ssd1306_draw_string(gameDisplay->ssd, gameDisplay->xChosenCard, gameDisplay->yChosenCard, str);
    }

}
// ----- Fim das funções auxiliares -----

// ----- Implementação das funções de saída -----
void displayGameInit(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 32, "INICIO DO JOGO");
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(3000);
}

void displayGame(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);

    displayHand(game);
    displayScore(game);
    displayDiscardTop(game);
    displayChosen(game);

    render_on_display(gameDisplay->ssd, &frame_area);
}

void displayFirstMove(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    displayGame(game);
}

void displayChooseSource(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    displayGame(game);
}

void displayCardAction(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    displayGame(game);
}

void displayGameEnd(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    displayGame(game);
}
// ----- Fim da implementação das funções de saída -----

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
    calculate_render_area_buffer_length(&frame_area);
}

void initGameDisplay(GameDisplay *gameDisplay) {
    // Inicialização do primeiro jogador
    gameDisplay->playersDisplay[0].xID = 1;
    gameDisplay->playersDisplay[0].yID = 2;

    gameDisplay->playersDisplay[0].xScore = 93;
    gameDisplay->playersDisplay[0].yScore = 2;

    gameDisplay->playersDisplay[0].xCards[0] = 11;
    gameDisplay->playersDisplay[0].xCards[1] = 29;
    gameDisplay->playersDisplay[0].xCards[2] = 47;

    gameDisplay->playersDisplay[0].yCards[0] = 2;
    gameDisplay->playersDisplay[0].yCards[1] = 12;
    gameDisplay->playersDisplay[0].yCards[2] = 22;
    
    // Inicialização do segundo jogador
    gameDisplay->playersDisplay[1].xID = 1;
    gameDisplay->playersDisplay[1].yID = 34;

    gameDisplay->playersDisplay[1].xScore = 93;
    gameDisplay->playersDisplay[1].yScore = 34;

    gameDisplay->playersDisplay[1].xCards[0] = 11;
    gameDisplay->playersDisplay[1].xCards[1] = 29;
    gameDisplay->playersDisplay[1].xCards[2] = 47;

    gameDisplay->playersDisplay[1].yCards[0] = 34;
    gameDisplay->playersDisplay[1].yCards[1] = 44;
    gameDisplay->playersDisplay[1].yCards[2] = 54;

    // Inicialização da pilha de descarte
    gameDisplay->xDiscardTop = 108;
    gameDisplay->yDiscardTop = 54;

    // Inicialização da carta pega (do baralho ou pilha de descarte)
    gameDisplay->xChosenCard = 73;
    gameDisplay->yChosenCard = 54;
}

int main() {
    // Inicialização
    stdio_init_all();
    initButtons();
    initJoystick();
    sleep_ms(1000);
    initDisplay();

    Game game;
    initGameDisplay(&game.gameDisplay);
    calculate_render_area_buffer_length(&frame_area);

    // Zera o display inteiro
    memset(game.gameDisplay.ssd , 0, 1024);
    render_on_display(game.gameDisplay.ssd, &frame_area);

    while(true) {
        gameLoop(&game);
    }
}