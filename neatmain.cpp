
#include <iostream>
#include <vector>
#include <io.h>
#include <cstring>
#include "neat.h"
#include "population.h"
#include "experiments.h"

using namespace std;


int main(int argc, char **argv) {

  char mazename[100]="../../input_files/hard_maze.txt";
  char outputdir[100]="../../output_files/";
  char settingsname[100]="../../input_files/maze.ne";

  NEAT::Population *p;

  //***********RANDOM SETUP***************//
  /* Seed the random-number generator with current time so that
      the numbers will be different every time we run.    */
  srand((unsigned)time(NULL));//  +getpid());

  //Load in the params
  if(argc>1)
	strcpy_s(settingsname,argv[1]);

  NEAT::load_neat_params(settingsname,true);

  cout<<"loaded :"<<endl;

  //args : maze.ne maze.txt choice#(1=fitness, 2=novelty) param

  int choice=(-1);
	if(argc>4)
		choice=atoi(argv[4]);

  int param=(-1);
	if(argc>5)
		param=atoi(argv[5]);

  if (argc>3)
	strcpy_s(mazename,argv[3]);

  if (argc>2)
	strcpy_s(outputdir,argv[2]);

  cout << "(.ne) " << settingsname << "    (outputdir) " << outputdir << "    (.txt) " << mazename << endl;

  cout<<"Please choose an experiment: "<<endl;
  cout<<"1 - Maze Fitness Run" <<endl;
  cout<<"2 - Maze Novelty Run" <<endl;
  cout<<"Number: ";

if(choice==-1)
  cin>>choice;

  switch ( choice )
    {
    case 1:
      p = maze_fitness_realtime(outputdir,mazename,param);
      break;
    case 2:
      p = maze_novelty_realtime(outputdir,mazename,param);
      break;
    default:
      cout<<"Not an available option."<<endl;
    }


  return(0);

}
