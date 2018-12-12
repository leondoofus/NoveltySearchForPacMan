#ifndef CSTATEGAME_H_DEFINED
#define CSTATEGAME_H_DEFINED

#include <Engine/CState.h>
#include <Game/CGame.h>

#include <vector>
using namespace std;

class CStateGame : public CState
	{
	public:
		CStateGame();
		CStateGame( std::string map, int difficulty );
		CStateGame( std::string map, int difficulty, vector<int> params );
		virtual ~CStateGame();
		virtual void load ();
		void unload ();

		void update ();
		void draw ();

	private:
		CGame * game;
		bool will_exit;
		bool will_go_to_menu;
		std::string map;
		unsigned int difficulty;
		vector<int> params;
		
	};

#endif //CSTATEGAME_H_DEFINED
