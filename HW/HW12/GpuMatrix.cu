//Author: Stephen Duffy duffysd
//HW12_GPU_Matrix_Multiply
//CS523 4:00pm tr
#include <iostream>
#include "Timer.hpp" //once again say hello to our good friend the timer class from CS 330

using namespace std;

__global__ void gpuMatrixKernel(float * matA, float * matB, float * resultMat, int aRows, int innerDim,int bCols) {
    //calculate the output row and col from the thread and block id
    int resultIndex = threadIdx.x + blockDim.x * blockIdx.x;//threadIdx, blockDim, blockIdx

    int resultRow = resultIndex / aRows;
    int resultCol = resultIndex % aRows;

    float accumulator = 0;
    for (int i=0;i<innerDim;i++) {
        int indexA = resultRow * aRows + i;
        int indexB = i*bCols + resultCol;

        accumulator += matA[indexA] * matB[indexB];
    }
    resultMat[resultIndex] = accumulator;
}

void initRandomMat(float* mat, int size) {
    for (int i=0;i < size;i++) {
        mat[i] = (float)(rand() % 1000-500)/500.0f;
    }
}

void doMultiplyWithParams(int blocks, int threadsPerBlock) {
    if (blocks * threadsPerBlock != 128*128) {
        throw std::invalid_argument("Blocs * threads must equal 128*128");
    }
    //allocate the 3 mats
    float * matA = nullptr, * matB = nullptr, * resultMat = nullptr;
    cudaMallocManaged(&matA, blocks*threadsPerBlock*sizeof(float));
    cudaMallocManaged(&matB, blocks*threadsPerBlock*sizeof(float));
    cudaMallocManaged(&resultMat, blocks*threadsPerBlock*sizeof(float));

    //random the first 2
    initRandomMat(matA,128*128);
    initRandomMat(matB,128*128);

    //do the multiplication
    gpuMatrixKernel<<<blocks,threadsPerBlock>>>(matA,matB,resultMat,128,128,128);
    cudaDeviceSynchronize();

    cudaFree(matA);
    cudaFree(matB);
    cudaFree(resultMat);
}

int main() {
    cout << "Executing warmup rounds"<< endl;
    doMultiplyWithParams(128,128);
    doMultiplyWithParams(128,128);
    doMultiplyWithParams(128,128);
    doMultiplyWithParams(128,128);
    //start of actual testing
    Timer gpuMatrixTimer;

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(4,4096);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 4 Blocks, 4096 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(8,2048);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 8 Blocks, 2048 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(16,1024);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 16 Blocks, 1024 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(32,512);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 32 Blocks, 512 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(64,256);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 64 Blocks, 256 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(128,128);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 128 Blocks, 128 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(256,64);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 256 Blocks, 64 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(512,32);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 512 Blocks, 32 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(1024,16);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 1024 Blocks, 64 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(2048,8);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 2048 Blocks, 8 threads" << endl;
    gpuMatrixTimer.printTimer();

    gpuMatrixTimer.startTimer();
    doMultiplyWithParams(4096,4);
    gpuMatrixTimer.endTimer();
    cout << "multiply with: 4096 Blocks, 4 threads" << endl;
    gpuMatrixTimer.printTimer();

    cout << "Done" << endl;
}