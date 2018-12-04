#ifndef _SPECIES_H_
#define _SPECIES_H_

#include "neat.h"
#include "organism.h"
#include "population.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>

namespace NEAT {

	class Organism;
	class Population;

	// ---------------------------------------------  
	// SPECIES CLASS:
	//   A Species is a group of similar Organisms      
	//   Reproduction takes place mostly within a
	//   single species, so that compatible organisms
	//   can mate.                                      
	// ---------------------------------------------  
	class Species {

	public:

		int id = 0;
		int age = 0; //The age of the Species
		double ave_fitness = 0; //The average fitness of the Species
		double max_fitness = 0; //Max fitness of the Species
		double max_fitness_ever = 0; //The max it ever had
		int expected_offspring = 0;
		bool novel = false;
		bool checked = false;
		bool obliterate = false;  //Allows killing off in competitive coevolution stagnation
		std::vector<Organism*> organisms; //The organisms in the Species
		std::vector<int> organisms_id;
		//std::vector<Organism*> reproduction_pool;  //The organisms for reproduction- NOT NEEDED 
		int age_of_last_improvement = 0;  //If this is too long ago, the Species will goes extinct
		double average_est = 0; //When playing real-time allows estimating average fitness

		bool add_Organism(Organism *o);

		Organism *first();

		bool print_to_file(std::ostream &outFile);
		bool print_to_file(std::ofstream &outFile);

		//Change the fitness of all the organisms in the species to possibly depend slightly on the age of the species
		//and then divide it by the size of the species so that the organisms in the species "share" the fitness
		void adjust_fitness();

		double compute_average_fitness(); 

		double compute_max_fitness();

		//Counts the number of offspring expected from all its members skim is for keeping track of remaining 
		// fractional parts of offspring and distributing them among species
		double count_offspring(double skim);

		//Compute generations since last improvement
		int last_improved() {
			return age-age_of_last_improvement;
		}

		//Remove an organism from Species
		bool remove_org(Organism *org);

		double size() {
			return organisms.size();
		}

		Organism *get_champ();

		//Perform mating and mutation to form next generation
		bool reproduce(int generation, Population *pop,std::vector<Species*> &sorted_species);

		// *** Real-time methods *** 

		//Place organisms in this species in order by their fitness
		bool rank();

		//Compute an estimate of the average fitness of the species
		//The result is left in variable average_est and returned
		//New variable: average_est, NEAT::time_alive_minimum (const) 
		//Note: Initialization requires calling estimate_average() on all species
		//      Later it should be called only when a species changes 
		double estimate_average();

		//Like the usual reproduce() method except only one offspring is produced
		//Note that "generation" will be used to just count which offspring # this is over all evolution
		//Here is how to get sorted species:
		//    Sort the Species by max fitness (Use an extra list to do this)
		//    These need to use ORIGINAL fitness
		//      sorted_species.sort(order_species);
		Organism *reproduce_one(int generation, Population *pop,std::vector<Species*> &sorted_species);
//		Organism *reproduce_one(int generation, Population *pop,Vector<Species*> &sorted_species, bool addAdv, Genome* adv);

		Species(int i);

		//Allows the creation of a Species that won't age (a novel one)
		//This protects new Species from aging inside their first generation
		Species(int i,bool n);

		Species(std::istream& iFile);

		bool serialize(std::ostream& ofile);

		~Species();

	};

	// This is used for list sorting of Species by fitness of best organism highest fitness first 
	bool order_species(Species *x, Species *y);

	bool order_new_species(Species *x, Species *y);

}

#endif
