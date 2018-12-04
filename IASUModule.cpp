#include "IASUModule.h"

#include <ctime>
#include <iostream>

using namespace BWAPI;
using namespace Filter;
using namespace NEAT;

static Population *pop;
std::vector<int> IASUModule::indiv;
static time_t lastKill;
static time_t gameStart;

//use this to create part of the genefile (only needed once)
void generate_genefile() {

	ofstream of("bwapi-data/read/g", ios::out);

	//7 traits minimum
	int trait = 0, n = 3;

	//nodes :
	//bias and middle node
	of << "node 1 0 1 3 1" << endl << "node 2 0 0 0 0" << endl;

	//PREVIOUS network

	//p_offset = 6 * 400, a_offset = p_offset + 6 * 400,
	//	eb_offset = a_offset + 6 * 400, eu_offset = eb_offset + 7 * 400; // enemy buildings and units
	//+ 7*400 + MAX_MAP_SIZE * MAX_MAP_SIZE

	//NEW network
	// -> all units grouped together, merged status with hp, aggregate position
	//input : 400 * 4 (id, pos, status, type) + 400 * 3 (enemy) 
	//+ 200 * 3 (neutral) + 20 (misc) + 50 (reentrant)
	//output : 50*3 (id, action, position) + 50 (reentrant)

	///* gene : traitnum, input_nodenum, output_nodenum, weight, recur, innovation_num, mutation_num, enable */

	//friendly units
	//400 units
	for (int i = 0; i < 4 * 400; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//enemy
	//400 units
	for (int i = 0; i < 3 * 400; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//map
	//300 mineral patches + gas, 50 neutral units, 10 for new creep ?
	for (int i = 0; i < 2 * 300; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	for (int i = 0; i < 2 * 50; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//new creep
	for (int i = 0; i < 2 * 10; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//reentrant nodes
	for (int i = 0; i < 50; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//misc
	for (int i = 0; i < 20; i++) {
		of << "node " << n << " " << trait << " 1 1 1" << endl;
		of << "gene " << trait + 1 << " " << n++ << " 2 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//outputs

	//orders
	for (int i = 0; i < 3*50; i++) {
		of << "node " << n << " 0 0 2 0" << endl;
		of << "gene " << trait + 1 << " 2 " << n++ << " 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	//reentrant nodes
	for (int i = 0; i < 50; i++) {
		of << "node " << n << " 0 0 2 0" << endl;
		of << "gene " << trait + 1 << " 2 " << n++ << " 0.0 0 1 0 1" << endl;
	}
	trait++;
	of << endl << endl;

	/*of << "node " << n++ << " 0 0 2 0" << endl;
	of << "node " << n++ << " 0 0 2 0" << endl;

	of << "gene " << trait + 1 << " 2 " << (n-2) << " 0.0 0 1 0 1" << endl;
	of << "gene " << trait + 1 << " 2 " << (n-1) << " 0.0 0 1 0 1" << endl;*/

		/* node : id, trait, type, label(place), ??? */
		/* (see nnode.h) */
		/* nodetype {		NEURON = 0,		SENSOR = 1 */

		/* nodeplace {HIDDEN = 0,INPUT = 1, OUTPUT = 2, BIAS = 3 */

		/* functype {SIGMOID = 0 */

		//node 1 0 1 3 1
		//node 2 0 1 1 1
		//node 3 0 1 1 1
		//node 4 0 1 1 1
		//node 5 0 1 1 1
		//node 6 0 1 1 1
		//node 7 0 1 1 1
		//node 8 0 1 1 1
		//node 9 0 1 1 1
		//node 10 0 1 1 1
		//node 11 0 1 1 1
		//node 12 0 0 0 0
		//node 13 0 0 2 0
		//node 14 0 0 2 0
		///* gene : traitnum, input_nodenum, output_nodenum, weight, recur, innovation_num, mutation_num, enable */
		//gene 1 1 12 0.0 0 1 0 1
		//gene 1 2 12 0.0 0 2 0 1
		//gene 1 3 12 0.0 0 3 0 1
		//gene 1 4 12 0.0 0 4 0 1
		//gene 1 5 12 0.0 0 5 0 1
		//gene 1 6 12 0.0 0 6 0 1
		//gene 1 7 12 0.0 0 7 0 1
		//gene 1 8 12 0.0 0 8 0 1
		//gene 1 9 12 0.0 0 9 0 1
		//gene 1 10 12 0.0 0 10 0 1
		//gene 1 11 12 0.0 0 11 0 1
		//gene 1 12 13 0.0 0 12 0 1
		//gene 1 12 14 0.0 0 13 0 1

	of << "/* Total : " << n << " nodes */" << endl;
	of << "/* " << trait << " traits used */" << endl;
}

void IASUModule::onStart()
{


	//uncomment to generate input file
	//generate_genefile();

	//***********RANDOM SETUP***************//
	srand((unsigned)time(NULL));//  +getpid());

	//set game speed and skip frames to run faster
#ifdef _DEBUG
	Broodwar->setLocalSpeed(20);
	//Broodwar->setFrameSkip(500);
#else // RELEASE
	Broodwar->setLocalSpeed(20);
	//Broodwar->setFrameSkip(1000);
#endif // _DEBUG



	// Print the map name.
	// BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
	//Broodwar << "The map is " << Broodwar->mapName() << " !" << std::endl;


	// Enable the UserInput flag, which allows us to control the bot and type messages.
	//Broodwar->enableFlag(Flag::UserInput);

	// Uncomment the following line and the bot will know about everything through the fog of war (cheat).
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	// Set the command optimization level so that common commands can be grouped
	// and reduce the bot's APM (Actions Per Minute).
	Broodwar->setCommandOptimizationLevel(2);

	//redirect cout to file
	std::ofstream out("bwapi-data/write/out.txt", ios::app);
	std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
	std::cout.rdbuf(out.rdbuf());
	std::time_t t = std::time(0);   // get time and print it
	std::tm* now = std::localtime(&t);
	std::cout << endl << (now->tm_year + 1900) << (now->tm_mon + 1) << now->tm_mday << '_' << now->tm_hour << '_' << now->tm_min << '_' << now->tm_sec << endl;

	//neat init
	NEAT::load_neat_params("bwapi-data/read/neatsettings.ne", false);


	pop = init_novelty_realtime("bwapi-data/write/");

	for (auto &u : Broodwar->self()->getUnits()) {

		// Ignore the unit if it no longer exists
		// Make sure to include this block when handling any Unit pointer!
		if (!u->exists())
			continue;

		if (u->getType().isWorker()) {
			u->gather(u->getClosestUnit(IsMineralField || IsRefinery));
		}
	}
	lastKill = std::time(0) + 2*60;
	gameStart = std::time(0);

	std::cout.rdbuf(coutbuf);
}

void IASUModule::onEnd(bool isWinner)
{
	//redirect cout to file
	std::ofstream out("bwapi-data/write/out.txt", ios::app);
	std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
	std::cout.rdbuf(out.rdbuf());

	Broodwar->sendText("GG !");
	// Called when the game ends
	if (isWinner)
	{
		// Log your win here!
	}
	eval_one();
	
#ifdef _DEBUG
#ifdef _WIN32
	Broodwar->restartGame();
#endif
#else // RELEASE
	//TODO change map to a random one to gain a lot of time not using menus
	//(no need for now)
	//Broodwar->setMap("TODO->RANDOM-MAP");
#ifdef _WIN32
	Broodwar->restartGame();
#endif
#endif // _DEBUG
	std::cout.rdbuf(coutbuf);
}

void IASUModule::onFrame()
{
	// Called once every game frame

#ifndef BEST_MODE
	//end game if it lasts too long
	time_t curtime = time(0);
	int diffk = (int)difftime(curtime, lastKill), diffg = (int)difftime(curtime, gameStart);
	if(diffk > 2*60 || diffg > 20*60){
        Broodwar << "No kill or game is too long. Aborting." << std::endl;
        Broodwar->leaveGame();
	}
    Broodwar->drawTextScreen(400, 5, "Game time : %d:%d", diffg/60, diffg%60);
    Broodwar->drawTextScreen(400, 25, "No-kill time : %d:%d", diffk/60, diffk%60);
#endif

	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(200, 5, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 25, "Average FPS: %d", (int)Broodwar->getAverageFPS());
	//Broodwar->drawTextScreen(200, 40, "Average fitness: %f", pop->mean_fitness);
	Broodwar->drawTextScreen(10, 80, "Generation #%d", IASUGameWrapper::generation);
	Broodwar->drawTextScreen(10, 100, "Indiv #%d", IASUGameWrapper::indiv);

	// Return if the game is a replay or is paused
	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;

	network_step();

	/*
	// Iterate through all the units that we own
	for (auto &u : Broodwar->self()->getUnits())
	{

		// Ignore the unit if it no longer exists
		// Make sure to include this block when handling any Unit pointer!
		if (!u->exists())
			continue;

		// Ignore the unit if it has one of the following status ailments
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
			continue;

		// Ignore the unit if it is in one of the following states
		if (u->isLoaded() || !u->isPowered() || u->isStuck())
			continue;

		// Ignore the unit if it is incomplete or busy constructing
		if (!u->isCompleted() || u->isConstructing())
			continue;


		// Finally make the unit do some stuff!

		// If the unit is a worker unit
		if (u->getType().isWorker())
		{
			// if our worker is idle
			if (u->isIdle())
			{
				// Order workers carrying a resource to return them to the center,
				// otherwise find a mineral patch to harvest.
				if (u->isCarryingGas() || u->isCarryingMinerals())
				{
					u->returnCargo();
				}
				else if (!u->getPowerUp())  // The worker cannot harvest anything if it
				{                             // is carrying a powerup such as a flag
				  // Harvest from the nearest mineral patch or gas refinery
					if (!u->gather(u->getClosestUnit(IsMineralField || IsRefinery)))
					{
						// If the call fails, then print the last error message
						//Broodwar << Broodwar->getLastError() << std::endl;
					}

				} // closure: has no powerup
			} // closure: if idle

		}
		else if (u->getType().isResourceDepot()) // A resource depot is a Command Center, Nexus, or Hatchery
		{

			// Order the depot to construct more workers! But only when it is idle.
			if (u->isIdle() && !u->train(u->getType().getRace().getWorker()))
			{
				// If that fails, draw the error at the location so that you can visibly see what went wrong!
				// However, drawing the error once will only appear for a single frame
				// so create an event that keeps it on the screen for some frames
				Position pos = u->getPosition();
				Error lastErr = Broodwar->getLastError();
				Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
					nullptr,    // condition
					Broodwar->getLatencyFrames());  // frames to run

// Retrieve the supply provider type in the case that we have run out of supplies
				UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
				static int lastChecked = 0;

				// If we are supply blocked and haven't tried constructing more recently
				if (lastErr == Errors::Insufficient_Supply &&
					lastChecked + 400 < Broodwar->getFrameCount() &&
					Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0)
				{
					lastChecked = Broodwar->getFrameCount();

					// Retrieve a unit that is capable of constructing the supply needed
					Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first &&
						(IsIdle || IsGatheringMinerals) &&
						IsOwned);
					// If a unit was found
					if (supplyBuilder)
					{
						if (supplyProviderType.isBuilding())
						{
							TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
							if (targetBuildLocation)
							{
								// Register an event that draws the target build location
								Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*)
								{
									Broodwar->drawBoxMap(Position(targetBuildLocation),
										Position(targetBuildLocation + supplyProviderType.tileSize()),
										Colors::Blue);
								},
									nullptr,  // condition
									supplyProviderType.buildTime() + 100);  // frames to run

			// Order the builder to construct the supply structure
								supplyBuilder->build(supplyProviderType, targetBuildLocation);
							}
						}
						else
						{
							// Train the supply provider (Overlord) if the provider is not a structure
							supplyBuilder->train(supplyProviderType);
						}
					} // closure: supplyBuilder is valid
				} // closure: insufficient supply
			} // closure: failed to train idle unit

		}


	} // closure: unit iterator
 */
}

void IASUModule::onSendText(std::string text)
{

	// Send the text to the game if it is not being processed.
	Broodwar->sendText("%s", text.c_str());


	// Make sure to use %s and pass the text as a parameter,
	// otherwise you may run into problems when you use the %(percent) character!

}

void IASUModule::onReceiveText(BWAPI::Player player, std::string text)
{
	// Parse the received text
	//Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
	if(text == "GG !" && Broodwar->self()->isVictorious()) IASUGameWrapper::isVictorious = true;
}

void IASUModule::onPlayerLeft(BWAPI::Player player)
{
	// Interact verbally with the other players in the game by
	// announcing that the other player has left.
	Broodwar->sendText("%s disconnected. Ciao :)", player->getName().c_str());
}

void IASUModule::onNukeDetect(BWAPI::Position target)
{

	// Check if the target is a valid position
	if (target)
	{
		// if so, print the location of the nuclear strike target
		Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	}
	else
	{
		// Otherwise, ask other players where the nuke is!
		Broodwar->sendText("Where's the nuke?");
	}

	// You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void IASUModule::onUnitDiscover(BWAPI::Unit unit)
{
}

void IASUModule::onUnitEvade(BWAPI::Unit unit)
{
}

void IASUModule::onUnitShow(BWAPI::Unit unit)
{
}

void IASUModule::onUnitHide(BWAPI::Unit unit)
{
}

void IASUModule::onUnitCreate(BWAPI::Unit unit)
{
	if (Broodwar->isReplay())
	{
		// if we are in a replay, then we will print out the build order of the structures
		if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
		{
			int seconds = Broodwar->getFrameCount() / 24;
			int minutes = seconds / 60;
			seconds %= 60;
			Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
		}
	}
    if(Broodwar->getFrameCount() < 1 || unit->getType().getID() == 35) return;

	if(unit->getPlayer()->getID() == Broodwar->self()->getID()) {
		IASUGameWrapper::unitScore++;
		addUnit(unit->getType().getID());
        if (unit->getType().isWorker()) {
            unit->gather(unit->getClosestUnit(IsMineralField || IsRefinery));
        }
	}
	else {
		//IASUGameWrapper::unitScore--;;
	}

}

void IASUModule::onUnitDestroy(BWAPI::Unit unit)
{
    if(unit->getPlayer()->getID() == Broodwar->enemy()->getID()) {
        IASUGameWrapper::killScore++;
        if(unit->getType().isBuilding()) IASUGameWrapper::killScore+=4;
    }
    //else IASUGameWrapper::killScore--;;
    if(!unit->getPlayer()->isNeutral())
        lastKill = time(0);
}

void IASUModule::onUnitMorph(BWAPI::Unit unit)
{
	if (Broodwar->isReplay())
	{
		// if we are in a replay, then we will print out the build order of the structures
		if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
		{
			int seconds = Broodwar->getFrameCount() / 24;
			int minutes = seconds / 60;
			seconds %= 60;
			Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
		}
	}
}

void IASUModule::onUnitRenegade(BWAPI::Unit unit)
{
}

void IASUModule::onSaveGame(std::string gameName)
{
}

void IASUModule::onUnitComplete(BWAPI::Unit unit)
{
    if(Broodwar->getFrameCount() < 1 || unit->getType().getID() == 35) return;
    if(unit->getPlayer()->getID() == Broodwar->self()->getID()) {
        IASUGameWrapper::unitScore++;
		addUnit(unit->getType().getID());
        if (unit->getType().isWorker()) {
            unit->gather(unit->getClosestUnit(IsMineralField || IsRefinery));
        }
    }
    else {
        //IASUGameWrapper::unitScore--;;
    }

}
