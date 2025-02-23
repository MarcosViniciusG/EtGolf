#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "inc/wifi.h"
#include "inc/http.h"
#include "hardware/i2c.h"

// ----- Macros -----
#define X_PIN 27
#define Y_PIN 26

#define BUTTON_A 5
#define BUTTON_B 6

#define I2C_SDA 14
#define I2C_SCL 15

#define JOYSTICK_LEFT_THRESHOLD 1500
#define JOYSTICK_RIGHT_THRESHOLD 2500
#define JOYSTICK_UP_THRESHOLD 1500
#define JOYSTICK_DOWN_THRESHOLD 2500
// ----- Fim dos macros -----

// Variáveis globais
bool WIFI_INITIATED = false;
bool WIFI_CONNECTED = false;
bool firstTime[3];

// ----- Implementação das funções de entrada -----
// Implementação da função getInput()
char getInput()
{
    char input;
    bool done = false;

    while (!done)
    {
        bool button_a_state = gpio_get(BUTTON_A);
        bool button_b_state = gpio_get(BUTTON_B);

        adc_select_input(1);
        uint16_t adc_x = adc_read();

        adc_select_input(0);
        uint16_t adc_y = adc_read();

        if (!button_a_state && !done)
        {
            input = 'A';
            done = true;
        }
        else if (!button_b_state && !done)
        {
            input = 'B';
            done = true;
        }

        if (adc_x < JOYSTICK_LEFT_THRESHOLD && !done)
        {
            input = 'L';
            done = true;
        }
        else if (adc_x > JOYSTICK_RIGHT_THRESHOLD && !done)
        {
            input = 'R';
            done = true;
        }

        if (adc_y < JOYSTICK_UP_THRESHOLD && !done)
        {
            input = 'D';
            done = true;
        }
        else if (adc_y > JOYSTICK_DOWN_THRESHOLD && !done)
        {
            input = 'U';
            done = true;
        }

        // Delay para evitar problemas de debounce
        sleep_ms(100);
    }

    return input;
}

char getRemoteInput(Game *game)
{
    // Enquanto o remote input não mudar, somente manda os dados
    while (remoteInput == '#')
    {
        sleep_ms(50);
    }
    char input = remoteInput;
    remoteInput = '#';
    return input;
}

void sendRemoteInput()
{
    sendRemote = true;
}
// ----- Fim da implementação das funções de entrada -----

// Tenta conectar ao WiFi
bool initWifi(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    if (cyw43_arch_init())
    {
        ssd1306_draw_string(gameDisplay->ssd, 5, 32, "ERRO AO");
        ssd1306_draw_string(gameDisplay->ssd, 5, 40, "INICIALIZAR WIFI");
        render_on_display(gameDisplay->ssd, &frame_area);
        sleep_ms(3000);
        return false;
    }

    cyw43_arch_enable_sta_mode();

    return true;
}

bool connectWifi(Game *game)
{
    if (!WIFI_INITIATED)
        return false;

    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);

    ssd1306_draw_string(gameDisplay->ssd, 5, 32, "CONECTANDO...");
    render_on_display(gameDisplay->ssd, &frame_area);
    // Altere em inc/wifi.h caso queira mudar a rede WiFi
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        clearDisplay(gameDisplay);
        ssd1306_draw_string(gameDisplay->ssd, 5, 32, "ERRO AO");
        ssd1306_draw_string(gameDisplay->ssd, 5, 40, "CONECTAR WIFI");
        render_on_display(gameDisplay->ssd, &frame_area);
        sleep_ms(3000);
        return false;
    }

    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 32, "CONECTADO AO");
    ssd1306_draw_string(gameDisplay->ssd, 5, 40, WIFI_SSID);
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(3000);
    return true;
}

bool setupServer(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 32, "ABRINDO SERVIDOR...");
    render_on_display(gameDisplay->ssd, &frame_area);

    sleep_ms(1000);
    clearDisplay(gameDisplay);

    uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char buffer[50];
    sprintf(buffer, "%d.%d.%d.%d", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    ssd1306_draw_string(gameDisplay->ssd, 5, 32, buffer);
    render_on_display(gameDisplay->ssd, &frame_area);

    start_http_server();

    sleep_ms(10000);
}

void getServerIp(ip_addr_t *serverIp)
{
    IP4_ADDR(serverIp, 192, 168, 18, 54);
}

void receiveGameState(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    struct tcp_pcb *pcb = tcp_new();
    tcp_arg(pcb, game);
    ip_addr_t serverIp;
    getServerIp(&serverIp);
    err_t erro = tcp_connect(pcb, &serverIp, 80, http_client_connected);
}

// ----- Fim das funções auxiliares -----

// ----- Implementação das funções de saída -----
void displayChooseGameMode(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);

    char *text[] = {
        "VS AI",
        "SERVER",
        "CLIENT"};

    int y = 32;
    for (uint8_t i = 0; i < count_of(text); i++)
    {
        if (game->selectedGameMode == i)
        {
            ssd1306_draw_string(gameDisplay->ssd, 5, y, "=>");
        }

        ssd1306_draw_string(gameDisplay->ssd, 25, y, text[i]);
        y += 8;
    }

    render_on_display(gameDisplay->ssd, &frame_area);
}

void displayGameInit(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 32, "INICIO DO JOGO");
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(3000);
}

void displayFirstMove(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    if (firstTime[0])
    {
        firstTime[0] = false;
        char playerString[16];
        snprintf(playerString, sizeof(playerString), "JOGADOR %d", game->currentPlayer +1);
        ssd1306_draw_string(gameDisplay->ssd, 5, 5, playerString);

        ssd1306_draw_string(gameDisplay->ssd, 5, 13, "PRIMEIRO:");
        ssd1306_draw_string(gameDisplay->ssd, 5, 32, "(A) REVELAR");
        render_on_display(gameDisplay->ssd, &frame_area);
        sleep_ms(4000);
    }
    displayGame(game);
}

void displayChooseSource(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    if (firstTime[1])
    {
        firstTime[1] = false;
        char playerString[16];
        snprintf(playerString, sizeof(playerString), "JOGADOR %d", game->currentPlayer+1);
        ssd1306_draw_string(gameDisplay->ssd, 5, 5, playerString);

        ssd1306_draw_string(gameDisplay->ssd, 5, 13, "ESCOLHA:");
        ssd1306_draw_string(gameDisplay->ssd, 5, 18, "(A) BARALHO");
        ssd1306_draw_string(gameDisplay->ssd, 5, 32, "(B) DESCARTE");
        render_on_display(gameDisplay->ssd, &frame_area);
        sleep_ms(4000);
    }
    displayGame(game);
}

void displayCardAction(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    clearDisplay(gameDisplay);
    if (firstTime[2])
    {
        firstTime[2] = false;
        char playerString[16];
        snprintf(playerString, sizeof(playerString), "JOGADOR %d", game->currentPlayer+1);
        ssd1306_draw_string(gameDisplay->ssd, 5, 5, playerString);

        ssd1306_draw_string(gameDisplay->ssd, 5, 13, "ESCOLHA:");
        ssd1306_draw_string(gameDisplay->ssd, 5, 18, "(A) TROCAR");
        ssd1306_draw_string(gameDisplay->ssd, 5, 32, "(B) REVELAR");
        render_on_display(gameDisplay->ssd, &frame_area);
        sleep_ms(4000);
    }
    displayGame(game);
}

void displayGameEnd(Game *game)
{
    GameDisplay *gameDisplay = &game->gameDisplay;
    sendGameState(game);
    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 5, "FIM DE JOGO");
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(3000);
    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 5, "VENCEDOR: ");
    char playerString[16];
    // Após a ordenação
    snprintf(playerString, sizeof(playerString), "JOGADOR %d", game->players[0].ID);
    ssd1306_draw_string(gameDisplay->ssd, 5, 14, playerString);
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(5000);
    clearDisplay(gameDisplay);
    ssd1306_draw_string(gameDisplay->ssd, 5, 5, "PERDEDOR: ");
    snprintf(playerString, sizeof(playerString), "JOGADOR %d", game->players[1].ID);
    ssd1306_draw_string(gameDisplay->ssd, 5, 14, playerString);
    render_on_display(gameDisplay->ssd, &frame_area);
    sleep_ms(5000);
}
// ----- Fim da implementação das funções de saída -----

// ----- Implementação das funções de rede -----
bool setupNetwork(Game *game)
{

    if (!WIFI_INITIATED)
        WIFI_INITIATED = initWifi(game);

    WIFI_CONNECTED = connectWifi(game);

    bool status = true;
    if (game->gameMode == SERVER && WIFI_CONNECTED)
        status = setupServer(game);

    return (WIFI_INITIATED && WIFI_CONNECTED && status);
}

// Inicialização dos botões
void initButtons()
{
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

// Inicialização do joystick
void initJoystick()
{
    adc_init();
    adc_gpio_init(Y_PIN);
    adc_gpio_init(X_PIN);
}

// Inicialização do display
void initDisplay()
{
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
}

void initGameDisplay(GameDisplay *gameDisplay)
{
    // Inicialização do primeiro jogador
    gameDisplay->playersDisplay[0].xID = 1;
    gameDisplay->playersDisplay[0].yID = 2;

    gameDisplay->playersDisplay[0].xScore = 93;
    gameDisplay->playersDisplay[0].yScore = 2;

    gameDisplay->playersDisplay[0].xCards[0] = 11;
    gameDisplay->playersDisplay[0].xCards[1] = 30;
    gameDisplay->playersDisplay[0].xCards[2] = 49;

    gameDisplay->playersDisplay[0].yCards[0] = 2;
    gameDisplay->playersDisplay[0].yCards[1] = 12;
    gameDisplay->playersDisplay[0].yCards[2] = 22;

    // Inicialização do segundo jogador
    gameDisplay->playersDisplay[1].xID = 1;
    gameDisplay->playersDisplay[1].yID = 34;

    gameDisplay->playersDisplay[1].xScore = 93;
    gameDisplay->playersDisplay[1].yScore = 34;

    gameDisplay->playersDisplay[1].xCards[0] = 11;
    gameDisplay->playersDisplay[1].xCards[1] = 30;
    gameDisplay->playersDisplay[1].xCards[2] = 49;

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

int main()
{
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
    memset(game.gameDisplay.ssd, 0, 1024);

    ssd1306_draw_string(game.gameDisplay.ssd, 5, 32, "PRESSIONE (A)");
    render_on_display(game.gameDisplay.ssd, &frame_area);

    char input = 'B';
    while (true)
    {
        input = getInput();
        if (input == 'A') {
            memset(firstTime, true, sizeof(firstTime));
            gameLoop(&game);
        }
    }
}