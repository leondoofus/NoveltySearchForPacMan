#include "IASUGameWrapper.h"

using namespace Filter;

// uncomment this to prevent learning and only play with one of the best versions so far
#define BEST_MODE

#define DEBUG_BROOD
#define MAX_INPUTS 3594
#define MAX_OUTPUTS 200

#define NB_REENTRANT 50

static char output_dir[100] = "bwapi-data/write/";
string genefile = "bwapi-data/read/startgenes";
string current_genefile = "bwapi-data/write/genefile";
string current_popfile = "bwapi-data/write/popfile";
string indiv_file = "bwapi-data/write/indiv";
string archive_file = "bwapi-data/write/archive";
string best_file = "bwapi-data/read/best";
string popmap = "bwapi-data/write/popmap";
string archivemap = "bwapi-data/write/archivemap";

static Population *pop;
//static int param = -1;
static bool isFirstGen = false;
static Organism* curorg;
static int offspring_count;
static int num_species_target;
static int compat_adjust_frequency;
data_record* newrec;

static double reentrant_nodes[NB_REENTRANT];
static std::vector<float> build_order;
static std::map<int,int> realToCustomID;
static std::map<int,int> customToRealID;

static float archive_thresh = 6.0; //initial novelty threshold

//archive of novel behaviors
static noveltyarchive archive(archive_thresh, *novelty_metric, false);

int IASUGameWrapper::indiv = 1;
int IASUGameWrapper::generation = 0;
int IASUGameWrapper::killScore = 0;
int IASUGameWrapper::unitScore = 0;
bool IASUGameWrapper::isVictorious = false;

//---------------------------------------------------------------------
//TODO : add mmap implemetation
//check for remaining non-serialized variables (in archive...)
//---------------------------------------------------------------------

void addUnit(float id){
	build_order.push_back(id);
}

//novelty metric to evaluate individual
float novelty_metric(noveltyitem* x, noveltyitem* y)
{
	float diff = 0.0;
	//for (int k = 0; k < (int)x->data.size() && k < (int)y->data.size(); k++)
	//{
    int k = 0;
    if(x->data[k].size() != 0) {
        if (y->data[k].size() == 0) {
            diff += x->data[k].size() * 50;
            //continue;
        }
        diff += hist_diff(x->data[k], y->data[k]);
    }
    k=1;
    if(x->data[k].size() != 0) {
        for (int l = 0; l < (int) x->data[k].size() && l < (int) y->data[k].size(); l++) {
            if (x->data[k][l] != y->data[k][l]) diff += 50;
        }
        if (x->data[k].size() > y->data[k].size())
            diff += x->data[k].size() - y->data[k].size() * 70; //reward additional units
    }
	//}
	if(x->data.size() > y->data.size())
		diff += 200 * (x->data.size() - y->data.size());	//reward additional vector
	return diff;
}

//evaluates an individual after an entire game and stores the novelty point
//a novelty point is a 2D array with the time of each unit/structure creation (and maybe the destroyed ennemy buildings)
noveltyitem* eval_novelty(Organism *org, data_record* record)
{
	//to compare two points, we can aggregate the number of units of each kind at different times (e.g. every 30 secs),
	//or try to match the build orders (% similarity), which would be more accurate
	//note : a building that has been rebuilt should not be taken into account as it is not something we want in any case

	noveltyitem *new_item = new noveltyitem;
	new_item->genotype = new Genome(*(org->gnome));
	new_item->phenotype = new Network(*(org->net));
	//vector to caracterize novelty
	vector< vector<float> > gather;

	vector<float> *scores = new vector<float>();
	gather.push_back(*scores);
	gather.push_back(build_order);
	gather[0].push_back(IASUGameWrapper::unitScore*5);
	gather[0].push_back(Broodwar->getFrameCount()/2520);
	gather[0].push_back(IASUGameWrapper::killScore*7);
	gather[0].push_back((Broodwar->self()->gatheredMinerals() - 50)/10);
	gather[0].push_back(Broodwar->self()->gatheredGas()/10);
	//gather[0].push_back(Broodwar->getAPM() / 1000.0);
	gather[0].push_back((int)(IASUGameWrapper::isVictorious) *1000);

	double fitness;
	static float highest_fitness = 0.0;

	fitness = Broodwar->self()->gatheredMinerals() + Broodwar->self()->gatheredGas()*2 - Broodwar->getFrameCount() / 1000.0 -
	        Broodwar->getAPM() / 1000.0 + IASUGameWrapper::killScore*50 + IASUGameWrapper::unitScore*5
	        + (int)(Broodwar->self()->isVictorious() && !Broodwar->enemy()->leftGame()) *10000;

	if (fitness > highest_fitness)
		highest_fitness = (float)fitness;

	//keep track of highest fitness so far in record
	if (record != NULL)
	{
		/*
	record->ToRec[19]=org->gnome->parent1;
	record->ToRec[18]=org->gnome->parent2;
	record->ToRec[17]=org->gnome->struct_change;
		*/
		record->ToRec[RECSIZE - 1] = highest_fitness;
	}

	//push back novelty characterization
	new_item->data.push_back(gather[0]);
    new_item->data.push_back(gather[1]);
	//set fitness (this is 'real' objective-based fitness, not novelty)
	new_item->fitness = (float)fitness;
	return new_item;
}

//get indiv to test and current generation
void read_indiv_number() {
	char curword[128];  //max word size of 128 characters
	char curline[1024]; //max line size of 1024 characters
	ifstream ifs(indiv_file, ios::in);
	if (ifs) {
		while (!ifs.eof())
		{
			ifs.getline(curline, sizeof(curline));
			std::stringstream ss(curline);
			//strcpy(curword, NEAT::getUnit(curline, 0, delimiters));
			ss >> curword;
			if (ifs.eof()) break;
			//std::cout << curline << std::endl;

			//Check for next
			if (strcmp(curword, "generation") == 0)
			{
				ss >> IASUGameWrapper::generation;
			}
			else if (strcmp(curword, "indiv") == 0)
			{
				ss >> IASUGameWrapper::indiv;
			}
		}
	}
	ifs.close();

	ofstream ofs(indiv_file, ios::out);
	if (IASUGameWrapper::indiv > 1 && IASUGameWrapper::indiv % NEAT::pop_size == 1) {
		IASUGameWrapper::generation = IASUGameWrapper::generation + 1;
		//IASUGameWrapper::indiv = 0;
	}
	ofs << "generation " << IASUGameWrapper::generation << std::endl << "indiv " << (IASUGameWrapper::indiv + 1) << std::endl;
	ofs.close();

	Broodwar << "Indiv # : " << IASUGameWrapper::indiv << ", generation " << IASUGameWrapper::generation << std::endl;
}

//novelty initialization
//call this when AI starts
Population *init_novelty_realtime(const char* outputdir) {
	Genome *start_genome;
	//genome
	char curword[20];
	int id;

	//param = par;
	if (outputdir != NULL) strcpy(output_dir, outputdir);


#ifdef BEST_MODE
	std::srand((unsigned)time(NULL));
    pop = new Population(best_file.c_str());
	int rn = std::rand();
    int r = (int)(rn % pop->organisms.size());
    curorg = pop->organisms[r];
    Broodwar << "Best mode : picked indiv #" << curorg->gnome->genome_id <<
        " (fitness " << curorg->noveltypoint->fitness << ", novelty " << curorg->noveltypoint->novelty << ")" << std::endl;
    return pop;
#endif


	//starter genes file
	ifstream iFile(current_genefile, ios::in);
	if (!iFile.is_open()) {
		Broodwar << "No startgene found in write folder. Copying from read folder." << endl;
		//get current generation from file
		cout << "(current generation file) " << current_genefile << endl;
		iFile.close();
		isFirstGen = true;

		//no file generated yet, getting file from read folder
		cout << "(startgenes) " << genefile << endl;

		iFile.open(genefile, ios::in);
		if (!iFile.is_open()) {
			Broodwar << "ERROR : CANNOT OPEN INPUT FILE !!!" << endl;
			cout << "Error : Unable to open input file";
			iFile.close();
			exit(EXIT_FAILURE);
		}

		//copy file to write folder for future use
		ifstream ifs(genefile, ios::binary);
		ofstream ofs(current_genefile, ios::binary);
		ofs << ifs.rdbuf();
		ifs.close();
		ofs.close();

	}

	//Spawn the Population from starter gene if it is first iteration
	if (isFirstGen) {

        //cout << "START IASU NOVELTY REAL-TIME EVOLUTION VALIDATION" << endl;

        cout << "Reading in the start genome" << endl;
        //Read in the start Genome
        iFile >> curword;
        iFile >> id;
        cout << "Reading in Genome id " << id << endl;
        start_genome = new Genome(id, iFile);

        cout << "Start Genome: " << start_genome << endl;
        iFile.close();

		cout << "Spawning Population off Genome" << endl;
		pop = new Population(start_genome, NEAT::pop_size);

		//save population to file for future use
		pop->print_to_file(current_popfile, false);
	}
	else {
        iFile.close();

		// get population from memory map or file
		if(!pop) {
			read_indiv_number();
            cout << IASUGameWrapper::indiv << " : read pop (init)" << std::endl;
            pop = new Population(popmap.c_str(), true);
            if(!pop || pop->organisms.size() == 0) {
                cout << "No mmap found for pop ! Loading from file." << endl;
                pop = new Population(current_popfile.c_str(), false);
            }
        }
	}

#ifdef DEBUG_BROOD
	//Broodwar << "Verifying Pop..." << endl;
	//if (pop->verify()) {
	//	Broodwar << "Done." << endl;
	/*}
	else {
		Broodwar << "WARNING ! Verification returned error !" << endl;
	}*/
#endif

	if(!pop || pop->organisms.size() == 0){
		cout << "ERROR EMPTY POPULATION !" << endl;
		Broodwar << "ERROR EMPTY POPULATION !" << endl;
		exit(1);
	}

	cout << IASUGameWrapper::indiv << " : read id (init)" << std::endl;
    if (IASUGameWrapper::generation == 0) isFirstGen = true;

//	if(offspring_count > 0 ) IASUGameWrapper::indiv = offspring_count;

	//get our indiv from pop
	for (vector<Organism*>::iterator org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
		if ((*org)->gnome->genome_id == IASUGameWrapper::indiv) {
		    if(isFirstGen)
			    curorg = (*org);
            else {
                //prevent erasing previous entry
                read_indiv_number();
            }
		}
	}



    offspring_count = IASUGameWrapper::indiv;

	//We try to keep the number of species constant at this number                                                    
	num_species_target = NEAT::pop_size / 15;

	//This is where we determine the frequency of compatibility threshold adjustment
	compat_adjust_frequency = NEAT::pop_size / 20;
	if (compat_adjust_frequency < 1)
		compat_adjust_frequency = 1;

	//generate indiv if it does not exist
	if (curorg == NULL) {
        cout << IASUGameWrapper::indiv << " : generate organism (init)" << std::endl;

        //Here we call two rtNEAT calls:
        //1) choose_parent_species() decides which species should produce the next offspring
        //2) reproduce_one(...) creates a single offspring from the chosen species
        curorg = (pop->choose_parent_species())->reproduce_one(offspring_count, pop, pop->species);

//        cout << IASUGameWrapper::indiv << " : no org to load, generating new one (init)" << std::endl;
//		offspring_count--;
//		eval_one();
//		init_novelty_realtime(outputdir);
//		offspring_count = IASUGameWrapper::indiv;
	}

	//shouldn't happen                                                                                                        
	if ((curorg->gnome) == 0) {
		cout << "ERROR EMPTY GENOME!" << endl;
		exit(1);
	}

	for (double &reentrant_node : reentrant_nodes) reentrant_node = 0;

    cout << IASUGameWrapper::indiv << " : init done" << std::endl;
	return pop;
}


//NOTE : this function is unused and needs to be refactored
void final_print() {

	//vector<Species*>::iterator curspecies;
	//vector<Species*>::iterator curspec; //used in printing out debug info                                                         

	//vector<Species*> sorted_species;  //Species sorted by max fit org in Species 
	//Record disabled (it displays information about individuals during all the tests)
	//data_rec Record; //stores run information
	//Real-time evolution variables

	//write out run information, archive, and final generation
	cout << "COMPLETED...";
	//char filename[100];
	//sprintf(filename, "%srecord.dat", output_dir);
	//Record.serialize(filename);
	char fname[100];
	sprintf(fname, "%srtarchive.dat", output_dir);
	archive.Serialize(fname);

	sprintf(fname, "%sfittest_final", output_dir);
	archive.serialize_fittest(fname);

	sprintf(fname, "%srtgen_final", output_dir);
	pop->print_to_file_by_species(fname);
}

//actions to perform at the end of first generation
void first_gen_end() {

	//load archive from file
	//(done in previous function)
	//archive = noveltyarchive(archive_thresh, *novelty_metric, archive_file);

	//remove indiv if it was not evaluated and link noveltypoint to genotype and phenotype
	vector<Organism*>::iterator org = (pop->organisms).begin();
	while (org != pop->organisms.end()) {
		//(*org)->noveltypoint = archive.get_item_from_id((*org)->gnome->genome_id);
		if (!(*org)->noveltypoint) {
			org = pop->organisms.erase(org);
		}
		else {
			(*org)->noveltypoint->genotype = (*org)->gnome;
			(*org)->noveltypoint->phenotype = (*org)->net;
			++org;
		}
	}

	//assign fitness scores based on novelty, this creates archive
	archive.evaluate_population(pop, true);
	//add to archive
	archive.evaluate_population(pop, false);

	//Get ready for real-time loop
	//Rank all the organisms from best to worst in each species
	pop->rank_within_species();

	//Assign each species an average fitness 
	//This average must be kept up-to-date by rtNEAT in order to select species probabilistically for reproduction
	pop->estimate_all_averages();

	archive.Serialize(archive_file.c_str());
}

//evaluate an individual of first generation at the end of game
void first_gen_eval_one() {
	//Evaluate organism on a test                   
	int indiv_counter = offspring_count;// +NEAT::pop_size * IASUGameWrapper::generation;

	curorg->noveltypoint = eval_novelty(curorg);
	curorg->noveltypoint->indiv_number = indiv_counter;
	curorg->noveltypoint->genotype = curorg->gnome;

	// save curorg noveltypoint for later use
	//reload and save pop
	//... or archive
	/*archive = noveltyarchive(archive_thresh, *novelty_metric, archive_file);
	archive.add_novel_item(curorg->noveltypoint);*/

	//end first gen at pop_size
	if (offspring_count % NEAT::pop_size == 0) {
		first_gen_end();
	}
	//reload and save pop
	pop->print_to_file(current_popfile, true);
	//archive.Serialize(archive_file.c_str());
}

// evaluate individual at the end of the game
// this happens only AFTER the first generation
void eval_one() {

#ifdef BEST_MODE
	return;
#endif
	
	if (isFirstGen) {
		first_gen_eval_one();
		if (offspring_count % (NEAT::pop_size * 1) != 0)
			return;
	}

    cout << IASUGameWrapper::indiv << " : load archive (eval)" << std::endl;
	archive = noveltyarchive(archive_thresh, *novelty_metric, archivemap, true);
	if(archive.get_set_size() == 0) archive = noveltyarchive(archive_thresh, *novelty_metric, archive_file, false);

//    for (vector<Organism*>::iterator org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
//        if((*org)->noveltypoint && !(*org)->noveltypoint->added && (*org)->noveltypoint->genotype == (*org)->gnome){
//            cout << "ERROR (6)" << std::endl;
//        }
//    }

    cout << IASUGameWrapper::indiv << " : reload population (eval)" << std::endl;
    //reload pop
    if(!pop->reload(popmap.c_str(), true, true))
        pop->reload(current_popfile.c_str(), true, false);

//    for (vector<Organism*>::iterator org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
//        if((*org)->noveltypoint && !(*org)->noveltypoint->added && (*org)->noveltypoint->genotype == (*org)->gnome){
//            cout << "ERROR (6.5)" << std::endl;
//        }
//    }

    cout << IASUGameWrapper::indiv << " : remove not evaluated indiv (eval)" << std::endl;
	//remove indiv if it was not evaluated
	vector<Organism*>::iterator org = (pop->organisms).begin();
	while (org != pop->organisms.end()) {
		if (!(*org)->noveltypoint && (!curorg || curorg->gnome->genome_id != (*org)->gnome->genome_id)) {
			org = pop->organisms.erase(org);
		}
		else {
			++org;
		}
	}

	//Now we evaluate the new individual
	//Note that in a true real-time simulation, evaluation would be happening to all individuals at all times.
	//That is, this call would not appear here in a true online simulation.

	data_record* newrec = new data_record();
	newrec->indiv_number = offspring_count;
	//evaluate individual, get novelty point
	curorg->noveltypoint = eval_novelty(curorg, newrec);
	curorg->noveltypoint->indiv_number = newrec->indiv_number;
	//new_org->noveltypoint->genotype = new Genome(*new_org->gnome);
	//calculate novelty of new individual
	archive.evaluate_individual(curorg, pop);
	newrec->ToRec[4] = archive.get_threshold();
	newrec->ToRec[5] = archive.get_set_size();
	newrec->ToRec[RECSIZE - 2] = curorg->noveltypoint->novelty;

	//add record of new indivdual to storage
	//Record.add_new(newrec);
	//indiv_counter++;

	//update fittest list
	archive.update_fittest(curorg);

	//Now we reestimate the baby's species' fitness
	curorg->species->estimate_average();

	/*
	//write out the first individual to beat opponent
	if (!firstflag && newrec->ToRec[3] > 0.0) {//TODO change record format
		firstflag = true;
		char filename[30];
		sprintf(filename, "%srtgen_first", output_dir);
		pop->print_to_file_by_species(filename);
		cout << "Maze solved by indiv# " << newrec->indiv_number << endl;
	}
	*/
    if(IASUGameWrapper::isVictorious) {
        cout << IASUGameWrapper::indiv << " : Victory !!" << std::endl;
        curorg->winner = true;
        curorg->print_to_file(const_cast<char *>("bwapi-data/write/victory"));
    }

    //update fittest individual list
    archive.update_fittest(pop);
    //refresh generation's novelty scores
    archive.evaluate_population(pop, true);

	//end of generation
	if (offspring_count % (NEAT::pop_size * 1) == 0)
	{
        Broodwar << "-------------END OF GENERATION----------" << endl;
        cout << "-------------END OF GENERATION----------" << endl;

		archive.end_of_gen_steady(pop);
		//archive.add_randomly(pop);
		archive.evaluate_population(pop, false);

		cout << "ARCHIVE SIZE:" << archive.get_set_size() << endl;
		Broodwar << "ARCHIVE SIZE:" << archive.get_set_size() << endl;

		//Save best individuals in species for best mode
        archive.serialize_fittest(best_file.c_str());
//		save_best_species(best_file);
	}

	//write out current generation and fittest individuals
	if (offspring_count % (NEAT::pop_size*NEAT::print_every) == 0)
	{
		cout << offspring_count << endl;
		char fname[100];

		sprintf(fname, "%sfittest_%d", output_dir, offspring_count / NEAT::print_every);
		archive.serialize_fittest(fname);

		sprintf(fname, "%srtgen_%d", output_dir, offspring_count / NEAT::print_every);
		pop->print_to_file_by_species(fname);
	}


	//Now create offspring one at a time, testing each offspring,                                                               
	// and replacing the worst with the new offspring if its better

		//Every pop_size reproductions, adjust the compat_thresh to better match the num_species_target
		//and reassign the population to new species                                              
	if (offspring_count % compat_adjust_frequency == 0) {
        //count++;
#ifdef DEBUG_BROOD
        cout << "Adjusting..." << endl;
#endif

        int num_species = pop->species.size();
        double compat_mod = 0.1;  //Modify compat thresh to control speciation

        // This tinkers with the compatibility threshold
        if (num_species < num_species_target) {
            NEAT::compat_threshold -= compat_mod;
        } else if (num_species > num_species_target)
            NEAT::compat_threshold += compat_mod;

        if (NEAT::compat_threshold < 0.3)
            NEAT::compat_threshold = 0.3;
#ifdef DEBUG_BROOD
        cout << "compat_thresh = " << NEAT::compat_threshold << endl;
#endif

        //Go through entire population, reassigning organisms to new species
        for (auto org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
            pop->reassign_species(*org);
        }

        //For printing only
#ifdef DEBUG_BROOD

        for (auto curspec = (pop->species).begin(); curspec != (pop->species).end(); curspec++) {
            cout << "Species " << (*curspec)->id << " size" << (*curspec)->organisms.size() << " average= " << (*curspec)->average_est << endl;
        }

        cout << "Pop size: " << pop->organisms.size() << endl;
#endif
    }

//    for (vector<Organism*>::iterator org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
//        if((*org)->noveltypoint && !(*org)->noveltypoint->added && (*org)->noveltypoint->genotype == (*org)->gnome){
//            cout << "ERROR (7)" << std::endl;
//        }
//    }

    if(!curorg->noveltypoint->data.empty() && !curorg->noveltypoint->data[0].empty() /*&& curorg->noveltypoint->data[0][0]*/) {

        Broodwar << "Finished ! Indiv fitness : " << curorg->noveltypoint->fitness <<
                 " novelty (" << curorg->noveltypoint->novelty << ") : "; //<<  << std::endl;

        for (auto item = curorg->noveltypoint->data[0].begin(); item != curorg->noveltypoint->data[0].end(); ++item) {
            Broodwar << *item << " ";
        }
        Broodwar << std::endl << "Build order : ";
        for (auto item = curorg->noveltypoint->data[1].begin(); item != curorg->noveltypoint->data[1].end(); ++item) {
            Broodwar << *item << " ";
        }
        Broodwar << std::endl;
    }
    else{
        Broodwar << "ERROR : No data !" << std::endl;
    }

    cout << IASUGameWrapper::indiv << " : print population to file (eval)" << std::endl;
	//print pop file and remove worst org
	//use memory map if possible, and save every x iterations
    if(!pop->print_to_mmap(popmap, false, true) || IASUGameWrapper::indiv % 15 == 0)
	    pop->print_to_file(current_popfile, false, true);

    cout << IASUGameWrapper::indiv << " : print archive to file (eval)" << std::endl;
    if(!archive.Serialize(archivemap.c_str(), true))
        archive.Serialize(archive_file.c_str());
}

void drawLine(Position pos, Position target, BWAPI::Color c) {
	//draw line to show order
#ifdef DEBUG_BROOD
	Broodwar->registerEvent([pos, target, c](Game*) {
		Broodwar->drawLine(BWAPI::CoordinateType::Enum::Map, pos.x, pos.y, target.x, target.y, c);
		Broodwar->drawCircle(BWAPI::CoordinateType::Enum::Map, pos.x, pos.y, 10, BWAPI::Color(255, 255, 255));
		Broodwar->drawCircle(BWAPI::CoordinateType::Enum::Map, target.x, target.y, 20, c);
	},   // action
		nullptr,    // condition
		Broodwar->getLatencyFrames() * 30);  // frames to run
#endif // DEBUG_BROOD
}

//transform map coordinates into a value between 0 and 1
double shrinkCoord(int x, int y) {
	return (x * Broodwar->mapWidth() * 32 + y) / (double)(Broodwar->mapHeight() * 32 * Broodwar->mapWidth() * 32);
}

//get back map coordinates from a value between 0 and 1
BWAPI::Position expandCoord(double c) {
	Position p;
	p.x = ((int)(c * Broodwar->mapHeight() * 32 * Broodwar->mapWidth() * 32)) / (Broodwar->mapWidth() * 32);
	p.y = ((int)(c * Broodwar->mapHeight() * 32 * Broodwar->mapWidth() * 32)) % (Broodwar->mapWidth() * 32);
	return p;
}

//create neural net inputs from sensors
void generate_neural_inputs(double* inputs)
{
	int i;
	//bias
	inputs[0] = (1.0);
	for (i = 1; i < MAX_INPUTS; i++) {
		inputs[i] = 0;//rand()/(double)RAND_MAX;//test
	}
	i = 1;

	int w = 0,/* p = 0, a = 0, eb = 0,*/ eu = 0, m = 0, n = 0, c = 0,
		/*p_offset = 6 * 400, a_offset = p_offset + 6 * 400,
		eb_offset = a_offset + 6 * 400,*/ eu_offset = 4 * 400; // enemy buildings and units
	for (auto &u : Broodwar->self()->getUnits())
	{
		// Ignore the unit if it no longer exists
		// Make sure to include this block when handling any Unit pointer!
		if (!u->exists()){
			//remove unit from realToCustomID
			if(realToCustomID.find(u->getID()) != realToCustomID.end()) {
				customToRealID.erase(realToCustomID[u->getID()]);
				realToCustomID.erase(u->getID());
			}
			continue;
		}

		if(realToCustomID.find(u->getID()) == realToCustomID.end()){
			if(realToCustomID.size() < 400){
				for (int id = 0; id < 400; id++) {
					if(customToRealID.find(id) == customToRealID.end()) {
						realToCustomID.insert(std::make_pair(u->getID(), id));
						customToRealID.insert(std::make_pair(id, u->getID()));
						break;
					}
				}
			}
			else{
				Broodwar << "Fixme : unit count exceeds 400 !" << std::endl;
				//put building that can perform action first in the list if it is saturated
			}
		}


		int status = 0;		//0 = idle
		// the unit may have one of the following status
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised()) status = 1;
		else if (u->isLoaded() || !u->isPowered() || u->isStuck()) status = 2;
		else if (u->isMorphing() || !u->isCompleted() || u->isConstructing() || u->isTraining()) status = 3;
		else if (u->isGatheringGas() || u->isGatheringMinerals() || u->isRepairing()) status = 4;
		else if (u->isAttackFrame() || u->isStartingAttack()) status = 5;
		else if (u->isBurrowed()) status = 6;
		else if (u->isHallucination()) status = 7;
		else if (u->isMoving() || u->isAttacking()) status = 8;
		else if (u->isSieged()) status = 9;
		if ((u->getHitPoints() + u->getShields()) /
			(double)(u->getType().maxHitPoints() + u->getType().maxShields()) < 0.6)
			status += 10; // combine HP with status, not optimal but saves input
		if ((u->getHitPoints() + u->getShields()) /
			(double)(u->getType().maxHitPoints() + u->getType().maxShields()) < 0.3)
			status += 10;

		//if (u->getType().isWorker()) {		//workers

		inputs[i + w++] = realToCustomID[u->getID()] / 400.0;	//id
		inputs[i + w++] = u->getType().getID() / 500.0;	//type
		inputs[i + w++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
		inputs[i + w++] = status / 30.0;

		//}
		/*
		//larvas and producer buildings
		else if (u->getType().canProduce() && !u->getType().canAttack() || u->getType().isBuilding()) {
			inputs[i + p_offset + p++] = u->getID() / 5000.0;	//id
			inputs[i + p_offset + p++] = u->getType().getID() / 500.0;	//type
			inputs[i + p_offset + p++] = u->getHitPoints() + u->getShields()
				/ u->getType().maxHitPoints() + u->getType().maxShields();	//HP
			inputs[i + p_offset + p++] = u->getPosition().x / (double)(Broodwar->mapWidth() * 32);	//position
			inputs[i + p_offset + p++] = u->getPosition().y / (double)(Broodwar->mapHeight() * 32);
			inputs[i + p_offset + p++] = status / 9.0;
		}
		else {	//other units
			inputs[i + a_offset + a++] = u->getID() / 5000.0;	//id
			inputs[i + a_offset + a++] = u->getType().getID() / 500.0;	//type
			inputs[i + a_offset + a++] = u->getHitPoints() + u->getShields()
				/ u->getType().maxHitPoints() + u->getType().maxShields();	//HP
			inputs[i + a_offset + a++] = u->getPosition().x / (double)(Broodwar->mapWidth() * 32);	//position
			inputs[i + a_offset + a++] = u->getPosition().y / (double)(Broodwar->mapHeight() * 32);
			inputs[i + a_offset + a++] = status / 9.0;
		}

		*/
		if (w >= 400 * 4) break; // not enough space on inputs, skip the rest

	}

	for (auto &u : Broodwar->enemy()->getUnits())
	{
		if (!u->exists())
			continue;

		//currently unused
		//TODO maybe ? remove useless information above and add a visible tag / lastseen instead

		int status = 0;		//0 = idle
		// the unit may have one of the following status
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised()) status = 1;
		else if (u->isLoaded() || !u->isPowered() || u->isStuck()) status = 2;
		else if (u->isMorphing() || !u->isCompleted() || u->isConstructing() || u->isTraining()) status = 3;
		else if (u->isGatheringGas() || u->isGatheringMinerals() || u->isRepairing()) status = 4;
		else if (u->isAttackFrame() || u->isStartingAttack()) status = 5;
		else if (u->isBurrowed()) status = 6;
		else if (u->isHallucination()) status = 7;
		else if (u->isMoving() || u->isAttacking()) status = 8;
		else if (u->isSieged()) status = 9;
		if ((u->getHitPoints() + u->getShields()) /
			(double)(u->getType().maxHitPoints() + u->getType().maxShields()) < 0.6)
			status += 10; // combine HP with status, not optimal but saves input
		if ((u->getHitPoints() + u->getShields()) /
			(double)(u->getType().maxHitPoints() + u->getType().maxShields()) < 0.3)
			status += 10;

		//if (u->getType().isBuilding()) {
			//inputs[i + eb_offset + eb++] = u->getID() / 5000.0;	//id
			//inputs[i + eb_offset + eb++] = u->getType().getID() / 500.0;	//type
			//inputs[i + eb_offset + eb++] = u->getHitPoints() + u->getShields()
			//	/ u->getType().maxHitPoints() + u->getType().maxShields();	//HP
			//inputs[i + eb_offset + eb++] = u->getPosition().x / (double)(Broodwar->mapWidth() * 32);	//position
			//inputs[i + eb_offset + eb++] = u->getPosition().y / (double)(Broodwar->mapHeight() * 32);
			//inputs[i + eb_offset + eb++] = status / 9.0;
			////inputs[i + eb_offset + eb++] = 1; //lastseen (currently unused)

		inputs[i + eu_offset + eu++] = u->getID() / 5000.0;	//id
		inputs[i + eu_offset + eu++] = u->getType().getID() / 500.0;	//type
		inputs[i + eu_offset + eu++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
		//inputs[i + eu_offset + eu++] = status / 30.0;

		//}
		//else {
		//	inputs[i + eu_offset + eu++] = u->getID() / 5000.0;	//id
		//	inputs[i + eu_offset + eu++] = u->getType().getID() / 500.0;	//type
		//	inputs[i + eu_offset + eu++] = u->getHitPoints() + u->getShields()
		//		/ u->getType().maxHitPoints() + u->getType().maxShields();	//HP
		//	inputs[i + eu_offset + eu++] = u->getPosition().x / (double)(Broodwar->mapWidth() * 32);	//position
		//	inputs[i + eu_offset + eu++] = u->getPosition().y / (double)(Broodwar->mapHeight() * 32);
		//	inputs[i + eu_offset + eu++] = status / 9.0;
		//	inputs[i + eu_offset + eu++] = 1; //lastseen (currently unused)
		//}

		if (eu >= 400 * 3) break; // not enough space on inputs, skip the rest
	}
	i = eu_offset + 3 * 400;

	//set map nodes
	for (auto &u : Broodwar->getMinerals()) {
		if (!u->exists())
			continue;
		if (u->getResources() < u->getInitialResources() / 5) {
			inputs[i + m++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
			inputs[i + m++] = 2 / 4.0;
		}
		else {
			inputs[i + m++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
			inputs[i + m++] = 1 / 4.0;
		}
		if (m >= 300 * 2) break;
	}
	for (auto &u : Broodwar->getGeysers()) {
		if (m >= 300 * 2) break;
		if (!u->exists())
			continue;
		if (u->getResources() < u->getInitialResources() / 5) {
			inputs[i + m++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
			inputs[i + m++] = 4 / 4.0;
		}
		else {
			inputs[i + m++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
			inputs[i + m++] = 3 / 4.0;
		}
	}

	int n_offset = 300 * 2;

	for (auto &u : Broodwar->getNeutralUnits()) {
		if (!u->exists())
			continue;
		inputs[i + n_offset + n++] = shrinkCoord(u->getPosition().x, u->getPosition().y);	//position
		if (u->getType().isBuilding()) {
			inputs[i + n_offset + n++] = 1 / 2.0;
		}
		else {
			inputs[i + n_offset + n++] = 2 / 2.0;
		}
		if (n >= 50 * 2) break;
	}

	//TODO put newest creep in the 10*2 next inputs

	i = i + n_offset + 50 * 2 + 10 * 2;

	/*
	for (int k = 0; k < Broodwar->mapHeight(); k++) {
		for (int j = 0; j < Broodwar->mapWidth(); j++) {
			if (inputs[i + k * MAX_MAP_SIZE + j] == 0) {
				int tile = 0;
				if (Broodwar->getRegionAt(k * 32, j * 32)->isHigherGround()) {
					if (Broodwar->hasCreep(k, j))
						tile = 1;
					else
						tile = 2;
				}
				else {
					if (Broodwar->hasCreep(k, j))
						tile = 3;
				}
				if (!Broodwar->getRegionAt(k * 32, j * 32)->isAccessible())
					tile = 10;
				inputs[i + k * MAX_MAP_SIZE + j] = tile / 10.0;
			}
		}
	}
	*/

	// reentrant nodes
	for (int r = 0; r < 50; r++) {
		inputs[i + r] = reentrant_nodes[r];
	}
	i += 50;

	//TODO add upgrades

	inputs[i++] = Broodwar->getFrameCount() / 90000.0;//time
	inputs[i++] = Broodwar->self()->minerals() / 10000.0;//resources
	if (Broodwar->self()->minerals() > 10000.0) inputs[i] = 1.0;
	inputs[i++] = Broodwar->self()->gas() / 10000.0;
	if (Broodwar->self()->gas() > 10000.0) inputs[i] = 1.0;
	inputs[i++] = Broodwar->self()->supplyUsed() / (double)Broodwar->self()->supplyTotal();//supply
	inputs[i++] = Broodwar->self()->getRace().getID() / 4.0;//races
	inputs[i++] = Broodwar->enemy()->getRace().getID() / 4.0;

	//Broodwar->drawTextScreen(10, 0, "Inputs : %d", i);
	//Broodwar->drawTextScreen(10, 20, "Map size : %d x %d", Broodwar->mapWidth(), Broodwar->mapHeight());
}

//transform neural net outputs into broodwar actions
void interpret_outputs(vector<NNode*> outputs)
{
	//Broodwar->drawTextScreen(200, 80, "Target: (%d, %d)", (int)((double)(Broodwar->mapWidth() * 32) * outputs.at(0)->get_active_out()), (int)((double)(Broodwar->mapHeight() * 32) * outputs.at(1)->get_active_out()));
	//Broodwar->drawTextScreen(200, 60, "Map: (%d, %d)", Broodwar->mapWidth() * 32, Broodwar->mapHeight() * 32);
	//Broodwar->drawCircle(BWAPI::CoordinateType::Enum::Map, (int)((Broodwar->mapWidth() * 32) * outputs.at(0)->get_active_out()), (int)((Broodwar->mapHeight() * 32) * outputs.at(1)->get_active_out()), 20, BWAPI::Color(255, 0, 0));

	std::map<int,int> movedUnits;

	BWAPI::Unit u;
	for (int i = 0; i < 50; i++) {
		int out = (int) (outputs[i * 3]->get_active_out() * 400);
		if(customToRealID.find(out) != customToRealID.end()) {
			u = Broodwar->getUnit(customToRealID[out]);
		}
		else continue;

		//if (!u || !u->exists() || u->getPlayer()->getID() != Broodwar->self()->getID()) {			//////////////TEST\\\\\\\\\\\\\\\|
		//	int r = (int)(Broodwar->self()->getUnits().size() * (rand() / (double)RAND_MAX));
		//	int i = 0;
		//	for (auto unit : Broodwar->self()->getUnits()) {
		//		if (i == r && unit->exists() && unit->isIdle()) {
		//			u = unit;
		//			break;
		//		}
		//		i++;
		//	}
		//}

		//if (u && u->exists() && u->getPlayer()->getID() == Broodwar->self()->getID() &&
			if (movedUnits.find(u->getID()) == movedUnits.end()) {

			movedUnits.insert(std::make_pair(u->getID(), 1));
			Position target = expandCoord(outputs[i * 3 + 1]->get_active_out());
			Position pos = u->getPosition();
			

			vector<int> available_actions;
			available_actions.push_back(0);//do nothing
			//available_actions.push_back(14);//cancel -- removed
			if (u->getType().canMove() && u->getType().toString() != "Zerg_Larva") {
				available_actions.push_back(1);
			}
			if (u->getType().canAttack() && u->canAttack() && u->canAttackUnit()) {
				available_actions.push_back(2);
			}
			if (u->getType().canBuildAddon()) {
			    int a = 3;
			    for(auto k : u->getType().buildsWhat()){
			        if(k.isAddon()) available_actions.push_back(a++);
			    }
			}
			if (u->getType().isBurrowable()) {
				available_actions.push_back(5);
			}
			if (u->getType().isCloakable()) {
				available_actions.push_back(6);
			}
			if (u->getType().isWorker()) {	//gather resources
				available_actions.push_back(7);
			}
			if (u->getType().isFlyingBuilding()) {
				available_actions.push_back(8);
			}
			if (u->canLoad() && (!u->getType().isFlyer() || u->getType().toString() != "Zerg_Overlord" || u->getType().toString() != "Terran_Transport")) {
				available_actions.push_back(9);	//TODO add for other transports
			}
			if (u->canRepair()) {
				available_actions.push_back(10);
			}
			if (u->canSetRallyPosition()) {
				available_actions.push_back(11);
			}
			if (u->canSiege() || u->canUnsiege()) {
				available_actions.push_back(12);
			}
			if (u->canUnload()) {
				available_actions.push_back(13);
			}
			//14 = cancel
			if (u->getType().isSpellcaster()) {
				for (int i = 0; i < u->getType().abilities().size(); i++)
					available_actions.push_back(100 + i);
			}
			if (u->canUpgrade()) {
				for (int i = 0; i < u->getType().upgradesWhat().size(); i++)
					available_actions.push_back(200 + i);
			}
			if (u->canTrain() || u->canBuild() || u->canMorph()) {
				for (int i = 0; i < u->getType().buildsWhat().size(); i++)
					available_actions.push_back(300 + i);
			}
			if (u->canResearch()) {
				for (int i = 0; i < u->getType().researchesWhat().size(); i++)
					available_actions.push_back(400 + i);
			}

			//int r = (int)(((rand() - 1) / (double)RAND_MAX) * available_actions.size()); //test
			int r = (int)(outputs[i * 3 + 2]->get_active_out() * available_actions.size());
			BWAPI::Unitset set;
            int b = 0;
			switch (available_actions[r])
			{
			case 0://do nothing, keep current order (needed to attack for example)
				break;
			case 1:
				if (u->getPosition() != target && u->getTargetPosition() != target) {
					u->move(target);
					drawLine(pos, target, Color(100, 100, 100));
				}
				drawLine(pos, target, Color(255, 255, 255));
				break;
			case 2:
				set = Broodwar->getUnitsInRadius(target, 15, IsEnemy || IsNeutral);
				if (set.empty()) {
					if(u->getPosition() != target && (u->getTargetPosition() != target || u->isAttacking()))
						u->attack(target);
				}
				else if(!u->isAttacking() || (u->getTarget() != *(set.begin()) && u->getOrderTarget() != *(set.begin())))
				    u->attack(*(set.begin()));

				drawLine(pos, target, Color(255, 0, 0));
				break;
			case 3:
                for(auto k : u->getType().buildsWhat()){
                    if(k.isAddon()){
                        u->buildAddon(k);
                        break;
                    }
                }
				break;
			case 4:
                for(auto k : u->getType().buildsWhat()){
                    if(k.isAddon()) {
                        b++;
                        if (b > 1) {
                            u->buildAddon(k);
                            break;
                        }
                    }
                }
				break;
			case 5:
				if (u->isBurrowed()) u->unburrow();
				else u->burrow();
				break;
			case 6:
				if (u->isCloaked()) u->decloak();
				else u->cloak();
				break;
			case 7:
                if (u->isCarryingGas() || u->isCarryingMinerals()) {
                    u->returnCargo();
                }
                else {
                    if(!u->isGatheringGas() && !u->isGatheringMinerals()) {
                        set = Broodwar->getUnitsInRadius(target, 15, IsMineralField || IsRefinery);
                        if (set.empty()) u->gather(u->getClosestUnit(IsMineralField || IsRefinery));
                        else u->gather(*(set.begin()));
                        drawLine(pos, target, Color(0, 0, 255));
                    }
                }
				break;
			case 8:
				if (u->isLifted())u->land(TilePosition(target));
				else u->lift();
				break;
			case 9:
				set = Broodwar->getUnitsInRadius(target, 15, IsOwned && !IsBuilding);
				if (set.empty()) u->load(u->getClosestUnit(IsOwned && !IsBuilding));
				else u->load(*(set.begin()));
				drawLine(pos, target, Color(255, 255, 0));
				break;
			case 10:
				set = Broodwar->getUnitsInRadius(target, 15, IsMechanical && IsAlly && HP_Percent < 100);
				if (set.empty()) u->repair(u->getClosestUnit(IsMechanical && IsAlly && HP_Percent < 100));
				else u->repair(*(set.begin()));
				break;
			case 11:
				u->setRallyPoint(target);
				drawLine(pos, target, Color(255, 255, 255));
				break;
			case 12:
				if (u->isSieged()) u->unsiege();
				else u->siege();
				break;
			case 13:
				u->unloadAll(target);
				drawLine(pos, target, Color(0, 0, 255));
				break;
			case 14:
				if(u->canCancelAddon())
					u->cancelAddon();
				if(u->canCancelConstruction())
					u->cancelConstruction();
				if(u->canCancelMorph())
					u->cancelMorph();
				if(u->canCancelResearch())
					u->cancelResearch();
				if(u->canCancelTrain())
					u->cancelTrain();
				if(u->canCancelUpgrade())
					u->cancelUpgrade();
				if(u->canStop())
					u->stop();
				break;
			default:
				if (available_actions[r] >= 100) {
					if (available_actions[r] < 200) {//spell
						int a = available_actions[r] - 100, i = 0;
						SetContainer<TechType> actions = u->getType().abilities();
						for (auto type : actions) {
							if (i == a) {
								if (type.targetsPosition()) {
									u->useTech(type, target);
									drawLine(pos, target, Color(255, 0, 255));
								}
								else if (type.targetsUnit()) {
									set = Broodwar->getUnitsInRadius(target, 15);
									if (set.empty()) u->useTech(type, u->getClosestUnit());
									else u->useTech(type, *(set.begin()));
									drawLine(pos, target, Color(255, 0, 255));
								}
								else
									u->useTech(type);
								break;
							}
							i++;
						}

					}
					else if (available_actions[r] < 300) {//upgrade
						int a = available_actions[r] - 200, i = 0;
						SetContainer<UpgradeType> actions = u->getType().upgradesWhat();
						for (auto type : actions) {
							if (i == a) {
								u->upgrade(type);
								break;
							}
							i++;
						}
					}
					else if (available_actions[r] < 400) {//build
						int a = available_actions[r] - 300, i = 0;
						SetContainer<UnitType> actions = u->getType().buildsWhat();
						if (u->canTrain() || u->canMorph()) {
							for (auto type : actions) {
								if (i == a) {
									u->train(type);
									break;
								}
								i++;
							}
						}
						else if (u->canBuild()) {
							int a = available_actions[r] - 300, i = 0;
							for (auto type : actions) {
								if (i == a) {
									if (!u->canBuild(type, TilePosition(target))) {
										TilePosition targetBuildLocation = Broodwar->getBuildLocation(type, TilePosition(target), 99999);
										if (targetBuildLocation)
										{
											u->build(type, targetBuildLocation);
											drawLine(pos, Position(targetBuildLocation), Color(0, 255, 150));
										}
										else {
											//u->move(target);
										}
									}
									else {
										u->build(type, TilePosition(target));
										drawLine(pos, target, Color(0, 255, 0));
									}
									break;
								}
								i++;
							}
						}
					}
					else if (available_actions[r] < 500) {//research
						int a = available_actions[r] - 400, i = 0;
						SetContainer<TechType> actions = u->getType().researchesWhat();
						for (auto type : actions) {
							if (i == a) {
								u->research(type);
								break;
							}
							i++;
						}
					}
				}
				break;
			}

			if (u->isIdle() && u->getPosition() != target && u->getTargetPosition() != target) {
				u->move(target);
				drawLine(pos, target, Color(200, 200, 200));
			}
		}
	}

	for (int i = 150; i < 200; i++) {
		reentrant_nodes[i - 150] = outputs[i]->get_active_out();
	}

	/*
	if (isnan(outputs.at(0)->get_active_out()) || isnan(outputs.at(1)->get_active_out()))
		Broodwar << "OUTPUT ISNAN" << endl;

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

		// If the unit is a worker unit
		if (u->getType().isWorker())
		{
			u->attack(BWAPI::Position((int)((Broodwar->mapWidth() * 32) * outputs.at(0)->get_active_out()), (int)((Broodwar->mapHeight() * 32) * outputs.at(1)->get_active_out())));
			//Broodwar->drawTextScreen(200, 100, "Unit: (%d, %d)", u->getPosition().x, u->getPosition().y);
			break;
		}
	}
	*/
}


//perform a step and apply output to the game
void network_step()
{
#ifndef BEST_MODE
    //leave game if the bot does nothing
    if(Broodwar->getFrameCount() == 24*60*2 && build_order.empty()){
        Broodwar << "Bot is doing nothing. Aborting." << std::endl;
        Broodwar->leaveGame();
    }

    //leave game if the bot does not build anything
    if(Broodwar->getFrameCount() == 24*60*4){
        bool nonworker = false;
        for(auto id : build_order){
            UnitType t = UnitType(id);
            if(!t.isWorker()){
                nonworker = true;
                break;
            }
        }
        if(!nonworker) {
            Broodwar << "Bot is not building anything. Aborting." << std::endl;
            Broodwar->leaveGame();
        }
    }

    //leave game if the bot does not build units
    if(Broodwar->getFrameCount() >= 24*60*5){
        bool attacker = false;
        for(auto id : build_order){
            UnitType t = UnitType(id);
            if(!t.isWorker() && !t.isBuilding()){
                attacker = true;
                break;
            }
        }
        if(!attacker) {
            Broodwar << "Bot has no unit. Aborting." << std::endl;
            Broodwar->leaveGame();
        }
    }

    //leave game if no kill occurred in the first 10 minutes
    if(Broodwar->getFrameCount() >= 24*60*12 && IASUGameWrapper::killScore == 0){
        Broodwar << "No kill occurred in the first 12 minutes. Aborting." << std::endl;
        Broodwar->leaveGame();
    }

    //leave game if it lasts too long
    if(Broodwar->getFrameCount() >= 24*60*60){
        Broodwar << "Game is too long. Aborting." << std::endl;
        Broodwar->leaveGame();
    }
#endif

	Network *net = curorg->net;
	double inputs[MAX_INPUTS];


	generate_neural_inputs(inputs);
	net->load_sensors(inputs);
	net->activate();
	interpret_outputs(net->outputs);
}
