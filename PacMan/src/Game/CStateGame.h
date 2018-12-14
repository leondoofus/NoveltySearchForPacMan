#ifndef CSTATEGAME_H_DEFINED
#define CSTATEGAME_H_DEFINED

#include <CState.h>
#include <CGame.h>

#include <vector>

#include <../../../neat.h>
#include <../../../network.h>

using namespace std;
using namespace NEAT;

class CStateGame : public CState
	{
	public:
		CStateGame();
		CStateGame( std::string map, int difficulty );
		CStateGame( std::string map, int difficulty, vector<int> params );
		CStateGame( std::string map, int difficulty, Network* network );
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
		Network* network;
		
	};

#endif //CSTATEGAME_H_DEFINED
