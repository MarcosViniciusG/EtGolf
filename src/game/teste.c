#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include "game.h"

void printCard(const Card *card) {
    const char suits[] = {'H', 'D', 'C', 'S', '$'};

    if(card->value == -1)
        printf("$");
    else if(card->value == 0)
        printf("K");
    else if(card->value == 1)
        printf("A");
    else if(card->value == 11)
        printf("J");
    else if(card->value == 12)
        printf("Q");
    else
        printf("%d", card->value);
    
    printf("%c ", suits[card->suit]);
}

void displayGameInit(const Game *game) {
    printf("Jogo iniciando...\n");
}

void printHand(const Player *player) {
    for(uint8_t i=0; i<HAND_SIZE; i++) {
        for(uint8_t j=0; j<HAND_SIZE; j++)
            if(player->isFaceUp[i][j])
                printCard(&player->cards[i][j]);
            else
                printf("** ");
        printf("\n");
    }
    printf("\n");
}

void displayCard(const Card *card) {
    printCard(card);
    printf("\n");
}

void displayHand(const Player *player) {
    printHand(player);
}

void displayScore(const Player *player) {
    printf("Pontuação do jogador %d: %d\n", player->ID, player->score);
} 

void displayDiscardTop(const Game *game) {
    printf("Topo do descarte:\n");
    printCard(&game->discard[game->discardTop]);
    printf("\n");
}

void displayChosen(const Game *game) {
    printf("Carta escolhida:\n");
    printCard(&game->chosenCard);
    printf("\n");
}

void displayFirstMove(const Game *game) {
    const Player *player = &game->players[game->currentPlayer];

    printf("\nTurno do jogador %u:\n", player->ID);
    printHand(player);
    printf("\nCarta na linha %u e coluna %u\n", player->selectedRow+1, player->selectedColumn+1);
    printf("Opções disponíveis:\n");
    printf("  (L, R, U, D) para se movimentar na matriz\n");
    printf("  (A) Revela a carta selecionada\n");
}

void displayChooseSource(const Game *game) {
    const Player *player = &game->players[game->currentPlayer];

    printf("\nTurno do jogador %u:\n", player->ID);
    printHand(player);
    printf("Escolha de onde pegar a carta:\n");
    printf("  (A) Pegar do baralho\n");
    printf("  (B) Pegar do topo pilha de descarte (Se tiver alguma)\n");
}

void displayCardAction(const Game *game) {
    const Player *player = &game->players[game->currentPlayer];

    printf("\nTurno do jogador %u:\n", player->ID);
    printHand(player);
    printf("\n Carta escolhida:\n");
    printCard(&game->chosenCard);
    printf("\n Carta na linha %u e coluna %u\n", player->selectedRow+1, player->selectedColumn+1);
    printf("Opções disponíveis:\n");
    printf("  (L, R, U, D) para se movimentar na matriz\n");
    printf("  (A) Troca a carta selecionada pela carta escolhida\n");
    printf("  (B) Revela a carta selecionada\n");
}

void displayGameEnd(const Game *game) {
    for(uint8_t p=0; p<N_PLAYERS; p++) {
        printf("%d. ", p+1);
        printf("Jogador %d com %d pontos\n", game->players[p].ID, game->players[p].score);
    }

    printf("Obrigado por jogar!\n");
}

char getInput() {
    char input;
    printf("Comando: ");
    scanf(" %c", &input);
    return toupper(input);
}

int main() {
    gameLoop();
    return 0;
}