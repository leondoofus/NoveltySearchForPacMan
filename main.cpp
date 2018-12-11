#include "main.h"

#include <ctime>
#include <iostream>

using namespace NEAT;

static Population *pop;
static time_t startTime;

//use this to create part of the genefile (only needed once)
void generate_genefile() {

	ofstream of("read/g", ios::out);

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

int main(int argc, char **argv) {

	//uncomment to generate input file
	//generate_genefile();

	//***********RANDOM SETUP***************//
	srand((unsigned)time(NULL));//  +getpid());

	//redirect cout to file
	std::ofstream out("write/out.txt", ios::app);
	std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
	std::cout.rdbuf(out.rdbuf());
	time_t t = time(0);   // get time and print it
	std::tm* now = std::localtime(&t);
	std::cout << endl << (now->tm_year + 1900) << (now->tm_mon + 1) << now->tm_mday << '_' << now->tm_hour << '_' << now->tm_min << '_' << now->tm_sec << endl;

	run_main_novelty("write/");

	//TODO MAIN LOOP

	startTime = std::time(0);

	std::cout.rdbuf(coutbuf);

	return EXIT_SUCCESS;
}
