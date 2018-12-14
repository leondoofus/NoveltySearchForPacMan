#include <CStateGame.h>
#include <CStateMenu.h>
#include <CStateManager.h>
#include <CDialog.h>
#include <CBoardUtils.h>
  

#include <vector>
#include <string>

template<typename T> void safe_delete(T*& a) 
	{
	  delete a;
	  a = NULL;
	}


CStateGame::CStateGame():
	game ( NULL ),
	will_exit ( false )
	{

	}
CStateGame::CStateGame( std::string map, int difficulty ):
	game ( NULL ),
	will_exit ( false),
	will_go_to_menu ( false ),
	map ( map ),
	difficulty ( difficulty ),
	network(NULL)
	{

	}
CStateGame::CStateGame( std::string map, int difficulty, vector<int> params ):
	game ( NULL ),
	will_exit ( false),
	will_go_to_menu ( false ),
	map ( map ),
	difficulty ( difficulty ),
	params(params),
	network(NULL)
	{

	}
CStateGame::CStateGame( std::string map, int difficulty, Network* network ):
	game ( NULL ),
	will_exit ( false),
	will_go_to_menu ( false ),
	map ( map ),
	difficulty ( difficulty ),
	network(network)
	{

	}
CStateGame::~CStateGame()
	{

	}
void CStateGame::load ()
	{
		try 
			{
				//if (params.size() == 0)
				if (!network)
				{
					game = new CGame ( map, difficulty );
					game -> start (); // level to chose
				} else
				{
					//game = new CGame ( map, difficulty, params );
					game = new CGame ( map, difficulty, network );
					game -> start (); // level to chose
				}
				
				// load scores
			}
		catch ( BadFileExeption & e )
			{
				
				safe_delete ( game );
				std::vector<std::string> v = {  "There was a problem with loading the map. ",
												"Please select another one.",
												"Returning back to main menu..." };
				CDialog::show ( v, "ERROR" );
				/*game = new CGame ( "", difficulty );
				game -> start ();*/
				will_go_to_menu = true;
			}
	}
void CStateGame::unload ()
	{
		safe_delete ( game );
	}
void CStateGame::update ()
	{
		if ( will_exit )
			CStateManager::quit();

		if ( will_go_to_menu )
			CStateManager::change ( new CStateMenu ());

		game -> handle_input ();
		game -> update ();

		if ( game -> will_exit() )
			{
				if ( game -> high_score >= game -> points)
					game -> save_high_score ();

				will_exit = true;
			}
		if ( game -> will_go_to_menu () )
			{
				if ( game -> high_score >= game -> points )
					game -> save_high_score ();

				CStateManager::change ( new CStateMenu ());
			}

	}
void CStateGame::draw ()
	{
		if ( ! will_exit )
			game -> draw ();
	}
