#include <Engine/CNCurses.h>
#include <Engine/CStateManager.h>
#include <Game/CStateMenu.h>
#include <Game/CStateGame.h>
 


int main(int argc, char const *argv[])
	{
		CNCurses::init();
		
		CStateManager states;
		states . run ( new CStateGame ( "default", CGameDifficulty::MEDIUM ) );
		//states . run ( new CStateMenu );
		
		CNCurses::exit();
		
		return 0;
	}
