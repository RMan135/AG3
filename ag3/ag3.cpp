// ag3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <Windows.h>

#define LOG_CONSOLE_ENABLED					1
#define LOG_FILE_ENABLED					1
#define ESTIMATE_TIME_ITERATIONS			0
#define ESTIMATE_TIME_GENERATIONS			0
#define MINIMIZE							1

#define ITERATIONS							1
#define MUTATIONS_ENABLED					1

#define DEBUG_EVALUATION                    0
#define DEBUG_SELECTION                     1
#define DEBUG_MUTATIONS                     1

FILE * logfile;
FILE * evolutionfile;
FILE * resultsfile;
FILE * avgfitnessfile;


#define LOG(x, ...)													\
{																	\
	if(LOG_CONSOLE_ENABLED) printf(x, __VA_ARGS__);					\
	if(LOG_FILE_ENABLED)fprintf(logfile, x, __VA_ARGS__);			\
}

void DisplayMatrix(int ** matrix, int n)
{
	LOG(" =====  M A T R I X  =====\n\n");
	for (int i = 0; i < n; i++)
	{
		LOG("  ");
		for (int j = 0; j < n; j++)
		{
			LOG("% 4d ", matrix[i][j]);
		}
		LOG("\n\n");
	}
	LOG(" =========================\n");
}

int Initialize_AG(int & n, int & maxGenerations, int & popSize, double & mutationProbability, int ** &matrix)
{

	FILE * input;
	fopen_s(&input, "input.txt", "r");
	
	fscanf_s(input, "%d\n", &n);

	maxGenerations = 200;
	popSize = 30;
	mutationProbability = 0.1;

	int rows = n;
	int cols = n;
	matrix = new int*[rows];
	if (rows)
	{
		matrix[0] = new int[rows * cols];
		for (int i = 1; i < rows; i++)
			matrix[i] = matrix[0] + i * cols;
	}

	int node1, node2, val;
	while (-1 != fscanf_s(input, "%d %d %d\n", &node1, &node2, &val))
	{
		matrix[node1][node2] = matrix[node2][node1] = val;
	}

	for (int i = 0; i < n; i++)
	{
		matrix[i][i] = 0;
	}
	
	return 1;
}

void GenerateRandomSolution(int * data, int n)
{
	for (int i = 0; i < n; i++)
	{
		data[i] = i;
	}

	int aux;

	for (int i = 0; i < n; i++)
	{
		int randomIndex = floor((double)rand() / RAND_MAX * n);
		aux = data[randomIndex];
		data[randomIndex] = data[i];
		data[i] = aux;
	}
}

double Evaluate(int * data, int n, int ** matrix)
{
	int total = 0;

    if (DEBUG_EVALUATION) LOG(" >>> Evaluation - START\n");

	for (int i = 0; i < n - 1; i++)
	{
        int n1 = data[i];
        int n2 = data[i + 1];
		total += matrix[data[i]][data[i + 1]];
		if(DEBUG_EVALUATION) LOG("%d to %d = %d (total %d)\n", data[i], data[i + 1], matrix[data[i]][data[i + 1]], total);
	}

	total += matrix[data[n-1]][data[0]];
    if (DEBUG_EVALUATION) LOG("%d to %d = %d (total %d) - End\n", data[n - 1], data[0], matrix[data[n - 1]][data[0]], total);

    if (DEBUG_EVALUATION) LOG(" >>> Evaluation - END\n");

	return total;
}

void Mutate(int * data, int n)
{
    int randomIndex1 = floor((double)rand() / RAND_MAX * n);
    int randomIndex2 = floor((double)rand() / RAND_MAX * n);
    int aux = data[randomIndex1];
    data[randomIndex1] = data[randomIndex2];
    data[randomIndex2] = aux;
}

double Fitness(double result)
{
	double fMin = 0.00;
	if (result != 0.00)
		return (1.00 / (result + fMin));
	return (1.00 / (result + 0.000001 + fMin));
}


double DifferencePercent(double v1, double v2)
{
	double difference = fabs(v1 - v2);
	return difference / (fabs(v1 + v2) / 2.00);
}

void DisplaySolutiion(int * data, int n)
{
    for (int i = 0; i < n; i++)
    {
        LOG(" % 3d ", (int*)*(data + i));
    }
}

void DisplayGeneration(int * data, int n, int popSize, int ** matrix)
{
    for (int i = 0; i < popSize; i++)
    {

        for (int j = 0; j < n; j++)
        {
            LOG(" % 3d ", (int*)*(data + (i * n) + j));
        }

        int result = Evaluate(data + i * n, n, matrix);

        LOG(" - %d (fit. %f)\n", result, Fitness(result));
    }
}

double AG_Main()
{
	int n;									// NUMBER OF NODES
	int maxGenerations;						// MAX NUM OF GENERATIONS
	int popSize;							// SIZE OF POPULATION
	double mutationProb;					// PROBABILITY OF MUTATION
	int ** matrix = NULL;					// COST MATRIX

	if (!Initialize_AG(n, maxGenerations, popSize, mutationProb, matrix))
	{
		LOG("Failed to initialize!\n");
		return 0;
	}

	LOG("Initialization successful.\nNumber of nodes:%d\nPopulation size: %d\nMaximum number of generations: %d\n\n", n, popSize, maxGenerations);

	DisplayMatrix(matrix, n);

    // initialize stuff

    int * data = new int[n*popSize];
    int * newData = new int[n*popSize];
    int * aux;
    
	int globalBestResult;

    double * q = new double[popSize];
    double * p = new double[popSize];
    double * eval = new double[popSize];
    double * fit = new double[popSize];

    double T = 0;
    q[0] = 0;

    bool stop = false;

    int goodCrosses;
    int badCrosses;
    int neutralCrosses;

    int goodMutations;
    int badMutations;
    int neutralMutations;

    double currentBest;

    double startTime;
    double lastTime;

	// generate generation 0

	for (int i = 0; i < popSize; i++)
	{
		GenerateRandomSolution(data + i * n, n);

		int result = Evaluate(data + i * n, n, matrix);
	}

	int gen = 0;

	// evaluate generation 0
	
	for (int i = 0; i < popSize; i++)
	{
		if (i == 0)
		{
            int result = Evaluate(data + i * n, n, matrix);
            globalBestResult = result;

			LOG("Starting best result = %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));
            eval[i] = result;
            fit[i] = Fitness(result);
            T += fit[i];
		}
		else
		{
			int result = Evaluate(data + i * n, n, matrix);
            eval[i] = result;
            fit[i] = Fitness(result);
            T += fit[i];
			if (result < globalBestResult)
			{
				globalBestResult = result;
				LOG("New global best result = %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));
			}
		}
	}

    //DisplayGeneration(data, n, popSize, matrix);

	gen++;

    startTime = clock();
    lastTime = startTime;

	while (!stop)
	{
		LOG("\n >>> GENERATION #%d\n", gen);
		if (ESTIMATE_TIME_GENERATIONS)
		{
			double time = clock();
			if (time - lastTime >= 1000)
			{
				system("cls");
				lastTime = time;
				double timeElapsed = time - startTime;
				printf("\n[%06d]\n", gen);
				printf("Time elapsed: %.1f s\nTime remaining: %.1f s\n", timeElapsed / 1000.00, ((timeElapsed / gen)*(maxGenerations - gen)) / 1000.00);
			}
		}

		// do selection

        if(DEBUG_SELECTION) LOG("\n > Selection - START\n");

        if (DEBUG_SELECTION) LOG("Probabilities (q - p - fit):\n");
        for (int i = 0; i < popSize; i++)
        {
            p[i] = fit[i] / T;
            if (i > 0)
            {
                q[i] = q[i - 1] + p[i - 1];
            }

            if (DEBUG_SELECTION) LOG("#%02d - %f - %f - %f\n", i, q[i], p[i], fit[i]);
        }


        for (int i = 0; i < popSize; i++)
        {
            double r = (double)rand() / RAND_MAX;
            //if (DEBUG_SELECTION) LOG("#%02d - random = %f", i, r);
            if (r > q[popSize - 1])
            {
                memcpy(newData + n * i, data + n * (popSize - 1), n * sizeof(int));
                //if (DEBUG_SELECTION) LOG(" - OK !\n")
            }
            for (int j = 0; j < popSize - 1; j++)
            {
                if (q[j] < r && r <= q[j + 1])
                {
                    memcpy(newData + n * i, data + n * j, n * sizeof(int));
                    //if (DEBUG_SELECTION) LOG(" - OK\n")
                }
            }
        }

        aux = newData;
        newData = data;
        data = aux;

        if (DEBUG_SELECTION) LOG(" New population:\n");
        if (DEBUG_SELECTION) DisplayGeneration(data, n, popSize, matrix);

        if (DEBUG_SELECTION) LOG(" > Selection - END\n\n");

		goodCrosses = 0;
		badCrosses = 0;
		neutralCrosses = 0;

		// do crosses

		//LOG("Crosses results:\n Good crosses: %d\n Bad crosses: %d\n Neutral crosses: %d\n", goodCrosses, badCrosses, neutralCrosses);

		if (MUTATIONS_ENABLED)
		{
            goodMutations = 0;
            badMutations = 0;
            neutralMutations = 0;

            if (DEBUG_MUTATIONS) LOG("\n > Mutations - START\n");

            for (int i = 0; i < popSize; i++)
            {
                double r = (double)rand() / RAND_MAX;
                if (r < mutationProb)
                {
                    int init = Evaluate(data + i * n, n, matrix);

                    if (DEBUG_MUTATIONS) printf("Mutation #%d\n init - ", i);
                    if (DEBUG_MUTATIONS) DisplaySolutiion(data + i * n, n);
                    if (DEBUG_MUTATIONS) printf(" - %d (fit. %f)\n", init, Fitness(init));
                    
                    Mutate(data + i * n, n);

                    int fin = Evaluate(data + i * n, n, matrix);

                    if (DEBUG_MUTATIONS) printf(" fin  - ", i);
                    if (DEBUG_MUTATIONS) DisplaySolutiion(data + i * n, n);
                    if (DEBUG_MUTATIONS) printf(" - %d (fit. %f)\n", fin, Fitness(fin));

                    if (fin > init)
                    {
                        badMutations++;
                    }
                    else if(fin == init)
                    {
                        neutralMutations++;
                    }
                    else
                    {
                        goodMutations++;
                    }
                }
            }

            if (DEBUG_MUTATIONS) LOG(" > Mutations - END\n\n");

            LOG("Mutations result:\n Good mutations: %d\n Bad mutations: %d\n Neutral mutations: %d\n\n", goodMutations, badMutations, neutralMutations);
		}

		// evaluate generation + stopping condition
        
        T = 0;

        for (int i = 0; i < popSize; i++)
        {
            int result = Evaluate(data + i * n, n, matrix);
            eval[i] = result;
            fit[i] = Fitness(result);
            T += fit[i];
            if (result < globalBestResult)
            {
                globalBestResult = result;
                LOG("New global best result = %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));
            }
        }

        if (gen == maxGenerations) stop = true;

        

		gen++;
	}

	LOG("Best result: %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));

	double endTime = clock();
	LOG("Time elapsed: %.1f s\n", (endTime - startTime) / 1000.00);

	delete[n*n] matrix;

    system("pause");

	return globalBestResult;
}

int main()
{
	srand(time(NULL));

	CreateDirectory("data", NULL);
	CreateDirectory("data\\TEST", NULL);

	fopen_s(&resultsfile, "data\\TEST\\resultsfile.log", "w");
	double result;
	double startTime = clock();
	double lastTime = startTime;

	for (int iteration = 0; iteration < ITERATIONS; iteration++)
	{
		if (ESTIMATE_TIME_ITERATIONS)
		{
			double time = clock();
			if (time - lastTime >= 1000)
			{
				system("cls");
				lastTime = time;
				double timeElapsed = time - startTime;
				printf("\n[%06d]\n", iteration);
				printf("Time elapsed: %.1f s\nTime remaining: %.1f s\n", timeElapsed / 1000.00, ((timeElapsed / iteration)*(ITERATIONS - iteration)) / 1000.00);
			}
		}

		char * logfilename = new char[MAX_PATH];
		char * evolutionfilename = new char[MAX_PATH];
		char * avgfitnessfilename = new char[MAX_PATH];

		snprintf(logfilename, MAX_PATH, "data\\TEST\\%03d_Log.log", iteration);
		snprintf(evolutionfilename, MAX_PATH, "data\\TEST\\%03d_Evolution.log", iteration);
		snprintf(avgfitnessfilename, MAX_PATH, "data\\TEST\\%03d_Fitness.log", iteration);

		fopen_s(&logfile, logfilename, "w");
		fopen_s(&evolutionfile, evolutionfilename, "w");
		fopen_s(&avgfitnessfile, avgfitnessfilename, "w");

		int result = AG_Main();
		fprintf(resultsfile, "%d\n", result);

		fclose(logfile);
		fclose(evolutionfile);
		fclose(avgfitnessfile);
	}
	double endTime = clock();
	fprintf(resultsfile, "\nTime elapsed: %.1f s", (endTime - startTime) / 1000.00);

	fclose(resultsfile);

	return 0;
}

