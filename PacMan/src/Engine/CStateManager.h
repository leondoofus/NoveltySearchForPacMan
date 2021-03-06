#ifndef CSTATEMANAGER_H_DEFINED
#define CSTATEMANAGER_H_DEFINED

//#define USE_NCURSES 1

#include <CState.h>

/// Controls states all over the game.
///
/// Contains the main game loop, make transitions 
/// between diferent gamestates, makes sure old
/// states are properly cleared, new ones initialized, etc.
class CStateManager
	{
	public:

		CStateManager ();
		virtual ~CStateManager();

		/// Imediatelly changes to `new_state`,
		/// cleans the old one.
		static void change ( CState * new_state );

		/// Imediatelly quits the game, cleans 
		/// the old state.
		static void quit ();

		static int quitScore(int _score);
		static int victory(int _score);

		/// Initializes basically everything. Place
		/// where all magic happens. Game never leaves
		/// this method.
		///
		/// If this method is left, game quits.
		void run ( CState * init_state );
		
	private:
		/// pointer to current state
		CState * current_state;	
	};

/// Exeption that's used to change one state instantly
/// to another
///
class CStateManagerChangeExeption 
	{
	public:
		CStateManagerChangeExeption( CState * _new_state)
		{
			new_state = _new_state;
		}
		
		CState * new_state;
	};

/// Exeption that's used in case of quitting the game
///	
class CStateManagerQuitExeption 
	{
	public:
		CStateManagerQuitExeption( )
		{}
		
	};

/// Exeption that's used in case of quitting the game and returning score
///	
class CStateManagerQuitExeptionReturnScore 
	{
	public:
		int score;
		CStateManagerQuitExeptionReturnScore(int _score )
		{
			score = _score;
		}
		int getScore()
		{
			return score;
		}
		
	};

/// Exeption that's used in case of victory
///	
class CStateManagerVictory 
	{
	public:
		int score;
		CStateManagerVictory(int _score )
		{
			score = _score;
		}
		int getScore()
		{
			return score;
		}
		
	};


#endif //CSTATEMANAGER_H_DEFINED
