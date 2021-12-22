

#ifndef __Main_H__
#define __Main_H__ 

#include "./TetrisGame/TetrisGame.h"
/*  
 ~~~~~~~~~~~~~~~~
 Wordclock-Project
 ~~~~~~~~~~~~~~~~
© SERENE - Rene Lang 2021 (http://withstupid.net)
 
*/

typedef enum gameCommand {GameCommandNone, GameCommandLeft, GameCommandRight, GameCommandDown, GameCommandFire, GameCommandStartStop} _gameCommand;
extern void handleTetrisCommand(gameCommand cmd);
extern void handleClientServer(); // Webserver-Maintain

#endif 
