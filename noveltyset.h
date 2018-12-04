#ifndef NVSET_H
#define NVSET_H

#include <math.h>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>

#include "population.h"
#include "sem.h"
#include "HsMmap.h"

#define ARCHIVE_SEED_AMOUNT 1

using namespace std;
using namespace NEAT;

//a novelty item is a "stake in the ground" i.e. a novel phenotype
class noveltyitem
{
private:


public:
	bool added = false;
	int indiv_number = 0;
	//we can keep track of genotype & phenotype of novel item
	NEAT::Genome* genotype;
	NEAT::Network* phenotype;

	//used to collect data
	vector< vector<float> > data;

	//future use
	float age = 0;

	//used for analysis purposes
	float novelty = 0;
	float fitness = 0;
	float generation = 0;

	//this will write a novelty item to file
	bool Serialize(ostream& ofile)
	{
		genotype->print_to_file(ofile);
		SerializeNoveltyPoint(ofile);
		return true;
	}

	//serialize the novelty point itself to file
	bool SerializeNoveltyPoint(ostream& ofile)
	{
		ofile << "noveltyitemstart" << endl <<
			"novelty " << novelty << endl <<
			"fitness " << fitness << endl <<
			"generation " << generation << endl <<
			"indiv " << indiv_number << endl;
		for (int i = 0; i < (int)data.size(); i++) {
			ofile << "point";
			for (int j = 0; j < (int)data[i].size(); j++) {
				ofile << " " << data[i][j];
			}
			ofile << endl;
		}
		ofile << "noveltyitemend" << endl;
		return true;
	}

	// get novelty item from file
	noveltyitem(istream& iFile) {

		added = false;
		genotype = NULL;
		phenotype = NULL;
		age = 0.0;
		generation = 0.0;
		indiv_number = (-1);
		data = vector< vector<float> >();

		char curword[1024];  //max word size of 128 characters
		char curline[8192]; //max line size of 1024 characters
		while (!iFile.eof())
		{
			iFile.getline(curline, sizeof(curline));
			std::stringstream ss(curline);
			ss >> curword;
			if (iFile.eof()) break;

			//Check for next
			if (strcmp(curword, "novelty") == 0)
			{
				ss >> novelty;
			}
			else if (strcmp(curword, "fitness") == 0)
			{
				ss >> fitness;
			}
			else if (strcmp(curword, "generation") == 0)
			{
				ss >> generation;
			}
			else if (strcmp(curword, "indiv") == 0)
			{
				ss >> indiv_number;
			}
			else if (strcmp(curword, "point") == 0)
			{
				//load noveltypoint
				data.push_back(vector<float>());
				float val;
				while (ss && !ss.eof()) {
					ss >> val;
					data[data.size() - 1].push_back(val);
				}
			}
			else if (strcmp(curword, "noveltyitemend") == 0)
			{
				break;
			}
		}
	}

	//copy constructor
	noveltyitem(const noveltyitem& item);

	//initialize...
	noveltyitem()
	{
		added = false;
		genotype = NULL;
		phenotype = NULL;
		age = 0.0;
		generation = 0.0;
		indiv_number = (-1);
	}

	~noveltyitem()
	{
		if (genotype)
			delete genotype;
		if (phenotype)
			delete phenotype;
	}


};

//different comparison functions used for sorting
bool cmp(const noveltyitem *a, const noveltyitem *b);
bool cmp_fit(const noveltyitem *a, const noveltyitem *b);


//the novelty archive contains all of the novel items we have encountered thus far
//Using a novelty metric we can determine how novel a new item is compared to everything
//currently in the novelty set 
class noveltyarchive
{

private:
	//are we collecting data?
	bool record;

	ofstream *datafile;
	ofstream *novelfile;
	typedef pair<float, noveltyitem*> sort_pair;
	//all the novel items we have found so far
	vector<noveltyitem*> novel_items;
	vector<noveltyitem*> fittest;

	//current generation novelty items
	vector<noveltyitem*> current_gen;

	//novel items waiting addition to the set pending the end of the generation 
	//vector<noveltyitem*> add_queue;
	int queue = 0;
	//the measure of novelty
	float(*novelty_metric)(noveltyitem*, noveltyitem*);
	//minimum threshold for a "novel item"
	float novelty_threshold;    //TODO add these to file
	float novelty_floor;
	//counter to keep track of how many gens since we've added to the archive
	int time_out;
	//parameter for how many neighbors to look at for N-nearest neighbor distance novelty
	int neighbors;
	//radius for SOG-type (not currently used)
	float radius;
	int this_gen_index;

	//hall of fame mode, add an item each generation regardless of threshold
	bool hall_of_fame;
	//add new items according to threshold
	bool threshold_add;

	//current generation
	int generation;
public:

	//constructor
	noveltyarchive(float threshold, float(*nm)(noveltyitem*, noveltyitem*), bool rec = true)
	{
		//how many nearest neighbors to consider for calculating novelty score?
		neighbors = 15;
		generation = 0;
		time_out = 0; //used for adaptive threshold
		novelty_threshold = threshold;
		novelty_metric = nm; //set the novelty metric via function pointer
		novelty_floor = 0.25; //lowest threshold is allowed to get
		record = rec;
		this_gen_index = ARCHIVE_SEED_AMOUNT;
		hall_of_fame = false;
		threshold_add = true;

		if (record)
		{
			datafile = new ofstream("runresults.dat");
		}
	}

	//constructor from file
	noveltyarchive(float threshold, float(*nm)(noveltyitem*, noveltyitem*), string filename, bool mmap = false)
	{
		Genome *gen;
		generation = 0;
		time_out = 0; //used for adaptive threshold
		novelty_threshold = threshold;

		//how many nearest neighbors to consider for calculating novelty score?
		neighbors = 15;
		novelty_metric = nm; //set the novelty metric via function pointer
		novelty_floor = 0.25; //lowest threshold is allowed to get
		record = false;
		this_gen_index = ARCHIVE_SEED_AMOUNT;
		hall_of_fame = false;
		threshold_add = true;

		//variables to keep : novelty_threshold, generation, time_out, queue
		//fittest ?, novel_items
		char curword[1024];  //max word size of 128 characters
		char curline[8192]; //max line size of 1024 characters


#ifndef _WIN32
        lockArchive();
#endif

        std::istream *input;
        if(mmap){
            //std::stringstream istr();
            input = new std::stringstream();

            //open file to get memory map location
            int* hFile = (int *)system_io_mmap_file_open(filename.c_str(), 3);
            if(!hFile){
                cout << "error : unable to open mmap file " << filename << std::endl;
                return;
            }


            //string s = str.str();
            //unsigned long size =  s.length() * sizeof(char);

            //get file size
            long long size = system_io_mmap_file_size(hFile);
//        if(system_io_mmap_extend_file_size(hFile, (long long)size) != 0){
//            cout << "error : unable to extend memory of mmap file " << filename << std::endl;
//        }

            //open memory map
            char* map = (char*)system_io_mmap_mmap(hFile, 3, 0, (size_t)size);
            if(!map || !strcmp(map,"")){
                cout << "error : unable to load mmap file " << filename << std::endl;
                if(map) system_io_mmap_munmap(&size, map);
                system_io_mmap_file_close(hFile);
#ifndef _WIN32
                releaseArchive();
#endif
                return;
            }
            //read from memory map
            (static_cast<std::stringstream&>(*input)) << map;

            system_io_mmap_munmap(&size, map);
            system_io_mmap_file_close(hFile);
        }
        else {
            input = new std::ifstream(filename);
            if (!*input) {
                printf("Can't open population file for input");
#ifndef _WIN32
                releaseArchive();
#endif
                return;
            }
        }

        bool best = false;
        //Loop until file is finished, parsing each line
        while (!(*input).eof())
        {
            (*input).getline(curline, sizeof(curline));
            if(curline[0] == '\0') continue;
            std::stringstream ss(curline);
            ss >> curword;
            if ((*input).eof()) break;

            //Check for next
            if (strcmp(curword, "params") == 0)
            {
                ss >> novelty_threshold >> generation >> time_out >> queue;
            }
            else if (strcmp(curword, "genomestart") == 0)
            {
                int id = 0;
                ss >> id;
                gen = new Genome(id, (*input));
            }
            else if (strcmp(curword, "noveltyitemstart") == 0)
            {
                noveltyitem *n = new noveltyitem((*input));
                gen->genome_id = n->indiv_number; // NOTE : value here may need to be changed !
                n->genotype = gen;
                if(!best) novel_items.push_back(n);
                else fittest.push_back(n);
            }
            else if (strcmp(curword, "bestitems") == 0)
            {
                best = true;
            }
        }

        if(!mmap)(static_cast<std::ifstream&>(*input)).close();

#ifndef _WIN32
        releaseArchive();
#endif
	}

	~noveltyarchive()
	{
		if (record)
		{
			datafile->close();
		}
		//probably want to delete all the noveltyitems at this point
	}

public:
	float get_threshold() { return novelty_threshold; }
	int get_set_size()
	{
		return (int)novel_items.size();
	}

	//add novel item to archive
	void add_novel_item(noveltyitem* item, bool aq = true)
	{
		item->added = true;
		item->generation = generation;
		novel_items.push_back(new noveltyitem(*item));
		if (aq)
			queue++;
			//add_queue.push_back(item);
	}

#define MIN_ACCEPTABLE_NOVELTY 0.005
	//not currently used
	void add_randomly(Population* pop)
	{
		for (int i = 0; i < (int)pop->organisms.size(); i++)
		{
			if (((float)rand() / RAND_MAX) < (0.0005))
			{
				noveltyitem* newitem = new noveltyitem(*pop->organisms[i]->noveltypoint);
				if (newitem->novelty > MIN_ACCEPTABLE_NOVELTY)
					add_novel_item(newitem, false);
				else delete newitem;
			}
		}
	}

	noveltyitem *get_item(int i) { return novel_items[i]; }
	noveltyitem *get_item_from_id(int i) { 
		for (vector<noveltyitem*>::iterator it = novel_items.begin(); it != novel_items.end(); ++it) {
			if ((*it)->indiv_number == i) {
				return (*it);
			}
		}
		return NULL;
	}

	//re-evaluate entire population for novelty
	void evaluate_population(Population* pop, bool fitness = true);
	//evaluate single individual for novelty
	void evaluate_individual(Organism* individual, Population* pop, bool fitness = true);

	//maintain list of fittest organisms so far
	void update_fittest(Organism* org)
	{
		int allowed_size = 5;
		if ((int)fittest.size() < allowed_size)
		{
			if (org->noveltypoint != NULL)
			{
				noveltyitem* x = new noveltyitem(*(org->noveltypoint));
				fittest.push_back(x);
				sort(fittest.begin(), fittest.end(), cmp_fit);
				reverse(fittest.begin(), fittest.end());
			}
			else
			{
				cout << "WHY NULL?" << endl;
			}
		}
		else
		{
			if (org->noveltypoint->fitness > fittest.back()->fitness)
			{
				noveltyitem* x = new noveltyitem(*(org->noveltypoint));
				fittest.push_back(x);

				sort(fittest.begin(), fittest.end(), cmp_fit);
				reverse(fittest.begin(), fittest.end());

				delete fittest.back();
				fittest.pop_back();

			}
		}
	}

	//resort fittest list
	void update_fittest(Population* pop)
	{

		sort(fittest.begin(), fittest.end(), cmp_fit);
		reverse(fittest.begin(), fittest.end());

	}

	//write out fittest list
	void serialize_fittest(const char *fn)
	{
		ofstream outfile(fn);
		for (int i = 0; i < (int)fittest.size(); i++)
			fittest[i]->Serialize(outfile);
		outfile.close();
	}

	//adjust dynamic novelty threshold depending on how many have been added to
	//archive recently
	void add_pending()
	{
		if (record)
		{
			(*datafile) << novelty_threshold << " " << queue << endl;
		}

		if (hall_of_fame)
		{
			if (queue >= 1) time_out++;
			else time_out = 0;
		}
		else
		{
			if (queue == 0)	time_out++;
			else time_out = 0;
		}

		//if no individuals have been added for 10 generations
		//lower threshold
		if (time_out >= 10) {
			novelty_threshold *= 0.95;
			if (novelty_threshold < novelty_floor)
				novelty_threshold = novelty_floor;
			time_out = 0;
		}

		//if more than four individuals added this generation
		//raise threshold
		if (queue > 4) novelty_threshold *= 1.2;

		queue = 0;

		this_gen_index = novel_items.size();
	}

	//criteria for adding to the archive
	bool add_to_novelty_archive(float novelty)
	{
		if (novelty > novelty_threshold)
			return true;
		else
			return false;
	}

	//only used in generational model (obselete)
	void end_of_gen()
	{
		generation++;


		if (threshold_add)
		{
			find_novel_items(true);
		}

		if (hall_of_fame)
		{
			find_novel_items(false);

			sort(current_gen.begin(), current_gen.end(), cmp);
			reverse(current_gen.begin(), current_gen.end());

			add_novel_item(current_gen[0]);
		}

		clean_gen();


		add_pending();
	}

	//steady-state end of generation call (every so many indivudals)
	void end_of_gen_steady(Population* pop)
	{

		generation++;

		add_pending();

		//vector<Organism*>::iterator cur_org;
	}

	void clean_gen()
	{
		vector<noveltyitem*>::iterator cur_item;

		bool datarecord = true;

		stringstream filename("");
		filename << "novrec/out" << generation << ".dat";
		ofstream outfile(filename.str().c_str());
		cout << filename.str() << endl;

		for (cur_item = current_gen.begin(); cur_item != current_gen.end(); cur_item++)
		{
			if (datarecord)
			{
				(*cur_item)->SerializeNoveltyPoint(outfile);
			}
			if (!(*cur_item)->added)
				delete (*cur_item);
		}
		current_gen.clear();
	}

	//see if there are any individuals in current generation
	//that need to be added to the archive (obselete)
	void find_novel_items(bool add = true)
	{
		vector<noveltyitem*>::iterator cur_item;
		for (cur_item = current_gen.begin(); cur_item != current_gen.end(); cur_item++)
		{
			float novelty = test_novelty((*cur_item));
			(*cur_item)->novelty = novelty;
			if (add && add_to_novelty_archive(novelty))
				add_novel_item(*cur_item);
		}
	}

	//add an item to current generation (obselete)
	void add_to_generation(noveltyitem* item)
	{
		current_gen.push_back(item);
	}

	//nearest neighbor novelty score calculation
	float novelty_avg_nn(noveltyitem* item, int neigh = -1, bool ageSmooth = false, Population* pop = NULL)
	{
		vector<sort_pair> novelties;
		if (pop)
			novelties = map_novelty_pop(novelty_metric, item, pop);
		else
			novelties = map_novelty(novelty_metric, item);
		sort(novelties.begin(), novelties.end());

		float density = 0.0;
		int len = novelties.size();
		float sum = 0.0;
		float weight = 0.0;


		if (neigh == -1)
		{
			neigh = neighbors;
		}

		if (len < ARCHIVE_SEED_AMOUNT)
		{
			item->age = 1.0;
			add_novel_item(item);
		}
		else
		{
			len = neigh;
			if ((int)novelties.size() < len)
				len = novelties.size();
			int i = 0;

			while (weight < neigh && i < (int)novelties.size())
			{
				float term = novelties[i].first;
				float w = 1.0;

				if (ageSmooth)
				{
					float age = (novelties[i].second)->age;
					w = 1.0 - pow((float)0.95, age);
				}

				sum += term * w;
				weight += w;
				i++;
			}

			if (weight != 0)
			{
				density = sum / weight;
			}
		}

		item->novelty = density;
		item->generation = generation;
		return density;
	}

	//fitness = avg distance to k-nn in novelty space
	float test_fitness(noveltyitem* item)
	{
		return novelty_avg_nn(item, -1, false);
	}

	float test_novelty(noveltyitem* item)
	{
		return novelty_avg_nn(item, 1, false);
	}

	//map the novelty metric across the archive
	vector<sort_pair> map_novelty(float(*nov_func)(noveltyitem*, noveltyitem*), noveltyitem* newitem)
	{
		vector<sort_pair> novelties;
		for (int i = 0; i < (int)novel_items.size(); i++)
		{
			novelties.push_back(make_pair((*novelty_metric)(novel_items[i], newitem), novel_items[i]));
		}
		return novelties;
	}

	//map the novelty metric across the archive + current population
	vector<sort_pair> map_novelty_pop(float(*nov_func)(noveltyitem*, noveltyitem*), noveltyitem* newitem, Population* pop)
	{
		vector<sort_pair> novelties;
		for (int i = 0; i < (int)novel_items.size(); i++)
		{
			novelties.push_back(make_pair((*novelty_metric)(novel_items[i], newitem), novel_items[i]));
		}
		for (int i = 0; i < (int)pop->organisms.size(); i++)
		{
			novelties.push_back(make_pair((*novelty_metric)(pop->organisms[i]->noveltypoint, newitem),
				pop->organisms[i]->noveltypoint));
		}
		return novelties;
	}

	//write out archive
	bool Serialize(const char* filename, bool mmap = false)
	{
		bool res = false;
        if(!mmap) {
#ifndef _WIN32
			lockArchive();
#endif
            ofstream outfile;
            outfile.open(filename);
            res = Serialize(outfile);
            outfile.close();
        }
        else {
        	std::stringstream str;
            res = Serialize(str);
#ifndef _WIN32
            lockArchive();
#endif
            //open file to get memory map location
            int *hFile = (int *) system_io_mmap_file_open(filename, 3);
            if (!hFile) {
                cout << "error : unable to open mmap file " << filename << std::endl;
#ifndef _WIN32
				releaseArchive();
#endif
                return false;
            }

            string s = str.str();
            unsigned long size = s.length() * sizeof(char);

            //extend file size
            if (system_io_mmap_extend_file_size(hFile, (long long) size) != 0) {
                cout << "error : unable to extend memory of mmap file " << filename << std::endl;
#ifndef _WIN32
				releaseArchive();
#endif
				return false;
            }

            //open memory map
            char *map = (char *) system_io_mmap_mmap(hFile, 3, 0, size);
            if (!map) {
                cout << "error : unable to load mmap file " << filename << std::endl;
                system_io_mmap_file_close(hFile);
#ifndef _WIN32
				releaseArchive();
#endif
                return false;
            }
            //write to memory map
            strcpy(map, s.c_str());

            system_io_mmap_munmap((long long int *) s.length(), map);
            system_io_mmap_file_close(hFile);
        }

#ifndef _WIN32
        releaseArchive();
#endif
		return res;
	}

	//write out archive
	bool Serialize(ostream& ofile)
	{
		ofile << "params " << novelty_threshold <<" "<< generation << " " << time_out << " " << queue << endl;
		for (auto &novel_item : novel_items) {
            novel_item->Serialize(ofile);
		}
        ofile << "bestitems" << std::endl;
        for (auto &novel_item : fittest) {
            novel_item->Serialize(ofile);
        }
		return true;
	}

};


#endif
