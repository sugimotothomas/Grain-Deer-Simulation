// Thomas Sugimoto
// CS 475 - Parallel Programming
// 5/13/2019
// MyAgent - Predator Wolves
// Project_3.cpp

#include "pch.h"
#include <random>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
using namespace std;
# define M_PI           3.14159265358979323846  /* pi */

// ----  Global Variables  ----
int	NowYear;			// 2019 - 2024
int	NowMonth;			// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int		NowNumDeer;		// number of deer in the current population
int		NowNumWolves;	// number of wolves in the current population

omp_lock_t	Lock;
int		NumInThreadTeam;
int		NumAtBarrier;
int		NumGone;

// ----  Constant Values  ----
const float GRAIN_GROWS_PER_MONTH = 9.0;
const float ONE_DEER_EATS_PER_MONTH = 0.5;
const int	X_WOLVES_EAT_ONE_DEER_PER_MONTH = 1;

const float AVG_PRECIP_PER_MONTH = 6.0;		// average
const float AMP_PRECIP_PER_MONTH = 6.0;		// plus or minus
const float RANDOM_PRECIP = 2.0;			// plus or minus noise

const float AVG_TEMP = 50.0;				// average
const float AMP_TEMP = 20.0;				// plus or minus
const float RANDOM_TEMP = 10.0;				// plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

// ---- Function Prototypes  ----
void	InitBarrier(int);
void	WaitBarrier();
float	SQR(float);
void	GrainDeer();
void	Grain();
void	Watcher();
void	MyAgent();
int		intRand(const int &, const int &);
float	floatRand(const float &, const float &);

// ----   Main Program:  ----
int main(int argc, char *argv[]) {
	omp_init_lock(&Lock);
	omp_set_num_threads(4);	// same as # of sections
	InitBarrier(4);
	// starting date and time:
	NowMonth = 0;
	NowYear = 2019;

	// starting state (feel free to change this if you want):
	NowNumDeer = 5;
	NowHeight = 1.;
	NowTemp = AVG_TEMP;
	NowNumWolves = 1;

	#pragma omp parallel sections
	{
		#pragma omp section
		{
			GrainDeer();
		}

		#pragma omp section
		{
			Grain();
		}

		#pragma omp section
		{
			Watcher();
		}
		
		#pragma omp section
		{
			MyAgent();
		}
		
		
	}       // implied barrier -- all functions must return in order
			// to allow any of them to get past here
}


// ----  Functions  ----
// specify how many threads will be in the barrier:
//	(also init's the Lock)
void InitBarrier(int n) {
	NumInThreadTeam = n;
	NumAtBarrier = 0;
	omp_init_lock(&Lock);
}


// have the calling thread wait here until all the other threads catch up:
void WaitBarrier() {
	omp_set_lock(&Lock);
	{
		NumAtBarrier++;
		if (NumAtBarrier == NumInThreadTeam)
		{
			NumGone = 0;
			NumAtBarrier = 0;
			// let all other threads get back to what they were doing
			// before this one unlocks, knowing that they might immediately
			// call WaitBarrier( ) again:
			while (NumGone != NumInThreadTeam - 1);
			omp_unset_lock(&Lock);
			return;
		}
	}
	omp_unset_lock(&Lock);

	while (NumAtBarrier != 0);	// this waits for the nth thread to arrive

	#pragma omp atomic
		NumGone++;			// this flags how many threads have returned
}

// Squares a given float value
float SQR(float x) {
	return x * x;
}

// calculates the number of dear for the next year
void GrainDeer() {
	while (NowYear < 2025) {
		int NextNumDeer = NowNumDeer;
		NextNumDeer -= NowNumWolves / X_WOLVES_EAT_ONE_DEER_PER_MONTH;
		if (NowHeight < NextNumDeer) {
			NextNumDeer--;
		}
		else if (NowHeight > NextNumDeer) {
			NextNumDeer += 4;
		}
		if (NextNumDeer < 0)
			NextNumDeer = 0;
		WaitBarrier();		// Done Computing

		NowNumDeer = NextNumDeer;

		WaitBarrier();		// Done Assigning
		WaitBarrier();		// Done Printing
	}
}

// calculates the grain height for the next year
void Grain() {
	while (NowYear < 2025) {
		float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.f));
		float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.f));

		float NextHeight = NowHeight;
		NextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
		NextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
		if (NextHeight < 0) {
			NextHeight = 0;
		}

		WaitBarrier();		// Done Computing

		NowHeight = NextHeight;

		WaitBarrier();		// Done Assigning
		WaitBarrier();		// Done Printing
	}
}

// prints the data and calculates new enviroment parameters
void Watcher() {
	ofstream outputFile;
	outputFile.open("data.txt");
	outputFile << "Date\tTemp\tHeight\tPrecip\tNumDeer\tNumWolves\n";
	while (NowYear < 2025) {
		WaitBarrier();		// Done Computing
		WaitBarrier();		// Done Assigning

		// Print out all the data in console
		float NowTempC = (5.f / 9.f)*(NowTemp - 32.f);
		float NowHeightcm = NowHeight * 2.54f;
		float NowPrecipcm = NowPrecip * 2.54f;

		printf("----- Date: %d/%d", (NowMonth + 1), NowYear);
		printf("     Temp: %f -----\n", NowTemp);
		printf("Grain Height	: %0.2f\n", NowHeight);
		printf("Precipitation	: %0.2f\n", NowPrecip);
		printf("Number of Deer	: %d\n", NowNumDeer);
		printf("Number of Wolves: %d\n", NowNumWolves);

		// Print to text file for easy excel import
		if (NowMonth < 9)
			outputFile << "0";
		outputFile << (NowMonth + 1) << "/01/";
		outputFile << NowYear << "\t";
		outputFile << NowTempC << "\t";
		outputFile << NowHeightcm << "\t";
		outputFile << NowPrecipcm << "\t";
		outputFile << NowNumDeer << "\t";
		outputFile << NowNumWolves << "\n";

		// Calculate New Enviroment Parameters
		NowMonth++;
		if (NowMonth > 11) {
			NowMonth = 0;
			NowYear++;
		}
		float ang = (30.f*(float)NowMonth + 15.f) * ((float)M_PI / 180.f);
		float temp = AVG_TEMP - AMP_TEMP * cos(ang);
		//unsigned int seed = 0;
		NowTemp = temp + floatRand(-RANDOM_TEMP, RANDOM_TEMP);

		float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
		NowPrecip = precip + floatRand(-RANDOM_PRECIP, RANDOM_PRECIP);
		if (NowPrecip < 0.f)
			NowPrecip = 0.f;
		WaitBarrier();		// Done Printing
	}
	outputFile.close();
}

// wolves predator growth rate
void MyAgent() {
	while (NowYear < 2025) {
		
		int NextNumWolves = NowNumWolves;
		if (NowMonth % 3 == 0) {
			if (NowNumDeer > 5)
				NextNumWolves++;
			else {
				NextNumWolves--;
			}
		}
		if (NextNumWolves < 0)
			NextNumWolves = 0;
			
		WaitBarrier();		// Done Computing
		
		NowNumWolves = NextNumWolves;

		WaitBarrier();		// Done Assigning
		WaitBarrier();		// Done Printing
	}
}

//  random integer number generator
int intRand(const int & min, const int & max) {
	static thread_local std::mt19937 generator;
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(generator);
}

//	random float number generator
float floatRand(const float & min, const float & max) {
	static thread_local std::mt19937 generator;
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(generator);
}
