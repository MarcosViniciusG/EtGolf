#include "game.c"

extern void gameLoop();            
extern void initGame(Game *game); 
extern void shuffleDeck(Game *game);          
extern void dealCards(Game *game);            
extern bool evaluateTurn(Game *game);         
extern bool allCardsFaceUp(const Player *player); 
extern void endGame(Game *game);              
extern void updateScore(Player *player);           
extern char getInput();                       
extern void turnAllCards(Game *game);         

extern bool commandLeft(Game *game);
extern bool commandRight(Game *game);
extern bool commandUp(Game *game);
extern bool commandDown(Game *game);      
extern bool commandChooseDeck(Game *game);   
extern bool commandChooseDiscard(Game *game); 
extern bool commandFlip(Game *game);          
extern bool commandReplace(Game *game);       

extern void processFirstMove(Game *game);     
extern void processChooseSource(Game *game);  
extern void processCardAction(Game *game);    

extern void aiProcessFirstMove(Game *game);
extern void aiProcessChooseSource(Game *game);
extern void aiProcessCardAction(Game *game);

extern void displayGameInit(const Game *game);
extern void displayCard(const Card *card);
extern void displayHand(const Player *player);
extern void displayScore(const Player *player);
extern void displayDiscardTop(const Game *game);
extern void displayChosen(const Game *game);
extern void displayFirstMove(const Game *game);
extern void displayChooseSource(const Game *game);
extern void displayCardAction(const Game *game);
extern void displayGameEnd(const Game *game);