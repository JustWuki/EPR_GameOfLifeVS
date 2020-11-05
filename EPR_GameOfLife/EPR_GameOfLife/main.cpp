// Includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include "Timing.h"
using namespace std;

// func declarations
char** initGrid(int arrayWidth, int arrayHeight);
void deinitArray(char** grid);
void printArray(char** grid, int arrayWidth, int arrayHeight);
char** nextGeneration(char** grid, int arrayWidth, int arrayHeight);
char** createGridFromFile(string fileName);
void writeGridToFile(char** grid, string fileName);
void compGrids(char** gridOne, char** gridTwo);

// gloabal vars
char dead = '.';
char alive = 'x';
int arrayHeight = 0;
int arrayWidth = 0;
int generations = 250;
string inFile;
string outFile;
bool measure = false;

void print_usage() {
	std::cerr << "Usage: gol --load inFile.gol --save outFile.gol --generations number [--measure]" << std::endl;
}

int main(int argc, char* argv[]) 
{
	//compGrids(createGridFromFile("random250_out.gol"), createGridFromFile("write.txt"));
	//return 0;
	if (argc < 6) {
		print_usage();
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "--load") {
			if (i + 1 < argc) {
				inFile = argv[i + 1];
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (std::string(argv[i]) == "--save") {
			if (i + 1 < argc) {
				outFile = argv[i + 1];
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (std::string(argv[i]) == "--generations") {
			if (i + 1 < argc) {
				generations = std::stoi(argv[i + 1]);
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (std::string(argv[i]) == "--measure") {
			measure = true;
		}
	}
	// start the Timing class
	Timing* timer = Timing::getInstance();

	timer->startSetup();
	char** grid = createGridFromFile(inFile);

	timer->stopSetup();

	timer->startComputation();
	for (int i = 0; i < generations; i++)
	{
		grid = nextGeneration(grid, arrayWidth, arrayHeight);
	}
	timer->stopComputation();

	timer->startFinalization();
	writeGridToFile(grid, outFile);
	timer->stopFinalization();

	// Print timing results
	if (measure) {
		cout << timer->getResults() << endl;
	}
	deinitArray(grid);

	return 0;
}

void writeGridToFile(char** grid, string fileName) 
{
	ofstream output(fileName);
	if (output.is_open() && output.good())
	{
		output << arrayWidth << "," << arrayHeight << "\n";
		for (int i = 0; i < arrayHeight; i++)
		{
			for (int j = 0; j < arrayWidth; j++)
			{
				output << grid[i][j];
			}
			output << "\n";

		}
	}
	else
	{
		std::cout << "Output file could not be opened. Invalid filename or destination";
		exit(0);
	}
	output.close();
}

char** createGridFromFile(string fileName)
{
	char** grid;

	ifstream input(fileName);
	if (input.is_open() && input.good())
	{
		string gridSize;
		vector<int> gridSizeValues;
		input >> gridSize;

		stringstream ss(gridSize);
		for (int i; ss >> i;)
		{
			gridSizeValues.push_back(i);
			if (ss.peek() == ',') 
			{
				ss.ignore();
			}
		}

		arrayWidth = gridSizeValues[0];
		arrayHeight = gridSizeValues[1];

		grid = initGrid(arrayWidth, arrayHeight);

		for (int i = 0; i < arrayHeight; i++)
		{
			for (int j = 0; j < arrayWidth; j++)
			{
				input >> grid[i][j];
			}
		}
	} 
	else 
	{
		std::cout << "Input file could not be opened. Invalid filename or destination";
		exit(0);
	}
	input.close();

	return grid;
}

char** initGrid(int arrayWidth, int arrayHeight)
{
	char** cellsArray;
	cellsArray = new char* [arrayHeight];
	for (int i = 0; i < arrayHeight; i++)
	{
		cellsArray[i] = new char[arrayWidth];
	}

	return cellsArray;
}

void deinitArray(char** grid) 
{
	for (int i = 0; i < arrayHeight; ++i) {
		delete[] grid[i];
	}
	delete[] grid;
}

void printArray(char** grid, int arrayWidth, int arrayHeight)
{
	for (int i = 0; i < arrayHeight; i++)
	{
		for (int j = 0; j < arrayWidth; j++)
		{
			std::cout << grid[i][j] << " |";
		}
		std::cout << "\n";
	}
	std::cout << "\n";
}

char** nextGeneration(char** grid, int arrayWidth, int arrayHeight)
{
	char** aliveNeighbours = initGrid(arrayWidth, arrayHeight);

	// Loop through every cell 
	for (int i = 0; i < arrayHeight; i++)
	{
		for (int j = 0; j < arrayWidth; j++)
		{
			// finding no Of Neighbours that are alive 
			int aliveNeighboursCount = 0;
			for (int n = -1; n < 2; n++)
			{
				for (int m = -1; m < 2; m++)
				{
					// Ignore current cell
					if (!(n == 0 && m == 0))
					{
						if (grid[(i + n + arrayHeight) % arrayHeight][(j + m + arrayWidth) % arrayWidth] == alive)
						{
							++aliveNeighboursCount;
						}
					}
				}
			}

			aliveNeighbours[i][j] = aliveNeighboursCount;
		}
	}

	for (int i = 0; i < arrayHeight; i++)
	{
		for (int j = 0; j < arrayWidth; j++)
		{
			int aliveNeighboursCount = aliveNeighbours[i][j];
			// Implementing the Rules of Life 

			// Cell is lonely and dies 
			if ((grid[i][j] == alive) && (aliveNeighboursCount < 2))
				grid[i][j] = dead;

			// Cell dies due to over population 
			else if ((grid[i][j] == alive) && (aliveNeighboursCount > 3))
				grid[i][j] = dead;

			// A new cell is born 
			else if ((grid[i][j] == dead) && (aliveNeighboursCount == 3))
				grid[i][j] = alive;

			// Remains the same 
			else
				grid[i][j] = grid[i][j];
		}
	}
	deinitArray(aliveNeighbours);

	return grid;
}

// compare if grid values match
void compGrids(char** gridOne, char** gridTwo) 
{
	int count = 0;
	for (int i = 0; i < arrayHeight; i++)
	{
		for (int j = 0; j < arrayWidth; j++)
		{
			if (gridTwo[i][j] != gridOne[i][j])
			{
				cout << count;
				cout << "The grids are not the same" << endl;
				exit(0);
			}
			count++;
		}
	}
	cout << "The grids are matching" << endl;
}