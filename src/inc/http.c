#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "jsmn.h"
#include "game_display.c"

// Variáveis globais
const char *header = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/json\r\n"
                     "Connection: close\r\n\r\n";
char httpBuffer[1024];
char remoteGameState[1024];
bool sendRemote = false;
char remoteInput = '#';
struct tcp_pcb *pendingPlayer = NULL;

// Serializa o jogo em uma string JSON
void gameToJSON(const Game *game, char *buffer, size_t buf_size)
{
    // Limpa o conteúdo do buffer
    memset(buffer, 0, buf_size);

    int offset = 0;

    // Serializa as informações do jogo
    offset += snprintf(buffer + offset, buf_size - offset,
                       "{\"gameMode\":%d,\"selectedGameMode\":%u,\"state\":%d,"
                       "\"deckTop\":%u,\"discardTop\":%u,\"currentPlayer\":%u,"
                       "\"turns\":%d,",
                       game->gameMode, game->selectedGameMode, game->state,
                       game->deckTop, game->discardTop, game->currentPlayer,
                       game->turns);

    // Serializa a carta escolhida
    offset += snprintf(buffer + offset, buf_size - offset,
                       "\"chosenCard\":{\"suit\":%d,\"value\":%d},",
                       game->chosenCard.suit, game->chosenCard.value);

    // Começa a serialização dos jogadores
    offset += snprintf(buffer + offset, buf_size - offset, "\"players\":[");

    for (uint8_t i = 0; i < N_PLAYERS; i++)
    {
        // Informações do jogador
        offset += snprintf(buffer + offset, buf_size - offset,
                           "{\"ID\":\"%u\",\"score\":%u,\"cards\":[",
                           game->players[i].ID, game->players[i].score);

        // Cartas do jogador
        for (uint8_t cardIndex = 0; cardIndex < HAND_SIZE * HAND_SIZE; cardIndex++)
        {
            uint8_t row = cardIndex / HAND_SIZE;
            uint8_t col = cardIndex % HAND_SIZE;
            offset += snprintf(buffer + offset, buf_size - offset,
                               "{\"suit\":%d,\"value\":%d}%s",
                               game->players[i].cards[row][col].suit,
                               game->players[i].cards[row][col].value,
                               (cardIndex < HAND_SIZE * HAND_SIZE - 1) ? "," : "");
        }
        // Fecha as cartas do jogador
        offset += snprintf(buffer + offset, buf_size - offset, "],");

        // Serializa o estado das cartas do jogador
        offset += snprintf(buffer + offset, buf_size - offset, "\"isFaceUp\":[");
        for (uint8_t faceIndex = 0; faceIndex < HAND_SIZE * HAND_SIZE; faceIndex++)
        {
            uint8_t row = faceIndex / HAND_SIZE;
            uint8_t col = faceIndex % HAND_SIZE;
            offset += snprintf(buffer + offset, buf_size - offset,
                               "%d%s",
                               game->players[i].isFaceUp[row][col],
                               (faceIndex < HAND_SIZE * HAND_SIZE - 1) ? "," : "");
        }

        // Fecha o estados da carta
        offset += snprintf(buffer + offset, buf_size - offset, "],");

        // Serializa a linha selecionada e a coluna selecionada
        offset += snprintf(buffer + offset, buf_size - offset,
                           "\"selectedRow\":%u,\"selectedColumn\":%u}",
                           game->players[i].selectedRow, game->players[i].selectedColumn);

        // Fecha o jogador
        offset += snprintf(buffer + offset, buf_size - offset, "%s",
                           (i < N_PLAYERS - 1) ? "," : "");
    }

    // Fecha os jogadores e o jogo
    offset += snprintf(buffer + offset, buf_size - offset, "]}");
}

int find_key(const char *json, jsmntok_t *tokens, int num_tokens, const char *key)
{
    int key_len = strlen(key);
    for (int i = 0; i < num_tokens; i++)
    {
        if (tokens[i].type == JSMN_STRING &&
            (tokens[i].end - tokens[i].start) == key_len &&
            strncmp(json + tokens[i].start, key, key_len) == 0)
        {
            return i + 1;
        }
    }
    return -1;
}

/* Helper: Converts the JSON token to an integer. */
int json_to_int(const char *json, jsmntok_t *token)
{
    char buf[32];
    int len = token->end - token->start;
    if (len >= (int)sizeof(buf))
        len = sizeof(buf) - 1;
    strncpy(buf, json + token->start, len);
    buf[len] = '\0';
    return atoi(buf);
}

bool JSONToGame(Game *game)
{
    jsmn_parser parser;
    jsmntok_t tokens[512];
    jsmn_init(&parser);

    int num_tokens = jsmn_parse(&parser, remoteGameState, strlen(remoteGameState),
                                tokens, sizeof(tokens) / sizeof(tokens[0]));
    if (num_tokens < 0)
    {
        return false;
    }

    // Extract top-level game fields
    int idx = find_key(remoteGameState, tokens, num_tokens, "gameMode");
    if (idx != -1)
        game->gameMode = json_to_int(remoteGameState, &tokens[idx]);

    idx = find_key(remoteGameState, tokens, num_tokens, "selectedGameMode");
    if (idx != -1)
        game->selectedGameMode = (unsigned int)json_to_int(remoteGameState, &tokens[idx]);

    idx = find_key(remoteGameState, tokens, num_tokens, "deckTop");
    if (idx != -1)
        game->deckTop = (unsigned int)json_to_int(remoteGameState, &tokens[idx]);

    idx = find_key(remoteGameState, tokens, num_tokens, "discardTop");
    if (idx != -1)
        game->discardTop = (unsigned int)json_to_int(remoteGameState, &tokens[idx]);

    idx = find_key(remoteGameState, tokens, num_tokens, "currentPlayer");
    if (idx != -1)
        game->currentPlayer = (unsigned int)json_to_int(remoteGameState, &tokens[idx]);

    idx = find_key(remoteGameState, tokens, num_tokens, "turns");
    if (idx != -1)
        game->turns = json_to_int(remoteGameState, &tokens[idx]);

    // Parse the chosenCard object
    idx = find_key(remoteGameState, tokens, num_tokens, "chosenCard");
    if (idx != -1)
    {
        jsmntok_t *cardObj = &tokens[idx];
        // cardObj should be an object with two keys ("suit" and "value")
        for (int j = idx + 1; j < idx + 1 + cardObj->size * 2 && j < num_tokens; j++)
        {
            int keyLen = tokens[j].end - tokens[j].start;
            if (tokens[j].type == JSMN_STRING)
            {
                if (keyLen == 4 && strncmp(remoteGameState + tokens[j].start, "suit", keyLen) == 0)
                {
                    game->chosenCard.suit = json_to_int(remoteGameState, &tokens[j + 1]);
                }
                else if (keyLen == 5 && strncmp(remoteGameState + tokens[j].start, "value", keyLen) == 0)
                {
                    game->chosenCard.value = json_to_int(remoteGameState, &tokens[j + 1]);
                }
            }
        }
    }

    // Process the "players" array
    idx = find_key(remoteGameState, tokens, num_tokens, "players");
    if (idx == -1)
        return false;
    jsmntok_t *playersToken = &tokens[idx];
    if (playersToken->type != JSMN_ARRAY)
        return false;

    int playerCount = playersToken->size;
    int tokenIndex = idx + 1; // Points to the first player object token
    for (int i = 0; i < playerCount && i < N_PLAYERS; i++)
    {
        jsmntok_t *playerObj = &tokens[tokenIndex];
        if (playerObj->type != JSMN_OBJECT)
            return false;
        int objSize = playerObj->size; // Number of key/value pairs for this player

        // Parse "ID" (stored as a string in JSON)
        for (int j = tokenIndex + 1; j < tokenIndex + 1 + objSize * 2 && j < num_tokens; j++)
        {
            int keyLen = tokens[j].end - tokens[j].start;
            if (tokens[j].type == JSMN_STRING &&
                keyLen == 2 &&
                strncmp(remoteGameState + tokens[j].start, "ID", keyLen) == 0)
            {
                char idStr[32];
                int len = tokens[j + 1].end - tokens[j + 1].start;
                if (len >= (int)sizeof(idStr))
                    len = sizeof(idStr) - 1;
                strncpy(idStr, remoteGameState + tokens[j + 1].start, len);
                idStr[len] = '\0';
                game->players[i].ID = (uint8_t)atoi(idStr);
                break;
            }
        }

        // Parse "score"
        for (int j = tokenIndex + 1; j < tokenIndex + 1 + objSize * 2 && j < num_tokens; j++)
        {
            int keyLen = tokens[j].end - tokens[j].start;
            if (tokens[j].type == JSMN_STRING &&
                keyLen == 5 &&
                strncmp(remoteGameState + tokens[j].start, "score", keyLen) == 0)
            {
                game->players[i].score = (int)atoi(remoteGameState + tokens[j + 1].start);
                break;
            }
        }

        // Parse "cards" array (a flat array representing a matrix)
        for (int j = tokenIndex + 1; j < tokenIndex + 1 + objSize * 2 && j < num_tokens; j++)
        {
            int keyLen = tokens[j].end - tokens[j].start;
            if (tokens[j].type == JSMN_STRING &&
                keyLen == 5 &&
                strncmp(remoteGameState + tokens[j].start, "cards", keyLen) == 0)
            {
                int cardsIdx = j + 1;
                jsmntok_t *cardsToken = &tokens[cardsIdx];
                if (cardsToken->type != JSMN_ARRAY)
                    break;
                int totalCards = cardsToken->size; // should equal HAND_SIZE * HAND_SIZE
                int cardTokenIndex = cardsIdx + 1;
                for (int cardIndex = 0; cardIndex < totalCards && cardIndex < HAND_SIZE * HAND_SIZE; cardIndex++)
                {
                    jsmntok_t *cardObj = &tokens[cardTokenIndex];
                    if (cardObj->type != JSMN_OBJECT)
                        break;
                    int cardObjSize = cardObj->size; // Expected to be 2: "suit" and "value"
                    int suit = 0, value = 0;
                    for (int k = cardTokenIndex + 1; k < cardTokenIndex + 1 + cardObjSize * 2 && k < num_tokens; k++)
                    {
                        int klen = tokens[k].end - tokens[k].start;
                        if (tokens[k].type == JSMN_STRING)
                        {
                            if (klen == 4 && strncmp(remoteGameState + tokens[k].start, "suit", klen) == 0)
                            {
                                suit = atoi(remoteGameState + tokens[k + 1].start);
                            }
                            else if (klen == 5 && strncmp(remoteGameState + tokens[k].start, "value", klen) == 0)
                            {
                                value = atoi(remoteGameState + tokens[k + 1].start);
                            }
                        }
                    }
                    int row = cardIndex / HAND_SIZE;
                    int col = cardIndex % HAND_SIZE;
                    game->players[i].cards[row][col].suit = suit;
                    game->players[i].cards[row][col].value = value;
                    // Advance past the card object tokens:
                    cardTokenIndex += 1 + (cardObj->size * 2);
                }
                break;
            }
        }

        // Parse "isFaceUp" array (flat array representing a boolean matrix)
        for (int j = tokenIndex + 1; j < num_tokens && tokens[j].start < playerObj->end; j++)
        {
            int keyLen = tokens[j].end - tokens[j].start;
            if (tokens[j].type == JSMN_STRING &&
                keyLen == 8 &&
                strncmp(remoteGameState + tokens[j].start, "isFaceUp", keyLen) == 0)
            {
                int faceIdx = j + 1;
                jsmntok_t *faceToken = &tokens[faceIdx];
                int totalFaces = faceToken->size; // should equal HAND_SIZE * HAND_SIZE
                int faceTokenIndex = faceIdx + 1;
                for (int faceIndex = 0; faceIndex < totalFaces && faceIndex < HAND_SIZE * HAND_SIZE; faceIndex++)
                {
                    int tokenLen = tokens[faceTokenIndex].end - tokens[faceTokenIndex].start;
                    char tokenBuf[16];
                    if (tokenLen >= (int)sizeof(tokenBuf))
                        tokenLen = sizeof(tokenBuf) - 1;
                    strncpy(tokenBuf, remoteGameState + tokens[faceTokenIndex].start, tokenLen);
                    tokenBuf[tokenLen] = '\0';

                    // Convert token to integer and treat nonzero as true.
                    int numVal = atoi(tokenBuf);
                    int row = faceIndex / HAND_SIZE;
                    int col = faceIndex % HAND_SIZE;
                    game->players[i].isFaceUp[row][col] = (numVal != 0);
                    faceTokenIndex++;
                }
                break;
            }
        }

        // Parse "selectedRow" and "selectedColumn" for this player
        for (int j = tokenIndex + 1; j < num_tokens && tokens[j].start < playerObj->end; j++)
        {
            int keyLen = tokens[j].end - tokens[j].start;
            if (tokens[j].type == JSMN_STRING)
            {
                if (keyLen == 11 && strncmp(remoteGameState + tokens[j].start, "selectedRow", keyLen) == 0)
                {
                    game->players[i].selectedRow = atoi(remoteGameState + tokens[j + 1].start);
                }
                else if (keyLen == 14 && strncmp(remoteGameState + tokens[j].start, "selectedColumn", keyLen) == 0)
                {
                    game->players[i].selectedColumn = atoi(remoteGameState + tokens[j + 1].start);
                }
            }
        }

        // Advance tokenIndex to the next player object
        int nextTokenIndex = tokenIndex + 1;
        while (nextTokenIndex < num_tokens && tokens[nextTokenIndex].start < playerObj->end)
        {
            nextTokenIndex++;
        }
        tokenIndex = nextTokenIndex;
    }

    return true;
}

static err_t http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a resposta
    char buffer[p->tot_len + 1];
    pbuf_copy_partial(p, buffer, p->tot_len, 0);
    buffer[p->tot_len] = '\0';

    // Encontra o JSON
    char *json = strstr(buffer, "\r\n\r\n");
    if (json)
    {
        strcpy(remoteGameState, json);
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    Game *game = (Game *)arg;
    GameDisplay *gameDisplay = &game->gameDisplay;
    if (err != ERR_OK)
        return ERR_VAL;

    char request[128];
    char input='D';
    if(sendRemote) {
        JSONToGame(game);
        displayGame(game);
        render_on_display(gameDisplay->ssd, &frame_area);

        sendRemote = false;
        input = getInput();
    }

    snprintf(request, sizeof(request), "GET /game-state?input=%c HTTP/1.1\r\nHost: pico-server\r\n\r\n", input);
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, http_client_recv);
    return ERR_OK;
}

// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Guarda o jogador para enviar uma resposta depois
    if (strstr((char *)p->payload, "GET /game-state"))
    {
        char *request = (char *)p->payload;
        char *params_start = strchr(request, '?'); // Encontra os parâmetros de query

        char input = 'L';

        if (params_start != NULL)
        {
            char *input_param = strstr(params_start, "input=");
            if (input_param)
            {
                input = input_param[6];
            }
        }
        remoteInput = input;
        pendingPlayer = tpcb;
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
        return ERR_OK;
    }

    // Caso for outro pedido, retorne 404
    const char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    tcp_close(tpcb);
    pbuf_free(p);
    return ERR_OK;
}

// Envia o estado jogo para o cliente
void sendGameState(Game *game)
{
    gameToJSON(game, httpBuffer, sizeof(httpBuffer));

    tcp_write(pendingPlayer, header, strlen(header), TCP_WRITE_FLAG_COPY);
    tcp_write(pendingPlayer, httpBuffer, strlen(httpBuffer), TCP_WRITE_FLAG_COPY);
    tcp_close(pendingPlayer);
    pendingPlayer = NULL; // Limpa para o jogador fazer outro pedido
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_callback); // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor TCP
static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);                // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback); // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}