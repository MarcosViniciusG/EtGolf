#include "game.c"

extern void gameLoop();            
extern void initGame(Game *game); 
extern void shuffleDeck(Game *game);          
extern void dealCards(Game *game);            
extern bool evaluateTurn(Game *game);         
extern bool allCardsFaceUp(const Player *player); 
extern void endGame(Game *game);              
extern void updateScore(Player *player);                                 
extern void turnAllCards(Game *game);         

extern bool commandChooseGameMode(Game *game);
extern bool commandUpChooseGameMode(Game *game);
extern bool commandDownChooseGameMode(Game *game);
extern bool commandLeft(Game *game);
extern bool commandRight(Game *game);
extern bool commandUp(Game *game);
extern bool commandDown(Game *game);      
extern bool commandChooseDeck(Game *game);   
extern bool commandChooseDiscard(Game *game); 
extern bool commandFlip(Game *game);          
extern bool commandReplace(Game *game);       

extern void processChooseGameMode(Game *game);
extern void processFirstMove(Game *game);     
extern void processChooseSource(Game *game);  
extern void processCardAction(Game *game);    

extern void aiProcessFirstMove(Game *game);
extern void aiProcessChooseSource(Game *game);
extern void aiProcessCardAction(Game *game);

// Para implementar (Entrada)
extern char getInput(); 

// Para implementar (Saída)
extern void displayChooseGameMode(Game *game);
extern void displayGameInit(Game *game);
extern void displayGame(Game *game);
extern void displayFirstMove(Game *game);
extern void displayChooseSource(Game *game);
extern void displayCardAction(Game *game);
extern void displayGameEnd(Game *game);

// Para implementar (Rede)
extern bool setupNetwork(Game *game);
extern void sendGameState(Game *game);
extern void receiveGameState(Game *game);
extern char getRemoteInput(Game *game);
extern void closeServer();