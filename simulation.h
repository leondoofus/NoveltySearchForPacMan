#pragma once
#ifndef G_WRAPPER_H
#define G_WRAPPER_H

//#define NULL 0

#include <ctime>

#include "neat.h"
#include "noveltyset.h"
#include "histogram.h"
#include "datarec.h"

using namespace std;
using namespace NEAT;

float novelty_metric(noveltyitem* x, noveltyitem* y);
noveltyitem* eval_novelty(Organism *org, data_record* record = NULL);

Population *init_novelty_realtime(const char* output_dir = NULL);
void eval_one();
void network_step();

/*

Population *fitness_realtime(char* output_dir = NULL, const char* mazefile = "maze.txt", int param = -1);
int fitness_realtime_loop(Population *pop);
*/

#endif
