// Includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <omp.h>
#include "Timing.h"
#include <CL/cl.hpp>
using namespace std;

// gloabal vars
char dead = '.';
char alive = 'x';
int arrayHeight = 0;
int arrayWidth = 0;
int generations = 250;
string inFile;
string outFile;
bool measure = true;
int mode = 0;

int threads = 4;

int* gridCL;
int* gridTempCL;
int gridSize;

const std::string KERNEL = "kernel.cl";
cl::Program program;
cl::Buffer gridBuffer;
cl::Buffer gridBufferTemp;
cl::Kernel kernel;
cl::CommandQueue cQueue;

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

void deinitGrid(char** grid)
{
	for (int i = 0; i < arrayHeight; ++i) {
		delete[] grid[i];
	}
	delete[] grid;
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
		cout << "Output file could not be opened. Invalid filename or destination";
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
		cout << "Input file could not be opened. Invalid filename or destination";
		exit(0);
	}
	input.close();

	return grid;
}

char** nextGenerationSeq(char** grid, int arrayWidth, int arrayHeight)
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
	deinitGrid(aliveNeighbours);

	return grid;
}

char** nextGenerationOpenMP(char** grid, int arrayWidth, int arrayHeight)
{
	char** aliveNeighbours = initGrid(arrayWidth, arrayHeight);

#pragma omp parallel num_threads(threads)
	#pragma omp for
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

#pragma omp parallel num_threads(threads)
	#pragma omp for
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
	deinitGrid(aliveNeighbours);

	return grid;
}

bool createGridFromFileCL(string fileName)
{
	ifstream input(fileName);
	if (input.is_open() && input.good())
	{
		string line;
		char comma;
		input >> arrayWidth;
		input >> comma;
		input >> arrayHeight;

		gridSize = arrayWidth * arrayHeight;

		gridCL = new int[gridSize];
		gridTempCL = new int[gridSize];

		int lineCount = 0;
		getline(input, line);
		while (getline(input, line))
		{
			int lineSize = line.size();
			{
				for (int i = 0; i < lineSize; i++)
				{
					gridCL[i + arrayWidth * lineCount] = line[i] == alive;
					gridTempCL[i + arrayWidth * lineCount] = false;
				}
			}
			++lineCount;
		}
	}
	else
	{
		cout << "Input file could not be opened. Invalid filename or destination";
		exit(0);
	}
	input.close();

	return true;
}

bool initOpenCL()
{
	// get all platforms (drivers - CUDA)
	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);

	if (all_platforms.size() == 0) {
		cout << " No platforms found. Check OpenCL installation!\n";
		exit(1);
	}

	cl::Platform default_platform = all_platforms[0];
	//cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

	// get default device (CPUs, GPUs) of the default platform
	std::vector<cl::Device> all_devices;
	default_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
	if (all_devices.size() == 0) {
		cout << " No devices found. Check OpenCL installation!\n";
		exit(1);
	}

	// device[0] equals to gpu 
	cl::Device default_device = all_devices[0];
	//cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

	// a context is like a "runtime link" to the device and platform
	cl::Context context({ default_device });

	// create the program that we want to execute on the device
	cl::Program::Sources sources;

	ifstream sourceFile(KERNEL);
	// if no file found, log error and quit
	if (!sourceFile)
	{
		cout << "kernel source file " << "kernel.cl" << " not found!" << std::endl;
		exit(1);
	}

	// parse the file
	string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
	sources.push_back({ sourceCode.c_str(), sourceCode.length() });

	// compile
    cl::Program program(context, sources);
    if (program.build({ default_device }) != CL_SUCCESS)
    {
        cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << "\n";
        exit(1);
    }

    // create buffers
    gridBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int) * gridSize);
    gridBufferTemp = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int) * gridSize);

    cQueue = cl::CommandQueue(context, default_device);

    cQueue.enqueueWriteBuffer(gridBuffer, CL_TRUE, 0, sizeof(int) * gridSize, gridCL);
    cQueue.enqueueWriteBuffer(gridBufferTemp, CL_TRUE, 0, sizeof(int) * gridSize, gridTempCL);

	kernel = cl::Kernel(program, "next_Gen");

    return true;
}

bool nextGenerationOpenCL()
{
    // temp switches the buffer to be written to
    bool temp = true;
    for (int i = 0; i < generations; i++)
    {
        temp = !temp;

        kernel.setArg(0, gridBuffer);
        kernel.setArg(1, gridBufferTemp);
        kernel.setArg(2, cl_int(arrayWidth));
        kernel.setArg(3, cl_int(arrayHeight));
		kernel.setArg(4, cl_int(temp));
        cQueue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(arrayWidth, arrayHeight), cl::NullRange);
        cQueue.finish();
    }
    if (temp)
    {
        cQueue.enqueueReadBuffer(gridBuffer, CL_TRUE, 0, sizeof(int) * gridSize, gridCL);
    }
    else
    {
        cQueue.enqueueReadBuffer(gridBufferTemp, CL_TRUE, 0, sizeof(int) * gridSize, gridTempCL);
        std::swap(gridTempCL, gridCL);
    }

    return true;
}

bool writeGridToFileCL(string fileName)
{
	ofstream output(fileName);
	if (output.is_open() && output.good())
	{
		output << arrayWidth << "," << arrayHeight << "\n";
		for (int i = 0; i < arrayHeight; i++)
		{
			for (int j = 0; j < arrayWidth; j++)
			{
				output << (gridCL[j + arrayWidth * i] ? 'x' : '.');
			}
			output << "\n";
		}
		output.flush();
	}
	else
	{
		cout << "Output file could not be opened. Invalid filename or destination";
		exit(0);
	}
	output.close();

    return true;
}

// helper func - compare if grid values match
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

void print_usage() {
	std::cerr << "Usage: gol --mode seq/omp/ocl [--threads threadNum] --load inFile.gol --save outFile.gol --generations number [--measure]" << std::endl;
}

int main(int argc, char* argv[]) 
{
	if (argc < 6) {
		print_usage();
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		if (string(argv[i]) == "--mode") {
			if (i + 1 < argc) {
				if (string(argv[i + 1]) == "seq") {
					mode = 1;
				}
				else if (string(argv[i + 1]) == "omp") {
					mode = 2;
				}
				else if (string(argv[i + 1]) == "ocl") {
					mode = 3;
				}
				else {
					print_usage();
					return 1;
				}
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (string(argv[i]) == "--threads") {
			if (i + 1 < argc) {
				threads = std::stoi(argv[i + 1]);
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (string(argv[i]) == "--load") {
			if (i + 1 < argc) {
				inFile = argv[i + 1];
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (string(argv[i]) == "--save") {
			if (i + 1 < argc) {
				outFile = argv[i + 1];
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (string(argv[i]) == "--generations") {
			if (i + 1 < argc) {
				generations = std::stoi(argv[i + 1]);
			}
			else {
				print_usage();
				return 1;
			}
		}
		else if (string(argv[i]) == "--measure") {
			measure = true;
		}
	}

	// start the Timing class
	Timing* timer = Timing::getInstance();
	char** grid;

	switch (mode) {
		case 1:
			timer->startSetup();
			grid = createGridFromFile(inFile);
			timer->stopSetup();

			timer->startComputation();
			for (int i = 0; i < generations; i++)
			{
				grid = nextGenerationSeq(grid, arrayWidth, arrayHeight);
			}
			timer->stopComputation();

			timer->startFinalization();
			writeGridToFile(grid, outFile);
			timer->stopFinalization();
			deinitGrid(grid);
			break;
		case 2:
			timer->startSetup();
			grid = createGridFromFile(inFile);
			timer->stopSetup();

			timer->startComputation();
				for (int i = 0; i < generations; i++)
				{
					grid = nextGenerationOpenMP(grid, arrayWidth, arrayHeight);
				}	
			timer->stopComputation();
			
			timer->startFinalization();
			writeGridToFile(grid, outFile);
			timer->stopFinalization();
			deinitGrid(grid);
			break;
		case 3:
			timer->startSetup();
			createGridFromFileCL(inFile);
			initOpenCL();
			timer->stopSetup();

			timer->startComputation();
			nextGenerationOpenCL();
			timer->stopComputation();

			timer->startFinalization();
			writeGridToFileCL(outFile);
			timer->stopFinalization();
			break;
	}

	// Print timing results
	if (measure) {
		cout << timer->getResults() << endl;
	}

	//compGrids(createGridFromFile("random1500_out.gol"), createGridFromFile("test.txt"));

	return 0;
}