#include <iostream>
#include <vector>
#include <mpi.h>
#include "../HW3/mpi_infra.hpp" // MPI infrastructure from HW 2

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

struct LocalMatrix {
    vector<float> matrix;
    int cols;
    int rows;
    LocalMatrix(int numCols,int numRows) : cols(numCols), rows(numRows){
        matrix.resize(numRows*numCols);
    }

    [[nodiscard]] size_t matIndex(const size_t rowNum, const size_t colNum) const {
        return rowNum * cols + colNum;
    }
};

class DistributedMatrix {
    vector<float> matrix;
    int rows, cols;
    int localRows;
    int totalProcesses;

    //copy constructor for convenience later
    DistributedMatrix(const int numRows, const int numCols, const int distributedRows, const vector<float> &localMatrix, int totalProcesses) {
        matrix.resize(numRows*numCols);
        for (size_t i = 0; i < localMatrix.size(); i++) {
            matrix[i] = localMatrix[i];
        }
        rows = numRows;
        cols = numCols;
        localRows = distributedRows;
        this->totalProcesses = totalProcesses;
    }

public:
    [[nodiscard]] size_t matIndex(const size_t rowNum, const size_t colNum) const {
        return rowNum * cols + colNum;
    }

    DistributedMatrix(const string &prompt, const int numRows, const int numCols,const int distributedRows, const ProcessInfo &process_info) {
        rows = numRows;
        cols = numCols;
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
        MPI_Scatter(tmpMatrix.data(),distributedRows*numCols,MPI_FLOAT,matrix.data(),distributedRows*numCols,MPI_FLOAT,0, MPI_COMM_WORLD);

        //should be all synced now
    }

    //sync the entire matrix to a single thread
    DistributedMatrix syncSingle(const int rootRank) {
        vector<float> tmpMatrix(rows*cols);
        tmpMatrix.resize(rows*cols);
        MPI_Gather(matrix.data(),localRows*cols,MPI_FLOAT,tmpMatrix.data(),localRows*cols,MPI_FLOAT,rootRank,MPI_COMM_WORLD);
        // the temp matrix should contain all the combined matrix data
        return {rows,cols,localRows,tmpMatrix,totalProcesses};//create a new matrix to return
    }

    /**Sync this entire matrix to all threads
     *@return a new matrix copy that contains all matrix data
     */
    LocalMatrix syncAll() const {
        LocalMatrix matOut(cols,rows);
        MPI_Allgather(matrix.data(),localRows*cols,MPI_FLOAT,matOut.matrix.data(),localRows*cols,MPI_FLOAT,MPI_COMM_WORLD);
        return matOut;
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

    //matrix vector multiplication
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

    //matrix matrix multiplication
    DistributedMatrix operator*(const LocalMatrix &matrixIn) {
        //verify correct sizes
        if (cols != matrixIn.rows) {
            throw runtime_error("Matrix dimensions do not match! "+std::to_string(rows)+"X"+std::to_string(cols)+" * "+std::to_string(matrixIn.rows)+"X"+std::to_string(matrixIn.cols));
        }
        vector<float> tmpMatrix(rows*matrixIn.cols);
        tmpMatrix.resize(rows*matrixIn.cols);
        //parallel matrix on local matrix multiply
        //i = mat 1 row
        //j = mat 1 col / mat 2 row
        //k = mat 2 col
        //dest = [i][k]
        for (size_t i=0;i<localRows;i++) {
            for (size_t k=0;k<matrixIn.cols;k++) {
                for (size_t j=0;j<cols;j++) {
                    tmpMatrix[i * matrixIn.cols + k] += matrix[matIndex(i,j)] * matrixIn.matrix[matrixIn.matIndex(j,k)];
                }
            }
        }

        return {rows,matrixIn.cols,localRows,tmpMatrix,totalProcesses};
    }

    //matrix matrix multiplication
    DistributedMatrix operator*(const DistributedMatrix &matrixIn) {
        //verify correct sizes
        if (cols != matrixIn.rows) {
            throw runtime_error("Matrix dimensions do not match! "+std::to_string(rows)+"X"+std::to_string(cols)+" * "+std::to_string(matrixIn.rows)+"X"+std::to_string(matrixIn.cols));
        }
        //get the entire other matrix on all threads
        LocalMatrix otherMatrix = matrixIn.syncAll();
        //multiply with that
        return this[0] * otherMatrix;//isn't this just the best C++ syntax you have ever scene?
    }


};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const ProcessInfo processInfo;//get the general process info

    int matrix1Rows, matrix1Cols;
    int matrix2Rows, matrix2Cols;

    if (processInfo.isMain()) {//main thread setup
        cout << "Enter the size of the matrix 1 (rows x cols):";
        //read in the matrix size from the user
        cin >> matrix1Rows >> matrix1Cols;

        //warn if entered rows or cols is not evenly dividable by the number of processes
        if (matrix1Rows % processInfo.getNumProcesses() != 0) {
            cerr << "WARNING: the number of matrix rows is not evenly dividable by the number of processes" << endl;
        }
        if (matrix1Cols % processInfo.getNumProcesses() != 0) {
            cerr << "WARNING: the number of matrix cols is not evenly dividable by the number of processes" << endl;
        }

        cout << "Enter the size of the matrix 2 (rows x cols):";
        //read in the matrix size from the user
        cin >> matrix2Rows >> matrix2Cols;

        //warn if entered rows or cols is not evenly dividable by the number of processes
        if (matrix2Rows % processInfo.getNumProcesses() != 0) {
            cerr << "WARNING: the number of matrix rows is not evenly dividable by the number of processes" << endl;
        }
        if (matrix2Cols % processInfo.getNumProcesses() != 0) {
            cerr << "WARNING: the number of matrix cols is not evenly dividable by the number of processes" << endl;
        }
    }

    //send the data from the main thread and load it on the other threads
    MPI_Bcast(&matrix1Rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&matrix1Cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //and the other matrix
    MPI_Bcast(&matrix2Rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&matrix2Cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int perThreadRows1 = matrix1Rows / processInfo.getNumProcesses();
    int perThreadRows2 = matrix2Rows / processInfo.getNumProcesses();

    MPI_Barrier( MPI_COMM_WORLD );//sync all threads

    DistributedMatrix matrix1("Enter matrix 1",matrix1Rows,matrix1Cols,perThreadRows1,processInfo);
    DistributedMatrix matrix2("Enter matrix 2",matrix1Rows,matrix1Cols,perThreadRows2,processInfo);

    DistributedMatrix result = matrix1 * matrix2;

    if (processInfo.isMain()) {
        cout << "Result: "<< endl;
    }
    result.printMatrix(processInfo);

    MPI_Barrier( MPI_COMM_WORLD );//sync all threads

    MPI_Finalize();
    return EXIT_SUCCESS;
}