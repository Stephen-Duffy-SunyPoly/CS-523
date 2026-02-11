//Author: Stephen Duffy duffysd
//HW3_MPI_MatrixVector
//CS523 4:00pm tr

#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <mpi.h>
#include "mpi_infra.hpp"

using namespace std;

class DistributedVector {
    vector<float> elements;
    int numProcess;
public:
    DistributedVector(const string &prompt,int size, const ProcessInfo &process_info) {
        numProcess = process_info.getNumProcesses();
        //the size is the number of total columns
        elements.resize(size);
        if (process_info.isMain()) {
            cout << prompt << " ("<<size<<")"<<endl;
            for (size_t i = 0; i < size; i++) {
                cin >> elements[i];
            }
            cout << endl;
        }
        //broadcast the vector to each thread, they all need the whole thing to do their calculations so might as well just send it now
        MPI_Bcast(elements.data(),size,MPI_FLOAT,0,MPI_COMM_WORLD);
    }

    DistributedVector(vector<float> data, int totalProcess) {
        elements = std::move(data);
        numProcess = totalProcess;
    }

    DistributedVector sync(int rootRank) {
        //create a tmp dest buffer
        vector<float> tmpVector(elements.size());
        tmpVector.resize(elements.size());
        MPI_Gather(elements.data(),static_cast<int>(elements.size())/numProcess,MPI_FLOAT,tmpVector.data(),static_cast<int>(elements.size())/numProcess,MPI_FLOAT,rootRank,MPI_COMM_WORLD);
        return {tmpVector,numProcess};//return a new vector from the cobined data
    }

    void printVector(bool syncDist, const ProcessInfo &process_info) {
        if (syncDist) {
            DistributedVector synedVector = sync(0);
            if (process_info.isMain()) {
                for (size_t i=0;i<synedVector.elements.size();i++) {
                    cout << synedVector.elements[i] << " ";
                }
                cout << endl;
            }
        } else {
            if (process_info.isMain()) {
                for (size_t i=0;i<elements.size();i++) {
                    cout << elements[i] << " ";
                }
                cout << endl;
            }
        }
    }

    float operator[](const size_t i) const {
        return elements[i];
    }
};

class DistributedMatrix {
    vector<float> matrix;
    int rows, cols;
    int localCols;
    int localRows;
    int totalProcesses;

    //copy constructor for convenience later
    DistributedMatrix(const int numRows, const int numCols, const int distributedCols, const int distributedRows, const vector<float> &localMatrix, int totalProcesses) {
        matrix.resize(numRows*numCols);
        for (size_t i = 0; i < localMatrix.size(); i++) {
            matrix[i] = localMatrix[i];
        }
        rows = numRows;
        cols = numCols;
        localCols = distributedCols;
        localRows = distributedRows;
        this->totalProcesses = totalProcesses;
    }

public:
    [[nodiscard]] size_t matIndex(const size_t rowNum, const size_t colNum) const {
        return rowNum * cols + colNum;
    }

    DistributedMatrix(const string &prompt, const int numRows, const int numCols, const int distributedCols,const int distributedRows, const ProcessInfo &process_info) {
        rows = numRows;
        cols = numCols;
        localCols = distributedCols;
        localRows = distributedRows;
        totalProcesses = process_info.getNumProcesses();
        //initialize the actual matrix values with 0 / put the vectors at capacity
        matrix.resize(numRows*numCols);
        vector<float> tmpMatrix(numRows*numCols);
        tmpMatrix.resize(numRows*numCols);

        if (process_info.isMain()) {//if on the main thread
            //read in the matrix form the user
            cout << prompt << " ("<<numRows << "X"<<numCols<<")"<<endl;
            for (size_t i = 0; i < rows; i++) {
                for (size_t j = 0; j < cols; j++) {
                    float val;
                    cin >> val;
                    tmpMatrix[matIndex(i,j)] = val;
                }
            }
        }

        //distribute the matrix to each thread
        MPI_Scatter(tmpMatrix.data(),distributedCols*numRows,MPI_FLOAT,matrix.data(),distributedCols*numRows,MPI_FLOAT,0, MPI_COMM_WORLD);

        //should be all synced now
    }

    //sync the entire matrix to a single thread
    DistributedMatrix syncSingle(const int rootRank) {
        vector<float> tmpMatrix(rows*cols);
        tmpMatrix.resize(rows*cols);
        MPI_Gather(matrix.data(),localCols*rows,MPI_FLOAT,tmpMatrix.data(),localCols*rows,MPI_FLOAT,rootRank,MPI_COMM_WORLD);
        // the temp matrix should contain all the combined matrix data
        return {rows,cols,localCols,localRows,tmpMatrix,totalProcesses};//create a new matrix to return
    }

    void printMatrix(const ProcessInfo &process_info) {
        const DistributedMatrix fullMatrix = syncSingle(0);
        if (process_info.isMain()) {
            for (size_t i=0;i<rows;i++) {
                for (size_t j=0;j<cols;j++) {
                    //TODO formatting for number of decimal points
                    cout << fullMatrix.matrix[matIndex(i,j)] << " ";
                }
                cout << endl;
            }
        }

    }

    DistributedVector operator*(const DistributedVector &vectorIn) {
        vector<float> tmpVec(rows);
        tmpVec.resize(rows);
        //parallel matrix vector product!
        for (size_t i=0;i<localRows;i++) {
            for (size_t j=0;j<cols;j++) {
                tmpVec[i] += matrix[matIndex(i,j)]*vectorIn[j];//accumulate the element value
            }
        }
        return {tmpVec,totalProcesses};
    }


};


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    const ProcessInfo processInfo;//get the general process info

    int matrixRows, matrixCols;

    if (processInfo.isMain()) {//main thread setup
        cout << "Enter the size of the matrix (rows x cols):";
        //read in the matrix size from the user
        cin >> matrixRows >> matrixCols;

        //warn if entered rows or cols is not evenly dividable by the number of processes
        if (matrixRows % processInfo.getNumProcesses() != 0) {
            cerr << "WARNING: the number of matrix rows is not evenly dividable by the number of processes" << endl;
        }
        if (matrixCols % processInfo.getNumProcesses() != 0) {
            cerr << "WARNING: the number of matrix cols is not evenly dividable by the number of processes" << endl;
        }
    }
    //send the data from the main thread and load it on the other threads
    MPI_Bcast(&matrixRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&matrixCols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int perThreadRows = matrixRows / processInfo.getNumProcesses();
    int perThreadCols = matrixCols / processInfo.getNumProcesses();

    MPI_Barrier( MPI_COMM_WORLD );//sync all threads

    //get the matrix from the user and distribute it to all the clients
    DistributedMatrix matrix("Enter the matrix",matrixRows,matrixCols,perThreadCols,perThreadRows,processInfo);

    if (processInfo.isMain()) {
        cout << "Read matrix:" << endl;
    }
    matrix.printMatrix(processInfo);

    //get the vector from the user and sync it to the clients
    DistributedVector inputVector("Enter vector",matrixCols,processInfo);
    if (processInfo.isMain()) {
        cout << "Read vector:" << endl;
    }
    inputVector.printVector(false,processInfo);

    //do the multiplication
    DistributedVector result = matrix * inputVector;//wow isent that nice and clean
    //too bad the implementing code feels like a mess

    if (processInfo.isMain()) {
        cout << "Result: "<< endl;
    }
    //make sure to use sync so we get the full result
    result.printVector(true,processInfo);

    MPI_Barrier( MPI_COMM_WORLD );//sync all threads

    MPI_Finalize();

    return EXIT_SUCCESS;
}