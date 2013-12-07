#define __CL_ENABLE_EXCEPTIONS

#include <stdio.h>
#include "cl.hpp"
#include "util.hpp"
#include <vector>

using namespace std;
using namespace cl;

#define N 1024

void h_matrixmul(vector<double> A, vector<double> B, vector<double> C) {
	int i,j, k;

	vector<double> ACopy(A);
	vector<double> BCopy(B);
	vector<double> CCopy(C);

	for(i=0; i<N; ++i)
	{
		for(j=0; j<N; ++j)
		{
			double temp = 0.0;
			for(k=0; k<N; ++k)
			{
				temp += BCopy[i * N + k] * CCopy[k * N + j];
			}

			ACopy[i * N + j] = temp;
		}
	}
}

void d_matrixmul(vector<double> A, vector<double> B, vector<double> C) {
	Context context(CL_DEVICE_TYPE_DEFAULT);

	vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
	Program program(context, util::loadProgram("kernels.cl"));
	CommandQueue queue(context);

	try {
		program.build();
		auto mmul = cl::make_kernel<int, Buffer, Buffer, Buffer, LocalSpaceArg>(program, "matrixmul");

		// Use the Draconion naming convention of d_* for declaring device memory
		Buffer d_A, d_B, d_C;
		LocalSpaceArg CWork = Local(sizeof(float) * N);

		d_A = Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * N * N);
		d_B = Buffer(context, begin(B), end(B), true);
		d_C = Buffer(context, begin(C), end(C), true);

		printf("Launching Kernel!\n");

		// Launch the specified kernel with the specified dimensions and arguments
		mmul(EnqueueArgs(queue, NDRange(N), NDRange(64)), N, d_A, d_B, d_C, CWork);

		queue.finish();

		// Once done we can copy the data from the operation
		cl::copy(queue, d_C, begin(C), end(C));	
	} 
	catch (cl::Error err) 
	{
		if(err.err() == CL_BUILD_PROGRAM_FAILURE) {
			string build_error = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
			std::cout << "Error compiling Kernel code: " << build_error << endl;
		}
		else {
	    	std::cout << "OpenCL Error: " << err.what() << " returned " << err.err() << std::endl;
	    	std::cout << "Check cl.h for error codes." << std::endl;
	    	exit(-1);
		}
	}
}

// We Will be calculating the equation A = B * C

int main(int argc, char *argv[]) {
	util::Timer timer;
	double rtime;	

	printf("Starting...\n");

	// We will be calculating calculations of square NxN matrices
	vector<double> A(N * N);
	vector<double> B(N * N);
	vector<double> C(N * N);

	// Initialise Matrices with random data
	for(int i=0; i < N * N; ++i)
	{
		B[i] = rand() % 100;
		C[i] = rand() % 100;
	}

	// HOST CPU MATRIX MULTIPLY
	timer.reset();
	printf("Performing matrix multiply on the host...\n");
	h_matrixmul(A, B, C);
	rtime = static_cast<double>(timer.getTimeMilliseconds() / 1000.0);		
	printf("Host computation complete in %f seconds\n", rtime);

	// DEVICE GPU MATRIX MULTIPLY
	timer.reset();
	printf("Performing matrix multiply on the device gpu...\n");
	d_matrixmul(A, B, C);
	rtime = static_cast<double>(timer.getTimeMilliseconds() / 1000.0);
	printf("The Kernels finished in %f seconds\n", rtime);
	
	printf("Success!\n");
}
