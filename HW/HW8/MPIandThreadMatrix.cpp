//Author: Stephen Duffy duffysd
//HW8_MPI_Pthreads
//CS523 4:00pm tr
#include <iostream>
#include <mpi.h>
#include <pthread.h>
#include "../HW3/mpi_infra.hpp"
#include <vector>

using namespace std;

struct SimpleMatrix {
    size_t rows=0;
    size_t cols=0;
    vector<float> data;
    SimpleMatrix(const size_t rows, const size_t cols) {
        this->rows = rows;
        this->cols = cols;
        data.resize(rows * cols);
    }
    SimpleMatrix() = default;
    [[nodiscard]] size_t index(const size_t row, const size_t col) const {
        return row * cols + col;
    }
};

struct ThreadMatrixData {
    SimpleMatrix * matrixA;
    SimpleMatrix * matrixB;
    SimpleMatrix * matrixC;
    int startRow;
    int endRow;
    pthread_t thread;
};

void * matrixThreadFunction(void * matData) {
    ThreadMatrixData * data = static_cast<ThreadMatrixData *>(matData);
    vector<float> localRowCache(data->matrixC->cols * (data->endRow - data->startRow),0.0f);
    for (int i = data->startRow; i < data->endRow; i++) {//mat 1 rows
        for (int mat2Col = 0; mat2Col < data->matrixB->cols; mat2Col++) {
            for (int cell = 0; cell < data->matrixB->rows; cell++) {
                localRowCache[(i-data->startRow)*data->matrixC->cols + mat2Col] +=
                    data->matrixA->data[data->matrixA->index(i,cell)] *
                        data->matrixB->data[data->matrixB->index(cell,mat2Col)];
            }
        }
    }
    size_t dataStart =data->matrixC->index(data->startRow,0);
    for (size_t i = dataStart; i < data->matrixC->index(data->endRow,0); i++) {
        data->matrixC->data[i] = localRowCache[i-dataStart];
    }
    return nullptr;
}

ostream & operator<<(ostream & out, const SimpleMatrix & mat) {
    for (size_t row = 0; row < mat.rows; row++) {
        for (size_t col = 0; col < mat.cols; col++) {
            const float val = mat.data[mat.index(row,col)];
            out << val << " ";
        }
        out << endl;
    }

    return out;
}

int main(int argc, char ** argv) {
    //Write a Matrix/matrix multiply program using multiple MPI nodes and threads within nodes

    MPI_Init(&argc, &argv);
    ProcessInfo processInfo;

    SimpleMatrix mat1, mat2, tmp;
    int mat1Rows, mat1Cols;
    int mat2Rows, mat2Cols;

    //read in both matrix
    // ReSharper disable once CppDFAConstantConditions
    if (processInfo.isMain()) {
        // ReSharper disable once CppDFAUnreachableCode
        cout << "enter matrix 1 size (row X col): ";

        cin >> mat1Rows >> mat1Cols;
        if (mat1Rows == 0 || mat1Cols == 0) {
            cerr << "ERROR: 0 dimensions not allowed (1)"<< endl;
            return EXIT_FAILURE;
        }

        mat1 = SimpleMatrix(mat1Rows, mat1Cols);
        tmp = SimpleMatrix(mat1Rows, mat1Cols);

        for (float & i : tmp.data) {
            float value;
            cin >> value;
            i = value;
        }

        cout << "enter matrix 2 size (row X col): ";
        cin >> mat2Rows >> mat2Cols;
        if (mat1Rows == 0 || mat1Cols == 0) {
            cerr << "ERROR: 0 dimensions not allowed (2)"<< endl;
            return EXIT_FAILURE;
        }
        mat2 = SimpleMatrix(mat2Rows, mat2Cols);
        for (float & i : mat2.data) {
            float value;
            cin >> value;
            i = value;
        }
        // cout <<"counts: "<<mat1Rows <<" "<< mat1Cols <<" "<< mat2Rows <<" "<< mat2Cols<< endl;
    }
    // cout << "prebdcast: " <<processInfo.getRank()<<endl;
    MPI_Bcast(&mat1Rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mat1Cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mat2Rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&mat2Cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // cout << "post bdcast: " <<processInfo.getRank() << " | " <<mat1Rows <<" "<< mat1Cols <<" "<< mat2Rows <<" "<< mat2Cols<< endl;
    if (!processInfo.isMain()) {
        mat1 = SimpleMatrix(mat1Rows, mat1Cols);
        mat2 = SimpleMatrix(mat2Rows, mat2Cols);
        // cout << "post alt alocs: " <<processInfo.getRank()<<endl;
    }
    //send all of matrix 2 to all nodes
    MPI_Bcast(mat2.data.data(), static_cast<int>(mat2.data.size()), MPI_FLOAT, 0, MPI_COMM_WORLD);
    // cout << "post mat 2 bdcast: " <<processInfo.getRank() <<endl;
    int rowsPerProcess = static_cast<int>(mat1.rows)/processInfo.getNumProcesses();
    //split the rows of the first matrix to all nodes,
    MPI_Scatter(processInfo.isMain() ? tmp.data.data() : nullptr,rowsPerProcess * static_cast<int>(mat1Cols), MPI_FLOAT, mat1.data.data(), rowsPerProcess * static_cast<int>(mat1Cols), MPI_FLOAT, 0, MPI_COMM_WORLD);
    // cout << "post mat 1 scatter: " <<processInfo.getRank() << endl;
    MPI_Barrier(MPI_COMM_WORLD);

    cout << "start 1 " <<processInfo.getRank() << " " <<mat2Cols<< endl;
    SimpleMatrix resultMatrix(rowsPerProcess,mat2.cols);
    //thread in each node compute parts of the assigned rows
    vector<ThreadMatrixData> threadData;
    for (int i=0;i<rowsPerProcess;i++) {
        ThreadMatrixData data;
        data.matrixA = &mat1;
        data.matrixB = &mat2;
        data.matrixC = &resultMatrix;
        data.startRow = i;
        data.endRow = i+1;
        threadData.push_back(data);
    }

    for (size_t i = 0; i < threadData.size(); i++) {
        pthread_create(&threadData[i].thread, nullptr, matrixThreadFunction, &threadData[i]);
    }

    for (size_t i = 0; i < threadData.size(); i++) {
        pthread_join(threadData[i].thread, nullptr);
    }

    SimpleMatrix allResultMatrix(mat1.rows,resultMatrix.cols);
    MPI_Gather(resultMatrix.data.data(),rowsPerProcess * mat2Cols, MPI_FLOAT, allResultMatrix.data.data(), rowsPerProcess * mat2Cols, MPI_FLOAT, 0, MPI_COMM_WORLD);
    // cout << "post gather " << processInfo.getRank() << endl;
    MPI_Barrier(MPI_COMM_WORLD);

    if (processInfo.isMain()) {
        cout << "Result: " << endl;
        cout << allResultMatrix << endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
