#pragma once
#ifndef G_WRAPPER_H
#define G_WRAPPER_H

//#define NULL 0
//#define USE_NCURSES 1

#include <ctime>

#include "neat.h"
#include "noveltyset.h"
#include "histogram.h"
#include "datarec.h"

using namespace std;
using namespace NEAT;

void run_main_novelty(const char* output_dir = NULL);
Population *init_novelty_realtime();
void novelty_loop();

float novelty_metric(noveltyitem* x, noveltyitem* y);
noveltyitem* eval_novelty(Organism *org, data_record* record = NULL);
void eval_one();
void network_step();
void final_print();

/*

Population *fitness_realtime(char* output_dir = NULL, const char* mazefile = "maze.txt", int param = -1);
int fitness_realtime_loop(Population *pop);
*/

#endif
