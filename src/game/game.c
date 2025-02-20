#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>

// ----- Macros -----
#define DECK_SIZE 54
#define HAND_SIZE 3
#define N_PLAYERS 2
// ------ Fim dos macros -----

// ----- Estruturas -----

// Estrutura que define onde cada informação visível
// do jogador vai ser mostrada. Para isso, ela contém
// as coordernadas de início X e início Y
typedef struct {
    uint8_t xID;
    uint8_t yID;

    uint8_t xScore;
    uint8_t yScore;

    uint8_t xCards[HAND_SIZE];
    uint8_t yCards[HAND_SIZE];
} PlayerDisplay;

typedef struct {
    PlayerDisplay playersDisplay[N_PLAYERS];
    
    uint8_t xDiscardTop;
    uint8_t yDiscardTop;

    uint8_t xChosenCard;
    uint8_t yChosenCard;

    uint8_t ssd[1024];
} GameDisplay;

// Estrutura que enumera os possíveis estados do jogo
typedef enum {
    INIT,
    DEAL,
    PLAYER_TURN,
    FIRST_MOVE,
    CHOOSE_SOURCE,
    CARD_ACTION,
    EVALUATE,
    LAST_MOVE,
    GAME_END
} GameState;

// Estrutura que enumera os possíveis naipes das cartas
typedef enum {
    HEARTS,
    DIAMONDS,
    CLUBS,
    SPADES,
    NONE
} Suit;

// Estrutura que define uma carta
typedef struct {
    int value;
    Suit suit;
} Card;

// Estrutura que define um jogador
typedef struct 
{
    Card cards[HAND_SIZE][HAND_SIZE];
    bool isFaceUp[HAND_SIZE][HAND_SIZE];
    uint8_t selectedRow;
    uint8_t selectedColumn;
    uint8_t ID;
    int score;
    bool isAi;
} Player;

// Estrutura que define o jogo
typedef struct
{
    GameState state;
    Card deck[DECK_SIZE];
    uint8_t deckTop;
    uint8_t discardTop;
    Card discard[DECK_SIZE];
    Player players[N_PLAYERS];
    uint8_t currentPlayer;
    Card chosenCard;
    int turns;
    GameDisplay gameDisplay;
} Game;

// Estrutura de um comando
typedef bool (*CommandFunc)(Game *game);

typedef struct {
    char code;       // e.g., 'L', 'R', etc.
    CommandFunc execute;
} Command;
// ----- Fim das estruturas -----

// Protótipos das funções que cuidam do jogo
void gameLoop();                       // Responsável por cuidar do loop do jogo (onde tudo acontece)
void initGame(Game *game);             // Inicia o jogo (variáveis e assim por diante)
void shuffleDeck(Game *game);          // Embaralha as cartas
void dealCards(Game *game);            // Distribui as cartas
bool evaluateTurn(Game *game);         // Atualiza a pontução dos jogadores e muda de jogador 
bool allCardsFaceUp(const Player *player); // Verifica se um jogador já virou todas as cartas
void endGame(Game *game);              // Termina o jogo e mostra a pontuação
void updateScore(Player *player);      // Atualiza a pontuação de um jogador em específico
char getInput();                       // Função que pega a entrada
void turnAllCards(Game *game);         // Quando o jogo finalizar, vira todas as cartas 

// Protótipos dos possíveis comandos do jogo
bool commandLeft(Game *game);          // Esquerda
bool commandRight(Game *game);         // Direita
bool commandUp(Game *game);            // Cima
bool commandDown(Game *game);          // Baixo
bool commandChooseDeck(Game *game);    // Escolhe o baralho como fonte
bool commandChooseDiscard(Game *game); // Escolhe o topo da pilha de descarte como fonte
bool commandFlip(Game *game);          // Vira a carta selecionada na mão
bool commandReplace(Game *game);       // Substitui a carta selecionada na mão

// Protótipos das funções processadoras dos comandos
void processFirstMove(Game *game);     // Primeiro movimento do jogo
void processChooseSource(Game *game);  // Escolher a fonte da carta escolhida
void processCardAction(Game *game);    // Escolher uma ação com a carta

// Protótipos das funções utilizados pela IA
void aiProcessFirstMove(Game *game);
void aiProcessChooseSource(Game *game);
void aiProcessCardAction(Game *game);

// Protótipos das funções para saída
void displayGameInit(Game *game);
void displayGame(Game *game);
void displayFirstMove(Game *game);
void displayChooseSource(Game *game);
void displayCardAction(Game *game);
void displayGameEnd(Game *game);

// Constantes
const Card NULL_CARD = {0, NONE};

// ----- Contextualização dos comandos -----
Command chooseSourceCommands[] = {
    { 'A', commandChooseDeck },
    { 'B', commandChooseDiscard }
};
uint8_t numChooseSourceCommands = sizeof(chooseSourceCommands) / sizeof(chooseSourceCommands[0]);

Command cardActionCommands[] = {
    { 'L', commandLeft },
    { 'R', commandRight },
    { 'U', commandUp },
    { 'D', commandDown },
    { 'A', commandReplace },
    { 'B', commandFlip }
};
uint8_t numCardActionCommands = sizeof(cardActionCommands) / sizeof(cardActionCommands[0]);

Command firstMoveCommands[] = {
    { 'L', commandLeft },
    { 'R', commandRight },
    { 'U', commandUp },
    { 'D', commandDown },
    { 'A', commandFlip },
};
uint8_t numFirstMoveCommands = sizeof(firstMoveCommands) / sizeof(firstMoveCommands[0]);
// ----- Fim da contextualização dos comandos -----

// ----- Implementação dos comandos -----
bool commandLeft(Game *game) {
    Player *player = &game->players[game->currentPlayer];

    if(player->selectedColumn==0)
        player->selectedColumn = 2;
    else
        player->selectedColumn--;

    return true;
}

bool commandRight(Game *game) {
    Player *player = &game->players[game->currentPlayer];

    player->selectedColumn = (player->selectedColumn + 1) % HAND_SIZE;
    return true;
}

bool commandUp(Game *game) {
    Player *player = &game->players[game->currentPlayer];

    if(player->selectedRow==0)
        player->selectedRow = 2;
    else
        player->selectedRow--;

    return true;
}

bool commandDown(Game *game) {
    Player *player = &game->players[game->currentPlayer];

    player->selectedRow = (player->selectedRow + 1) % HAND_SIZE;

    return true;
}

bool commandChooseDeck(Game *game) {
    game->chosenCard = game->deck[game->deckTop];
    game->deckTop++;

    // TODO: 
    // Manda pegar as cartas de descarte e reembaralhar
    if(game->deckTop == DECK_SIZE) {

    }

    return true;
}

bool commandChooseDiscard(Game *game) {
    if(!game->discardTop) {
        return false;
    }
    
    game->chosenCard = game->discard[game->discardTop];
    game->discardTop--;

    return true;
}

bool commandReplace(Game *game) {
    Player *player = &game->players[game->currentPlayer];
    uint8_t row = player->selectedRow;
    uint8_t col = player->selectedColumn;

    player->isFaceUp[row][col] = true;
    game->discardTop++;
    game->discard[game->discardTop] = player->cards[row][col];
    player->cards[row][col] = game->chosenCard;

    return true;
}

bool commandFlip(Game *game) {
    Player *player = &game->players[game->currentPlayer];

    uint8_t row = player->selectedRow;
    uint8_t col = player->selectedColumn;

    if (player->isFaceUp[row][col]) {
        return false;
    }

    // Se alguma carta já foi retirada do baralho ou pilha de descarte
    if(game->chosenCard.suit != NULL_CARD.suit || game->chosenCard.value != NULL_CARD.value) {
        game->discardTop++;
        game->discard[game->discardTop] = game->chosenCard;
    }
    
    player->isFaceUp[row][col] = true;
    
    return true;
}
// ----- Fim da implementação dos comandos -----

// ----- Implementação das funções processadoras -----
void processFirstMove(Game *game) {
    char input;
    uint8_t moves=0;
    Player *player = &game->players[game->currentPlayer];
    if(player->isAi) {
        aiProcessFirstMove(game);
        return;
    }

    while (moves < 2) {
        displayFirstMove(game);
        input = getInput();
        bool processed = false;
        for (uint8_t i = 0; i < numFirstMoveCommands; i++) {
            if (input == firstMoveCommands[i].code && moves < 2) {
                    if(input=='A')
                        moves += (firstMoveCommands[i].execute(game) ? 1 : 0);
                    else
                        firstMoveCommands[i].execute(game);

                    processed = 1;
                    break;
                }
            }
        if (!processed) {

        }
    }
}

void processChooseSource(Game *game) {
    char input;
    bool done = false;
    Player *player = &game->players[game->currentPlayer];
    if(player->isAi) {
        aiProcessChooseSource(game);
        return;
    }

    while (!done) {
        displayChooseSource(game);
        input = getInput();
        bool found = false;
        for (uint8_t i = 0; i < numChooseSourceCommands; i++) {
            if (input == chooseSourceCommands[i].code) {
                done = chooseSourceCommands[i].execute(game);
                found = 1;
                break;
            }
        }
        if (!found) {

        }
    }
}

void processCardAction(Game *game) {
    char input;
    bool done = 0;
    Player *player = &game->players[game->currentPlayer];
    if(player->isAi) {
        aiProcessCardAction(game);
        return;
    }

    while (!done) {
        displayCardAction(game);
        input = getInput();
        bool processed = false;
        for (uint8_t i = 0; i < numCardActionCommands; i++) {
            if (input == cardActionCommands[i].code) {
                if(input == 'A' || input == 'B') {
                    done = cardActionCommands[i].execute(game);
                }
                else
                    cardActionCommands[i].execute(game);

                processed = 1;
                break;
            }
        }
        if (!processed) {
            
        }
    }
}
// ----- Fim da implementação das funções processadoras -----

// ----- Funções utilizadas pela IA -----
void aiProcessFirstMove(Game *game) {
    uint8_t p = game->currentPlayer;
    Player *player = &game->players[p];

    player->isFaceUp[0][2] = true;
    player->isFaceUp[1][2] = true;
}

void aiProcessChooseSource(Game *game) {
    int discardValue = game->discard[game->discardTop].value;
    
    if (discardValue <= 4) {
        commandChooseDiscard(game);
    } else {
        commandChooseDeck(game);
    }
}

void aiProcessCardAction(Game *game) {
    int drawnValue = game->chosenCard.value;
    uint8_t p = game->currentPlayer;
    Player *player = &game->players[p];

    uint8_t selectedRow=0;
    uint8_t selectedColumn=0;
    if (drawnValue <= 5) {
        // Pega a carta com maior valor para substituir
        int maxValue = 0;
        for (uint8_t row = 0; row < HAND_SIZE; row++) {
            for(uint8_t col = 0; col < HAND_SIZE; col++) {
                if (player->cards[row][col].value > maxValue) {
                    maxValue = player->cards[row][col].value;
                    selectedRow = row;
                    selectedColumn = col;
                }
            }
        }
        player->selectedRow = selectedRow;
        player->selectedColumn = selectedColumn; 
        commandReplace(game);
    } else {
        // Pega a primeira carta que não esteja virada
        for (uint8_t row = 0; row < HAND_SIZE; row++) {
            for(uint8_t col = 0; col < HAND_SIZE; col++) {
                if (!player->isFaceUp[row][col]) {
                    selectedRow = row;
                    selectedColumn = col;
                    break;
                } 
            }
        }
        
        player->selectedRow = selectedRow;
        player->selectedColumn = selectedColumn;
        commandFlip(game);
    }
}
// ----- Fim das funções utilizadas pela IA -----

void gameLoop(Game *game) {
    initGame(game);

    while(game->state != GAME_END) {
        switch(game->state) {
            case INIT:
                game->state = DEAL;
                break;
                
            case DEAL:
                dealCards(game);
                game->state = FIRST_MOVE;
                break;
            
            case FIRST_MOVE:
                for(uint8_t p=0; p<N_PLAYERS; p++) {
                    processFirstMove(game);
                    evaluateTurn(game);
                }
                game->state = CHOOSE_SOURCE;
                break;

            case CHOOSE_SOURCE:
                processChooseSource(game);
                game->state = CARD_ACTION;
                break;

            case CARD_ACTION:
                processCardAction(game);
                game->state = EVALUATE;
                break;
                
            case EVALUATE:
                bool isGameOver = evaluateTurn(game);
                if(isGameOver)
                    game->state = LAST_MOVE;
                else
                    game->state = CHOOSE_SOURCE;
                break;
            
            case LAST_MOVE:
                for(uint8_t p=0; p<N_PLAYERS-1; p++) {
                    processChooseSource(game);
                    processCardAction(game);
                    evaluateTurn(game);
                }
                turnAllCards(game);
                evaluateTurn(game);
                game->state = GAME_END;
                break;
            
            default:
                game->state = GAME_END;
                break;
        }
    }

    endGame(game);
}

int comp(const void* a,const void* b) {
    Player *playerA = (Player *)a;
    Player *playerB = (Player *)b;
  
    return (playerA->score - playerB->score);
}

void initGame(Game *game) {
    // Cria um baralho padrão com 54 cartas
    // sendo 13 de cada naipe e dois coringas
    uint8_t index=0;
    for(uint8_t s= HEARTS; s <= SPADES; s++) {
        for(uint8_t value=0; value <= 12; value++) {
            game->deck[index].value = value;
            game->deck[index].suit = (Suit)s;
            index++;
        }
    }

    // Adiciona os 2 coringas
    game->deck[index].value = -1;
    game->deck[index].suit = NONE;
    index++;
    game->deck[index].value = -1;
    game->deck[index].suit = NONE;

    shuffleDeck(game);

    game->deckTop = 0;
    game->turns = 0;
    game->currentPlayer = 0;
    game->discardTop = 0;

    // Placeholder para falar que nenhuma carta foi escolhida
    game->chosenCard = NULL_CARD;

    for(uint8_t p=0; p<N_PLAYERS; p++) {
        game->players[p].score = 0;
        game->players[p].selectedColumn = 0;
        game->players[p].selectedRow = 0;
        game->players[p].ID = p+1;
        if(!p)
            game->players[p].isAi = false;
        else
            game->players[p].isAi = true;
    }

    game->state = INIT;
    displayGameInit(game);
}

void shuffleDeck(Game *game) {
    srand((unsigned)time(NULL));
    for(uint8_t i= DECK_SIZE - 1; i > 0; i--) {
        uint8_t j = rand() % (i+1);
        Card temp = game->deck[i];
        game->deck[i] = game->deck[j];
        game->deck[j] = temp;
    }
}

void dealCards(Game *game) {
    for(uint8_t p=0; p<N_PLAYERS; p++) {
        for(uint8_t i=0; i<HAND_SIZE; i++) {
            for(uint8_t j=0; j<HAND_SIZE; j++) {
                game->players[p].cards[i][j] = game->deck[game->deckTop];
                game->players[p].isFaceUp[i][j] = false;
                game->deckTop++;
            }
        }
    }
}

void updateScore(Player *player) {
    int score=0;
    for(uint8_t col=0; col<HAND_SIZE; col++) {
        bool clean=true;
        int colScore=0;
        for(uint8_t row=0; row<HAND_SIZE; row++) {
            if(player->isFaceUp[row][col]) {
                colScore += player->cards[row][col].value;
                if(player->cards[0][col].value != player->cards[row][col].value)
                    clean = false;
            }
            else
                clean = false;
        }

        // Se todas as cartas da coluna forem do mesmo valor,
        // a pontuação da coluna vai ser zero
        if(clean)
            colScore = 0;
        
        score += colScore;
    }

    player->score = score;
}

bool evaluateTurn(Game *game) {
    // Atualiza a pontuação de todos os jogadores
    for(uint8_t p = 0; p<N_PLAYERS; p++) {
        updateScore(&game->players[p]);
    }

    // Verifica se depois do turno do jogador atual
    // todas as cartas dele foram viradas para cima
    bool isGameOver = allCardsFaceUp(&game->players[game->currentPlayer]);

    game->currentPlayer = (game->currentPlayer + 1) % N_PLAYERS; // Troca o jogador
    game->turns++; // Aumenta a quantidade de turnos jogados

    return isGameOver;
}

bool allCardsFaceUp(const Player *player) {
    for(uint8_t i=0; i<HAND_SIZE; i++)
    for(uint8_t j=0; j<HAND_SIZE; j++)
        if(!player->isFaceUp[i][j]) return false;

    return true;
}

void turnAllCards(Game *game) {
    Player *player;
    for(uint8_t p = 0; p<N_PLAYERS; p++) {
        player = &game->players[p];
        for(uint8_t row=0; row<HAND_SIZE; row++) 
        for(uint8_t col=0; col<HAND_SIZE; col++)
            player->isFaceUp[row][col] = true;
    }
}

void endGame(Game *game) {
    uint8_t n = sizeof(game->players) / sizeof(game->players[0]);
    qsort(game->players, n, sizeof(Player), comp);
    displayGameEnd(game);
}