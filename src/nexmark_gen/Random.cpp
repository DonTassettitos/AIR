#include "Random.hpp"
#include <stdio.h> // NULL
#include <stdlib.h> // srand, rand
#include <time.h> // time
#include <mpi.h>

using nexmark_gen::Random;

bool Random::initialized = false;

int Random::getRandomInt(const int min, const int max)
{ //range : [min, max]
	if (!Random::initialized) {
		unsigned int seed = MPI_Wtime() * 1000000000; // different ranks must call different seeds, or else both instances will act the same
		srand(seed);
		initialized = true;
	}
	return min + rand() % ((max + 1) - min);
}