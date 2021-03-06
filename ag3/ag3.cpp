#include "stdafx.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <Windows.h>

#define LOG_CONSOLE_ENABLED					0
#define LOG_FILE_ENABLED					1
#define ESTIMATE_TIME_ITERATIONS			1
#define ESTIMATE_TIME_GENERATIONS			0
#define MINIMIZE							1

#define ITERATIONS							500
#define MUTATIONS_ENABLED					1

#define DEBUG_POPULATION_GENERATION         0
#define DEBUG_EVALUATION                    0
#define DEBUG_SELECTION                     0
#define DEBUG_CROSSING                      0
#define DEBUG_MUTATIONS                     0
#define DEBUG_CHECK_INTEGRITY               0

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

int Initialize_AG(int & n, int & maxGenerations, int & popSize, double & mutationProbability, double & crossProbability, int ** &matrix)
{

	FILE * input;
	fopen_s(&input, "input.txt", "r");
	
	fscanf_s(input, "%d\n", &n);

	maxGenerations = 5000;
	popSize = 1000;
	mutationProbability = 0.1;
	crossProbability = 0.5;

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

int Evaluate(int * data, int n, int ** matrix)
{
    int total = 0;
    int n1, n2;

    if (DEBUG_EVALUATION) LOG(" >>> Evaluation - START\n");

    for (int i = 0; i < n - 1; i++)
    {
        n1 = *(int*)(data + i);
        n2 = *(int*)(data + 1 + i);

        if (n1 >= n || n2 >= n || n1 < 0 || n2 < 0)
        {
            LOG("\n!!!! BAD THING HAPPENED !!!!\n");
            //system("pause");
        }

        total += matrix[n1][n2];
        if (DEBUG_EVALUATION) LOG("%d to %d = %d (total %d)\n", n1, n2, matrix[n1][n2], total);
    }

    n1 = *(int*)(data + n - 1);
    n2 = *(int*)(data);

    total += matrix[data[n1]][n2];
    if (DEBUG_EVALUATION) LOG("%d to %d = %d (total %d) - End\n", n1, n2, matrix[n1][n2], total);

    if (DEBUG_EVALUATION) LOG(" >>> Evaluation - END\n");

    return total;
}

double Fitness(int result)
{
    double res = (1.00 / result);
    if (isinf(res))
    {
        LOG(" !!!! BAD THING HAPPENED !!!!\n Fitness is inf for %d\n", result);
        //system("pause");
    }
    return res;;
    
}

void DisplaySolution(int * data, int n)
{
    for (int i = 0; i < n; i++)
    {
        LOG(" % 3d ", *(int*)(data + i));
    }
}

void DisplayGeneration(int * data, int n, int popSize, int ** matrix)
{
    for (int i = 0; i < popSize; i++)
    {
        LOG("#%03d > ", i);
        for (int j = 0; j < n; j++)
        {
            LOG(" % 3d ", *(int*)(data + (i * n) + j));
        }

        int result = Evaluate(data + i * n, n, matrix);

        LOG(" - %d (fit. %f)\n", result, Fitness(result));
    }
}

void GenerateRandomSolution(int * data, int n)
{
    if (DEBUG_POPULATION_GENERATION) LOG("\n > Generation - START\n");

	for (int i = 0; i < n; i++)
	{
		data[i] = i;
	}

	int aux;

	for (int i = 0; i < n; i++)
	{
		int randomIndex = floor((double)rand() / RAND_MAX * (n - 1));

        if (DEBUG_POPULATION_GENERATION) LOG("Swapping #%d with #%d\n", i, randomIndex);

		aux = data[randomIndex];
		data[randomIndex] = data[i];
		data[i] = aux;
	}

    if (DEBUG_POPULATION_GENERATION) DisplaySolution(data, n);

    if (DEBUG_POPULATION_GENERATION) LOG(" > Generation - END\n\n");
}

void Mutate(int * data, int n)
{
    int randomIndex1 = floor((double)rand() / RAND_MAX * (n - 1));
    int randomIndex2 = floor((double)rand() / RAND_MAX * (n - 1));
    int aux = data[randomIndex1];
    data[randomIndex1] = data[randomIndex2];
    data[randomIndex2] = aux;
}

void Cross(int * data1, int * data2, int n)
{
	int i;
	bool * freq = new bool[n + 1];
	int * data1copy = new int[n];
	memcpy(data1copy, data1, n * sizeof(int));

	int currentIndex;

	memset(freq, 0, n);
	int cutIndex = rand() % (n - 1) + 1;

	for (i = 0; i < cutIndex; ++i)
		freq[data1[i]] = 1;

	currentIndex = cutIndex;

	for (i = 0; i < n; ++i)
		if (!freq[data2[i]])
			data1[currentIndex++] = data2[i];

	memset(freq, 0, n);
	for (i = 0; i < cutIndex; ++i)
		freq[data2[i]] = 1;

	currentIndex = cutIndex;

	for (i = 0; i < n; ++i)
		if (!freq[data1copy[i]])
			data2[currentIndex++] = data1copy[i];

	delete[n] freq;
	delete[n] data1copy;
}


double DifferencePercent(double v1, double v2)
{
	double difference = fabs(v1 - v2);
	return difference / (fabs(v1 + v2) / 2.00);
}

void GenerationIntegrityCheck(int * data, int n, int popSize, int ** matrix)
{
    for (int i = 0; i < popSize; i++)
    {
        int * freq = new int[n];
        memset(freq, 0, n * sizeof(int));
        for (int j = 0; j < n; j++)
        {
            freq[*(int*)(data + i * n + j)]++;
        }
        for (int j = 0; j < n; j++)
        {
            if (freq[j] != 1)
            {

                LOG("DUPLICATE ELEMENTS FOUND (#%d)! ABORTING!\n\nPopulation:\n\n", i);

                DisplayGeneration(data, n, popSize, matrix);
                delete[n] freq;
                exit(1);
            }
        }
        delete[n] freq;
    }
}

int Initialize_HC(int & n, int ** &matrix)
{

	FILE * input;
	fopen_s(&input, "input.txt", "r");

	fscanf_s(input, "%d\n", &n);

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

void SwitchPos(int * data, int pos1, int pos2)
{
	int aux = data[pos1];
	data[pos1] = data[pos2];
	data[pos2] = aux;
}

double HillClimbing()
{
	int n;                                    // NUMBER OF NODES
	int ** matrix = NULL;                    // COST MATRIX

	if (!Initialize_HC(n, matrix))
	{
		LOG("Failed to initialize!\n");
		return 0;
	}

	LOG("Initialization successful.\nNumber of nodes:%d\n\n", n);

	int * data = new int[n];
	int currentResult;
	int newResult;
	bool foundBetterSolution = 1;

	GenerateRandomSolution(data, n);
	//DisplaySolutiion(data, n);
	//printf("%d\n", Evaluate(data, n, matrix));

	currentResult = Evaluate(data, n, matrix);

    int i = 0;

    fprintf_s(evolutionfile, "%d %d\n", i, currentResult);
    fprintf_s(avgfitnessfile, "%d %f\n", i, Fitness(currentResult));

    double startTime = clock();

	while (foundBetterSolution)
	{
		foundBetterSolution = 0;
		for (int i = 0; i < n - 1; ++i)
		{
			int j = i;
			while (j < n)
			{
				SwitchPos(data, i, j);
				newResult = Evaluate(data, n, matrix);

				if (newResult < currentResult)
				{
					currentResult = newResult;
					foundBetterSolution = 1;
					break;
				}

				SwitchPos(data, i, j);
				j += rand() % (int)(floor(sqrt(n)) + 1);
			}
		}
        i++;

        fprintf_s(evolutionfile, "%d %d\n", i, currentResult);
        fprintf_s(avgfitnessfile, "%d %f\n", i, Fitness(currentResult));
	}
	
    LOG("Best result: %d (fit. %f)\n", currentResult, Fitness(currentResult));

    double endTime = clock();
    LOG("Time elapsed: %.1f s\n", (endTime - startTime) / 1000.00);

	return currentResult;
}

double HillClimbingBI()
{
	int n;                                    // NUMBER OF NODES
	int ** matrix = NULL;                    // COST MATRIX

	if (!Initialize_HC(n, matrix))
	{
		LOG("Failed to initialize!\n");
		return 0;
	}

	LOG("Initialization successful.\nNumber of nodes:%d\n\n", n);

	int * data = new int[n];
	int indexIbest, indexJbest;
	int currentResult;
	int newResult;
	bool foundBetterSolution = 1;

	GenerateRandomSolution(data, n);
	//DisplaySolutiion(data, n);
	//printf("%d\n", Evaluate(data, n, matrix));

	currentResult = Evaluate(data, n, matrix);

    int i = 0;

    fprintf_s(evolutionfile, "%d %d\n", i, currentResult);
    fprintf_s(avgfitnessfile, "%d %f\n", i, Fitness(currentResult));

    double startTime = clock();

	while (foundBetterSolution)
	{
		foundBetterSolution = 0;
		for (int i = 0; i < n - 1; ++i)
		{
			int j = i;
			while (j < n)
			{
				SwitchPos(data, i, j);
				newResult = Evaluate(data, n, matrix);

				if (newResult < currentResult)
				{
					currentResult = newResult;
					foundBetterSolution = 1;
					indexIbest = i;
					indexJbest = j;
				}

				SwitchPos(data, i, j);
				j += rand() % (int)(floor(sqrt(n)) + 1);
			}
		}
		SwitchPos(data, indexIbest, indexJbest);

        i++;

        fprintf_s(evolutionfile, "%d %d\n", i, currentResult);
        fprintf_s(avgfitnessfile, "%d %f\n", i, Fitness(currentResult));
	}

    LOG("Best result: %d (fit. %f)\n", currentResult, Fitness(currentResult));

    double endTime = clock();
    LOG("Time elapsed: %.1f s\n", (endTime - startTime) / 1000.00);

	return currentResult;
}

double HillClimbingAllNeighbors()
{
	int n;                                    // NUMBER OF NODES
	int ** matrix = NULL;                    // COST MATRIX

	if (!Initialize_HC(n, matrix))
	{
		LOG("Failed to initialize!\n");
		return 0;
	}

	LOG("Initialization successful.\nNumber of nodes:%d\n\n", n);

	int * data = new int[n];
	int currentResult;
	int newResult;
	bool foundBetterSolution = 1;

	GenerateRandomSolution(data, n);
	//DisplaySolutiion(data, n);
	//printf("%d\n", Evaluate(data, n, matrix));

	currentResult = Evaluate(data, n, matrix);

    int i = 0;

    fprintf_s(evolutionfile, "%d %d\n", i, currentResult);
    fprintf_s(avgfitnessfile, "%d %f\n", i, Fitness(currentResult));

    double startTime = clock();

	while (foundBetterSolution)
	{
		foundBetterSolution = 0;
		for (int i = 0; i < n - 1; ++i)
		{
			int j = i;
			while (j < n)
			{
				SwitchPos(data, i, j);
				newResult = Evaluate(data, n, matrix);

				if (newResult < currentResult)
				{
					currentResult = newResult;
					foundBetterSolution = 1;
					break;
				}

				SwitchPos(data, i, j);
				j++;
			}
		}
        i++;

        fprintf_s(evolutionfile, "%d %d\n", i, currentResult);
        fprintf_s(avgfitnessfile, "%d %f\n", i, Fitness(currentResult));
    }

    LOG("Best result: %d (fit. %f)\n", currentResult, Fitness(currentResult));

    double endTime = clock();
    LOG("Time elapsed: %.1f s\n", (endTime - startTime) / 1000.00);

	return currentResult;
}

double Greedy()
{
	int n;                                    // NUMBER OF NODES
	int ** matrix = NULL;                    // COST MATRIX

	if (!Initialize_HC(n, matrix))
	{
		LOG("Failed to initialize!\n");
		return 0;
	}

	bool * visited = new bool[n];
	memset(visited, 0, n);
	int firstNode = rand() % n;
	int currentNode = firstNode;
	int sum = 0;
	int minJ;
	int visitedNodes = 1;
	visited[currentNode] = 1;
	
	
	while (visitedNodes < n)
	{
		int min = ~(1 << 31);
		for (int j = 0; j < n; ++j)
			if (!visited[j] && matrix[currentNode][j] < min)
			{
				minJ = j;
				min = matrix[currentNode][j];
			}
		visited[minJ] = 1;
		sum += matrix[currentNode][minJ];
		currentNode = minJ;
		visitedNodes++;
	}

	sum += matrix[currentNode][firstNode];

	return sum;
}

double AG_Main()
{
	int n;									// NUMBER OF NODES
	int maxGenerations;						// MAX NUM OF GENERATIONS
	int popSize;							// SIZE OF POPULATION
	double mutationProb;					// PROBABILITY OF MUTATION
	double crossProb;						// PROBABILITY OF CROSSING
	int ** matrix = NULL;					// COST MATRIX

	if (!Initialize_AG(n, maxGenerations, popSize, mutationProb, crossProb, matrix))
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
    double R = 0;
    q[0] = 0;

    bool stop = false;

    int goodCrosses;
    int badCrosses;
    int neutralCrosses;

    int goodMutations;
    int badMutations;
    int neutralMutations;

    double currentBest;

    int gen;

    double startTime;
    double lastTime;

    double minFitness;
    double maxFitness;
    double avgFitness;
    double maxFitnessGlobal;

    double minResult;
    double maxResult;
    double avgResult;
    double minResultGlobal;

	// generate generation 0

    gen = 0;

	for (int i = 0; i < popSize; i++)
	{
        if (DEBUG_POPULATION_GENERATION) LOG("Solution #%d\n", i);
		GenerateRandomSolution(data + i * n, n);
	}

    
    LOG(" Starting generation:\n");
    DisplayGeneration(data, n, popSize, matrix);

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
            R += eval[i];

            minFitness = fit[i];
            maxFitness = fit[i];
            maxFitnessGlobal = fit[i];

            minResult = eval[i];
            maxResult = eval[i];
            minResultGlobal = eval[i];
        }
        else
        {
            int result = Evaluate(data + i * n, n, matrix);

            if (result < globalBestResult)
            {
                globalBestResult = result;
                LOG("New global best result = %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));
            }
            
            eval[i] = result;
            fit[i] = Fitness(result);
            T += fit[i];
            R += eval[i];

            if (fit[i] > maxFitness)
            {
                maxFitness = fit[i];
                minResult = eval[i];
            }
            if (fit[i] < minFitness)
            {
                minFitness = fit[i];
                maxResult = eval[i];
            }
            if (fit[i] > maxFitnessGlobal)
            {
                maxFitnessGlobal = fit[i];
                minResultGlobal = eval[i];
            }
        }
    }

    avgResult = R / popSize;
    avgFitness = T / popSize;

    fprintf_s(avgfitnessfile, "%d %lf %lf %lf %lf\n", gen, minFitness, maxFitness, maxFitnessGlobal, avgFitness);
    fprintf_s(evolutionfile, "%d %lf %lf %lf %lf\n", gen, maxResult, minResult, minResultGlobal, avgResult);

    if (DEBUG_CHECK_INTEGRITY) GenerationIntegrityCheck(data, n, popSize, matrix);
    
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

        if (DEBUG_CHECK_INTEGRITY) GenerationIntegrityCheck(data, n, popSize, matrix);

		goodCrosses = 0;
		badCrosses = 0;
		neutralCrosses = 0;

		// do crosses

        if (DEBUG_CROSSING) LOG("\n > Crossing - START\n");

        for (int i = 0; i < popSize - 1; i += 2)
        {
            if ((double)rand() / RAND_MAX < crossProb)
            {
                int init1 = Evaluate(data + i * n, n, matrix);
                int init2 = Evaluate(data + (i + 1) * n, n, matrix);
                double initAvg = (init1 + init2) / 2.00;
                double initFitAvg = (Fitness(init1) + Fitness(init2)) / 2.00;

                if (DEBUG_CROSSING) LOG("Crossing %d and %d\n init:\n", i, i + 1);
                if (DEBUG_CROSSING) DisplaySolution(data + i * n, n);
                if (DEBUG_CROSSING) LOG("\n");
                if (DEBUG_CROSSING) DisplaySolution(data + (i + 1) * n, n);
                if (DEBUG_CROSSING) LOG("\nInitial avg: %f (fit. %f)\n", initAvg, initFitAvg);

                Cross(data + i * n, data + (i + 1) * n, n);

                int fin1 = Evaluate(data + i * n, n, matrix);
                int fin2 = Evaluate(data + (i + 1) * n, n, matrix);
                double finAvg = (fin1 + fin2) / 2.00;
                double finFitAvg = (Fitness(fin1) + Fitness(fin2)) / 2.00;

                if (DEBUG_CROSSING) LOG(" fin:\n", i, i + 1);
                if (DEBUG_CROSSING) DisplaySolution(data + i * n, n);
                if (DEBUG_CROSSING) LOG("\n");
                if (DEBUG_CROSSING) DisplaySolution(data + (i + 1) * n, n);
                if (DEBUG_CROSSING) LOG("\nFinal avg: %f (fit. %f)\n", finAvg, finFitAvg);

                if (finFitAvg > initFitAvg)
                {
                    goodCrosses++;
                }
                else if (finFitAvg == initFitAvg)
                {
                    neutralCrosses++;
                }
                else
                {
                    badCrosses++;
                }
            }
        }

        if (DEBUG_CROSSING) LOG(" > Crossing - END\n\n");

        if (DEBUG_CHECK_INTEGRITY) GenerationIntegrityCheck(data, n, popSize, matrix);

		LOG("Crosses results:\n Good crosses: %d\n Bad crosses: %d\n Neutral crosses: %d\n", goodCrosses, badCrosses, neutralCrosses);

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

                    if (DEBUG_MUTATIONS) LOG("Mutation #%d\n init - ", i);
                    if (DEBUG_MUTATIONS) DisplaySolution(data + i * n, n);
                    if (DEBUG_MUTATIONS) LOG(" - %d (fit. %f)\n", init, Fitness(init));
                    
                    Mutate(data + i * n, n);

                    int fin = Evaluate(data + i * n, n, matrix);

                    if (DEBUG_MUTATIONS) LOG(" fin  - ", i);
                    if (DEBUG_MUTATIONS) DisplaySolution(data + i * n, n);
                    if (DEBUG_MUTATIONS) LOG(" - %d (fit. %f)\n", fin, Fitness(fin));

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

            if (DEBUG_CHECK_INTEGRITY) GenerationIntegrityCheck(data, n, popSize, matrix);

            LOG("Mutations result:\n Good mutations: %d\n Bad mutations: %d\n Neutral mutations: %d\n\n", goodMutations, badMutations, neutralMutations);
		}



		// evaluate generation + stopping condition
        
        T = 0;
        R = 0;

        for (int i = 0; i < popSize; i++)
        {
            int result = Evaluate(data + i * n, n, matrix);

            if (result < globalBestResult)
            {
                globalBestResult = result;
                LOG("New global best result = %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));
            }

            if (result == 0)
            {
                DisplayGeneration(data, n, popSize, matrix);
            }

            eval[i] = result;
            fit[i] = Fitness(result);
            T += fit[i];
            R += eval[i];
            
            if (i == 0)
            {
                maxFitness = fit[i];
                minFitness = fit[i];
                minResult = eval[i];
                maxResult = eval[i];
            }
            else
            {
                if (fit[i] > maxFitness)
                {
                    maxFitness = fit[i];
                    minResult = eval[i];
                }
                if (fit[i] < minFitness)
                {
                    minFitness = fit[i];
                    maxResult = eval[i];
                }
            }

            if (fit[i] > maxFitnessGlobal)
            {
                maxFitnessGlobal = fit[i];
                minResultGlobal = eval[i];
            }
        }

        avgFitness = T / popSize;
        avgResult = R / popSize;

        fprintf_s(avgfitnessfile, "%d %lf %lf %lf %lf\n", gen, minFitness, maxFitness, maxFitnessGlobal, avgFitness);
        fprintf_s(evolutionfile, "%d %lf %lf %lf %lf\n", gen, maxResult, minResult, minResultGlobal, avgResult);

        if (gen == maxGenerations) stop = true;

		gen++;
	}

    LOG(" Final generation:\n");
    DisplayGeneration(data, n, popSize, matrix);

	LOG("Best result: %d (fit. %f)\n", globalBestResult, Fitness(globalBestResult));

	double endTime = clock();
	LOG("Time elapsed: %.1f s\n", (endTime - startTime) / 1000.00);

	delete[n*n] matrix;

    delete[n*popSize] data;
    delete[n*popSize] newData;
    
    delete[popSize] q;
    delete[popSize] p;
    delete[popSize] eval;
    delete[popSize] fit;

    //system("pause");

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

		int result = Greedy();
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

