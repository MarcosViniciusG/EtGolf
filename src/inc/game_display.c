#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "../game/game.h"

struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

void clearDisplay(GameDisplay *gameDisplay) {
    memset(gameDisplay->ssd , 0, 1024);
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(50);
}

void cardToString(const Card *card, char *str) {
    const char suits[] = {'H', 'D', 'C', 'S', '$'};

    uint8_t off=0;
    if(card->value == -1) 
        str[off] = '$';
    else if(card->value == 0)
        str[off] = 'K';
    else if(card->value == 1)
        str[off] = 'A';
    else if(card->value == 10) {
        str[off] = '1';
        off++;
        str[off] = '0';
    }
    else if(card->value == 11)
        str[off] = 'J';
    else if(card->value == 12)
        str[off] = 'Q';
    else
        str[off] = card->value + '0';
    
    off++;
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
                if(row == player->selectedRow && col == player->selectedColumn && !player->isAi && (game->state!=CHOOSE_SOURCE) && p==game->currentPlayer) {
                    ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xCards[col], playerDisplay->yCards[row], "OO");
                }     
                else if(isFaceUp) {
                    char str[3];
                    cardToString(card, str);
                    ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xCards[col], playerDisplay->yCards[row], str);
                }
                else {
                    ssd1306_draw_string(gameDisplay->ssd, playerDisplay->xCards[col], playerDisplay->yCards[row], "XX");
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
        char str[3];
        cardToString(card, str);
        ssd1306_draw_string(gameDisplay->ssd, gameDisplay->xDiscardTop, gameDisplay->yDiscardTop, str);
    }
}

void displayChosen(Game *game) {
    GameDisplay *gameDisplay = &game->gameDisplay;
    Card *card = &game->chosenCard;
    if((card->value != NULL_CARD.value || card->suit != NULL_CARD.suit) && game->state!=CHOOSE_SOURCE) {
        char str[3];
        cardToString(card, str);
        ssd1306_draw_string(gameDisplay->ssd, gameDisplay->xChosenCard, gameDisplay->yChosenCard, str);
    }
}

void displayGame(Game *game) {
    if(game->gameMode==SERVER) {
        sendGameState(game);
    }
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);

    displayHand(game);
    displayScore(game);
    displayDiscardTop(game);
    displayChosen(game);

    render_on_display(gameDisplay->ssd, &frame_area);
}