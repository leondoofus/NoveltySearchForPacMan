#pragma once
#ifndef G_WRAPPER_H
#define G_WRAPPER_H

//#define NULL 0
#define MAX_MAP_SIZE 128

#include <ctime>
#include <cstdlib>

#include "neat.h"
#include "noveltyset.h"
#include "histogram.h"
#include "datarec.h"

#include "HsMmap.h"

#include <BWAPI.h>

using namespace std;
using namespace BWAPI;
using namespace NEAT;

float novelty_metric(noveltyitem* x, noveltyitem* y);
noveltyitem* eval_novelty(Organism *org, data_record* record = NULL);

void addUnit(float id);

Population *init_novelty_realtime(const char* output_dir = NULL);
void eval_one();
void network_step();

class IASUGameWrapper {
public:
	static int indiv;
	static int generation;
	static int killScore;
	static int unitScore;
	static bool isVictorious;
};
/*

Population *fitness_realtime(char* output_dir = NULL, const char* mazefile = "maze.txt", int param = -1);
int fitness_realtime_loop(Population *pop);
*/

#endif
