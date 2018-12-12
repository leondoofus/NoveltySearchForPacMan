#include "main.h"

#include <ctime>
#include <iostream>

using namespace NEAT;

static Population *pop;
static time_t startTime;

int main(int argc, char **argv) {

	//uncomment to generate input file
	//generate_genefile();

	//***********RANDOM SETUP***************//
	srand((unsigned)time(NULL));//  +getpid());

    /*
    //redirect cout to file
    std::ofstream out("write/out.txt", ios::app);
    std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf());
    time_t t = time(0);   // get time and print it
    std::tm* now = std::localtime(&t);
    std::cout << endl << (now->tm_year + 1900) << (now->tm_mon + 1) << now->tm_mday << '_' << now->tm_hour << '_' << now->tm_min << '_' << now->tm_sec << endl;



    //startTime = std::time(0);

    std::cout.rdbuf(coutbuf);

     */

    run_main_novelty("write/");

    return EXIT_SUCCESS;
}
