#include <Engine/CNCurses.h>
#include <Engine/CStateManager.h>
#include <Game/CStateMenu.h>
#include <Game/CStateGame.h> 
#include <vector>
using namespace std;

int run()
	{
		CNCurses::init();
		
		CStateManager states;
		try
		{
			states . run ( new CStateGame ( "default", CGameDifficulty::MEDIUM) );
			CNCurses::exit();
		}
		catch ( CStateManagerQuitExeptionReturnScore & e)
		{
			CNCurses::exit();
			return e.getScore();
		}
		//states . run ( new CStateMenu );
		
		
		return 0;
	}

int run(std::vector<int> params)
	{
		CNCurses::init();
		
		CStateManager states;
		try
		{
			states . run ( new CStateGame ( "default", CGameDifficulty::MEDIUM, params ));
			CNCurses::exit();
		}
		catch ( CStateManagerQuitExeptionReturnScore & e)
		{
			CNCurses::exit();
			return e.getScore();
		}
		//states . run ( new CStateMenu );
		
		
		return 0;
	}

int main(int argc, char const *argv[])
	{
		/*CNCurses::init();
		
		CStateManager states;
		try
		{
			vector<int> params = {2,2,3,4,0,1};
			states . run ( new CStateGame ( "default", CGameDifficulty::MEDIUM , params) );
			CNCurses::exit();
		}
		catch ( CStateManagerQuitExeptionReturnScore & e)
		{
			CNCurses::exit();
			printf("%d\n", e.getScore());
		}*/
		printf("%d\n", run());
		vector<int> params = {2,2,3,4,0,1};
		//for each iteration (each update) take one element from params
		// 0 do nothing (continue), 1 go right, 2 go left, 3 go up, 4 go down
		printf("%d\n", run(params));
		//states . run ( new CStateMenu );
		
		
		return 0;
	}