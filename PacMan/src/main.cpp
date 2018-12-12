#include <Engine/CNCurses.h>
#include <Engine/CStateManager.h>
#include <Game/CStateMenu.h>
#include <Game/CStateGame.h> 

int main(int argc, char const *argv[])
	{
		CNCurses::init();
		
		CStateManager states;
		try
		{
			states . run ( new CStateGame ( "default", CGameDifficulty::MEDIUM ) );
			CNCurses::exit();
		}
		catch ( CStateManagerQuitExeptionReturnScore & e)
		{
			CNCurses::exit();
			printf("%d\n", e.getScore());
		}
		//states . run ( new CStateMenu );
		
		
		return 0;
	}
