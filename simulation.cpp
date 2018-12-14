#include "simulation.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>


#include <CNCurses.h>
#include <CStateManager.h>
#include <CStateMenu.h>
#include <CStateGame.h>

// uncomment this to prevent learning and only play with one of the best versions so far
//#define BEST_MODE

//indicates whether we should use graphical mode
//#define USE_NCURSES

#define VERBOSE
#define MAX_INPUTS 8
#define MAX_OUTPUTS 2

#define NB_REENTRANT 2

static char output_dir[100] = "write/";
string genefile = "read/startgenes";
string settingsfile = "read/neatsettings.ne";
string current_genefile = "write/genefile";
string current_popfile = "write/popfile";
string indiv_file = "write/indiv";
string archive_file = "write/archive";
string best_file = "read/best";
string popmap = "write/popmap";
string archivemap = "write/archivemap";

static Population *pop;
//static int param = -1;
static bool isFirstGen = false;
static Organism *curorg;
static int offspring_count;
static int num_species_target;
static int compat_adjust_frequency;
data_record *newrec;

static int generation = 0;
static int indiv = 1;

static double reentrant_nodes[NB_REENTRANT];
static std::vector<float> build_order;

static float archive_thresh = 6.0; //initial novelty threshold

//archive of novel behaviors
static noveltyarchive archive(archive_thresh, *novelty_metric, false);

//main program
void run_main_novelty(const char *outputdir) {
    if (outputdir != NULL) strcpy(output_dir, outputdir);
    //neat init
    NEAT::load_neat_params(settingsfile.c_str(), false);

    pop = init_novelty_realtime();

    novelty_loop();

}

//main novelty training loop
void novelty_loop() {
    //TODO loop



    //run game

    int score = 0;
#ifdef USE_NCURSES
    CNCurses::init();
#endif
    CStateManager states;
    try {
        states.run(new CStateGame("default", CGameDifficulty::MEDIUM));
#ifdef USE_NCURSES
        CNCurses::exit();
#endif

    }
    catch (CStateManagerQuitExeptionReturnScore &e) {
#ifdef USE_NCURSES
        CNCurses::exit();
#endif
        score = e.getScore();
    }


    eval_one();

    //TODO generate next indiv

}

//novelty metric to evaluate individual
float novelty_metric(noveltyitem *x, noveltyitem *y) {
    return 0;   //TODO
    float diff = 0.0;
    //for (int k = 0; k < (int)x->data.size() && k < (int)y->data.size(); k++)
    //{
    int k = 0;
    if (x->data[k].size() != 0) {
        if (y->data[k].size() == 0) {
            diff += x->data[k].size() * 50;
            //continue;
        }
        diff += hist_diff(x->data[k], y->data[k]);
    }
    k = 1;
    if (x->data[k].size() != 0) {
        for (int l = 0; l < (int) x->data[k].size() && l < (int) y->data[k].size(); l++) {
            if (x->data[k][l] != y->data[k][l]) diff += 50;
        }
        if (x->data[k].size() > y->data[k].size())
            diff += x->data[k].size() - y->data[k].size() * 70; //reward additional units
    }
    //}
    if (x->data.size() > y->data.size())
        diff += 200 * (x->data.size() - y->data.size());    //reward additional vector
    return diff;
}

//evaluates an individual after an entire simulation and stores the novelty point
//a novelty point is a 2D array with relevant information
noveltyitem *eval_novelty(Organism *org, data_record *record) {
    noveltyitem *new_item = new noveltyitem;
    new_item->genotype = new Genome(*(org->gnome));
    new_item->phenotype = new Network(*(org->net));
    //vector to caracterize novelty
    vector<vector<float> > gather;

    vector<float> *scores = new vector<float>();
    gather.push_back(*scores);
    gather.push_back(build_order);
    gather[0].push_back(42);//...

    double fitness;
    static float highest_fitness = 0.0;

    fitness = 0; //TODO

    if (fitness > highest_fitness)
        highest_fitness = (float) fitness;

    //keep track of highest fitness so far in record
    if (record != NULL) {
        /*
    record->ToRec[19]=org->gnome->parent1;
    record->ToRec[18]=org->gnome->parent2;
    record->ToRec[17]=org->gnome->struct_change;
        */
        record->ToRec[RECSIZE - 1] = highest_fitness;
    }

    //push back novelty characterization
    new_item->data.push_back(gather[0]);
    //new_item->data.push_back(gather[1]);
    //set fitness (this is 'real' objective-based fitness, not novelty)
    new_item->fitness = (float) fitness;
    return new_item;
}

//write current indiv number to file
void write_indiv_number() {
    ofstream ofs(indiv_file, ios::out);
    ofs << "generation " << generation << std::endl << "indiv " << (indiv + 1) << std::endl;
    ofs.close();
}

//get indiv to test and current generation
void read_indiv_number() {
    char curword[128];  //max word size of 128 characters
    char curline[1024]; //max line size of 1024 characters
    ifstream ifs(indiv_file, ios::in);
    if (ifs) {
        while (!ifs.eof()) {
            ifs.getline(curline, sizeof(curline));
            std::stringstream ss(curline);
            //strcpy(curword, NEAT::getUnit(curline, 0, delimiters));
            ss >> curword;
            if (ifs.eof()) break;
            //std::cout << curline << std::endl;

            //Check for next
            if (strcmp(curword, "generation") == 0) {
                ss >> generation;
            } else if (strcmp(curword, "indiv") == 0) {
                ss >> indiv;
            }
        }
    }
    ifs.close();

    if (indiv > 1 && indiv % NEAT::pop_size == 1) {
        generation = generation + 1;
        //indiv = 0;
    }

    write_indiv_number();

    std::cout << "Indiv # : " << indiv << ", generation " << generation << std::endl;
}

//novelty initialization
//call this when program starts
Population *init_novelty_realtime() {
    Genome *start_genome;
    //genome
    char curword[20];
    int id;

#ifdef BEST_MODE
    std::srand((unsigned)time(NULL));
    pop = new Population(best_file.c_str());
    int rn = std::rand();
    int r = (int)(rn % pop->organisms.size());
    curorg = pop->organisms[r];
    std::cout << "Best mode : picked indiv #" << curorg->gnome->genome_id <<
        " (fitness " << curorg->noveltypoint->fitness << ", novelty " << curorg->noveltypoint->novelty << ")" << std::endl;
    return pop;
#endif


    //starter genes file
    ifstream iFile(current_genefile, ios::in);
    if (!iFile.is_open()) {
        std::cout << "No startgene found in write folder. Copying from read folder." << endl;
        //get current generation from file
        cout << "(current generation file) " << current_genefile << endl;
        iFile.close();
        isFirstGen = true;

        //no file generated yet, getting file from read folder
        cout << "(startgenes) " << genefile << endl;

        iFile.open(genefile, ios::in);
        if (!iFile.is_open()) {
            std::cout << "ERROR : CANNOT OPEN INPUT FILE !!!" << endl;
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
    } else {
        iFile.close();

        // get population from memory map or file
        if (!pop) {
            read_indiv_number();
            cout << indiv << " : read pop (init)" << std::endl;
//            pop = new Population(popmap.c_str(), true);
//            if (!pop || pop->organisms.size() == 0) {
//                cout << "No mmap found for pop ! Loading from file." << endl;
                pop = new Population(current_popfile.c_str(), false);
//            }
        }
    }

#ifdef VERBOSE
    //std::cout << "Verifying Pop..." << endl;
    //if (pop->verify()) {
    //	std::cout << "Done." << endl;
    /*}
    else {
        std::cout << "WARNING ! Verification returned error !" << endl;
    }*/
#endif

    if (!pop || pop->organisms.size() == 0) {
        cout << "ERROR EMPTY POPULATION !" << endl;
        std::cout << "ERROR EMPTY POPULATION !" << endl;
        exit(EXIT_FAILURE);
    }

    cout << indiv << " : read id (init)" << std::endl;
    if (generation == 0) isFirstGen = true;

//	if(offspring_count > 0 ) indiv = offspring_count;

    //get our indiv from pop
    for (vector<Organism *>::iterator org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
        if ((*org)->gnome->genome_id == indiv) {
            if (isFirstGen) {
                curorg = (*org);
            }
            else {
                //prevent erasing previous entry
                read_indiv_number();
            }
        }
    }


    offspring_count = indiv;

    //We try to keep the number of species constant at this number
    num_species_target = NEAT::pop_size / 15;

    //This is where we determine the frequency of compatibility threshold adjustment
    compat_adjust_frequency = NEAT::pop_size / 20;
    if (compat_adjust_frequency < 1)
        compat_adjust_frequency = 1;

    //generate indiv if it does not exist
    if (curorg == nullptr) {
        cout << indiv << " : generate organism (init)" << std::endl;

        //Here we call two rtNEAT calls:
        //1) choose_parent_species() decides which species should produce the next offspring
        //2) reproduce_one(...) creates a single offspring from the chosen species
        curorg = (pop->choose_parent_species())->reproduce_one(offspring_count, pop, pop->species);

//        cout << indiv << " : no org to load, generating new one (init)" << std::endl;
//		offspring_count--;
//		eval_one();
//		init_novelty_realtime(outputdir);
//		offspring_count = indiv;
    }

    //shouldn't happen
    if ((curorg->gnome) == nullptr) {
        cout << "ERROR EMPTY GENOME!" << endl;
        exit(1);
    }

    for (double &reentrant_node : reentrant_nodes) reentrant_node = 0;

    cout << indiv << " : load archive (init)" << std::endl;
    //archive = noveltyarchive(archive_thresh, *novelty_metric, archivemap, true);
    //if (archive.get_set_size() == 0)
    archive = noveltyarchive(archive_thresh, *novelty_metric, archive_file, false);

    cout << indiv << " : init done" << std::endl;
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
    vector<Organism *>::iterator org = (pop->organisms).begin();
    while (org != pop->organisms.end()) {
        //(*org)->noveltypoint = archive.get_item_from_id((*org)->gnome->genome_id);
        if (!(*org)->noveltypoint) {
            org = pop->organisms.erase(org);
        } else {
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
    int indiv_counter = offspring_count;// +NEAT::pop_size * generation;

    curorg->noveltypoint = eval_novelty(curorg);
    curorg->noveltypoint->indiv_number = indiv_counter;
    curorg->noveltypoint->genotype = curorg->gnome;

    //end first gen at pop_size
    if (offspring_count % NEAT::pop_size == 0) {
        first_gen_end();
    }
    //reload and save pop
    pop->print_to_file(current_popfile, false);
    //archive.Serialize(archive_file.c_str());
}

// evaluate individual at the end of the game
// this happens only AFTER the first generation
void eval_one() {

#ifdef BEST_MODE
    return;
#endif

    if (isFirstGen || indiv == 0) {
        first_gen_eval_one();
        if (offspring_count % (NEAT::pop_size * 1) != 0)
            return;
    }

//    cout << indiv << " : reload population (eval)" << std::endl;
//    //reload pop
//    if (!pop->reload(popmap.c_str(), true, true))
//        pop->reload(current_popfile.c_str(), true, false);

//    cout << indiv << " : remove not evaluated indiv (eval)" << std::endl;
//    //remove indiv if it was not evaluated
//    vector<Organism *>::iterator org = (pop->organisms).begin();
//    while (org != pop->organisms.end()) {
//        if (!(*org)->noveltypoint && (!curorg || curorg->gnome->genome_id != (*org)->gnome->genome_id)) {
//            org = pop->organisms.erase(org);
//        } else {
//            ++org;
//        }
//    }

    //Now we evaluate the new individual
    //Note that in a true real-time simulation, evaluation would be happening to all individuals at all times.
    //That is, this call would not appear here in a true online simulation.

    data_record *newrec = new data_record();
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

    //TODO
    /*
    //write out the first individual to beat opponent
    if (!firstflag && newrec->ToRec[3] > 0.0) {//TODO change record format
        firstflag = true;
        char filename[30];
        sprintf(filename, "%srtgen_first", output_dir);
        pop->print_to_file_by_species(filename);
        cout << "Maze solved by indiv# " << newrec->indiv_number << endl;
    }

    if(isVictorious) {
        cout << indiv << " : Victory !!" << std::endl;
        curorg->winner = true;
        curorg->print_to_file(const_cast<char *>("write/victory"));
    }
    */

    //update fittest individual list
    archive.update_fittest(pop);
    //refresh generation's novelty scores
    archive.evaluate_population(pop, true);

    //end of generation
    if (offspring_count % (NEAT::pop_size * 1) == 0) {

        archive.end_of_gen_steady(pop);
        //archive.add_randomly(pop);
        archive.evaluate_population(pop, false);

        std::cout << "-------------END OF GENERATION----------" << endl;
        std::cout << "ARCHIVE SIZE:" << archive.get_set_size() << endl;

        //Save best individuals in species for best mode
        archive.serialize_fittest(best_file.c_str());
    }

    //write out current generation and fittest individuals
    if (offspring_count % (NEAT::pop_size * NEAT::print_every) == 0) {
        cout << offspring_count << endl;
        char fname[100];

        sprintf(fname, "%sfittest_%d", output_dir, offspring_count / NEAT::print_every);
        archive.serialize_fittest(fname);

        sprintf(fname, "%srtgen_%d", output_dir, offspring_count / NEAT::print_every);
        pop->print_to_file_by_species(fname);

        cout << indiv << " : print population to file (eval)" << std::endl;
        //print pop file and remove worst org
        //use memory map if possible, and save every x iterations
        if (!pop->print_to_mmap(popmap, false, true) || indiv % 15 == 0)
            pop->print_to_file(current_popfile, false, true);

        cout << indiv << " : print archive to file (eval)" << std::endl;
        if (!archive.Serialize(archivemap.c_str(), true))
            archive.Serialize(archive_file.c_str());
    }


    //Now create offspring one at a time, testing each offspring,
    // and replacing the worst with the new offspring if its better

    //Every pop_size reproductions, adjust the compat_thresh to better match the num_species_target
    //and reassign the population to new species
    if (offspring_count % compat_adjust_frequency == 0) {
        //count++;
#ifdef VERBOSE
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
#ifdef VERBOSE
        cout << "compat_thresh = " << NEAT::compat_threshold << endl;
#endif

        //Go through entire population, reassigning organisms to new species
        for (auto org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
            pop->reassign_species(*org);
        }

        //For printing only
#ifdef VERBOSE

        for (auto curspec = (pop->species).begin(); curspec != (pop->species).end(); curspec++) {
            cout << "Species " << (*curspec)->id << " size" << (*curspec)->organisms.size() << " average= "
                 << (*curspec)->average_est << endl;
        }

        cout << "Pop size: " << pop->organisms.size() << endl;
#endif
    }

//    for (vector<Organism*>::iterator org = (pop->organisms).begin(); org != pop->organisms.end(); ++org) {
//        if((*org)->noveltypoint && !(*org)->noveltypoint->added && (*org)->noveltypoint->genotype == (*org)->gnome){
//            cout << "ERROR (7)" << std::endl;
//        }
//    }

//    if (!curorg->noveltypoint->data.empty() &&
//        !curorg->noveltypoint->data[0].empty() /*&& curorg->noveltypoint->data[0][0]*/) {
//
//        std::cout << "Finished ! Indiv fitness : " << curorg->noveltypoint->fitness <<
//                  " novelty (" << curorg->noveltypoint->novelty << ") : "; //<<  << std::endl;
//
//        for (auto item = curorg->noveltypoint->data[0].begin(); item != curorg->noveltypoint->data[0].end(); ++item) {
//            std::cout << *item << " ";
//        }
//        std::cout << std::endl << "Build order : ";
//        for (auto item = curorg->noveltypoint->data[1].begin(); item != curorg->noveltypoint->data[1].end(); ++item) {
//            std::cout << *item << " ";
//        }
//        std::cout << std::endl;
//    } else {
//        std::cout << "ERROR : No data !" << std::endl;
//    }

}

/*
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
*/


//create neural net inputs from sensors
void generate_neural_inputs(double *inputs) {
    int i;
    //bias
    inputs[0] = (1.0);
    for (i = 1; i < MAX_INPUTS; i++) {
        inputs[i] = 0;//rand()/(double)RAND_MAX;//test
    }
    i = 1;

    /*
    // reentrant nodes
    for (int r = 0; r < 50; r++) {
        inputs[i + r] = reentrant_nodes[r];
    }
    i += 50;
    */
}

//transform neural net outputs into broodwar actions
void interpret_outputs(vector<NNode *> outputs) {
    /*
    for (int i = 150; i < 200; i++) {
        reentrant_nodes[i - 150] = outputs[i]->get_active_out();
    }
    */
    /*
    if (isnan(outputs.at(0)->get_active_out()) || isnan(outputs.at(1)->get_active_out()))
        std::cout << "OUTPUT ISNAN" << endl;
    */
}


//perform a step and apply output to the game
void network_step() {
#ifndef BEST_MODE

#endif

    Network *net = curorg->net;
    double inputs[MAX_INPUTS];


    generate_neural_inputs(inputs);
    net->load_sensors(inputs);
    net->activate();
    interpret_outputs(net->outputs);
}
