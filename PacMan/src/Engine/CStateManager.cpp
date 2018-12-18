#include <CStateManager.h>
#include <CTime.h>
#include <CInputManager.h>
#include <stdlib.h>

template<typename T> void safe_delete(T*& a) 
	{
	  delete a;
	  a = NULL;
	}

CStateManager::CStateManager ():
	current_state ( NULL )
	{

	}
CStateManager::~CStateManager()
	{
		if ( current_state) current_state -> unload ();
		safe_delete ( current_state );
	}
void CStateManager::change ( CState * new_state )
	{
		throw CStateManagerChangeExeption (new_state);
	}
void CStateManager::quit ()
	{
		throw CStateManagerQuitExeption();
	}
int CStateManager::quitScore (int _score)
	{
		throw CStateManagerQuitExeptionReturnScore(_score);
	}
int CStateManager::victory (int _score)
	{
		throw CStateManagerVictory(_score);
	}
void CStateManager::run ( CState * init_state )
	{
		if ( ! init_state)
			throw "no init state";
		current_state = init_state;
		current_state -> load ();

		while ( true )
			{
				try 
					{
#ifdef USE_NCURSES
						CInputManager::update ();
#endif
						current_state -> update ();
#ifdef USE_NCURSES
						if ( current_state) current_state -> draw ();
						CTime::delay_ms (120);
#endif
					//CTime::delay_ms (0);
				}
				catch ( CStateManagerChangeExeption & e )
					{
						current_state -> unload ();
						safe_delete ( current_state );
						current_state = e.new_state;
						current_state -> load ();
					}
				catch ( CStateManagerQuitExeption & e)
					{
						current_state -> unload ();
						safe_delete ( current_state );
						break;
					}
			}
	}
