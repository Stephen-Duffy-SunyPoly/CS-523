//Author: Stephen Duffy duffysd
//HW6_Pthreads_MatrixMatrix
//CS523 4:00pm tr
#include <iostream>
#include "pthread.h"
#include <vector>

using namespace std;

void * parallelMatrixThread(void *);

class ParallelMatrix {
    vector<float> cells;
    int rows;
    int cols;
    public:
    [[nodiscard]] int matIndex (int row, int col) const {
        return row * cols + col;
    }

    ParallelMatrix(int rows,int cols): rows(rows),cols(cols) {
        cells.resize(rows*cols);
    }

    ParallelMatrix(const string& prompt,int rows,int cols): rows(rows),cols(cols) {
        cells.resize(rows*cols);
        cout << prompt << " ("<<rows << "X"<< cols <<")"<<endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                float val;
                cin >> val;
                cells[matIndex(i,j)] = val;
            }
        }
    }

    //this weird struct allow for the double [][] operation to be valid syntax and return the result wanted
    struct matrixPatialReturnVal{
        int i;
        ParallelMatrix & mat;
        float& operator[](int j) {
            return mat.cells[mat.matIndex(i,j)];
        }
    };
    matrixPatialReturnVal operator[](int i) {
        return {i,*this};
    }

    //this one is defined later so we can use a struct defined later
    ParallelMatrix operator*(ParallelMatrix &other);

    [[nodiscard]] int getRows() const {
        return rows;
    }

    [[nodiscard]] int getCols() const {
        return cols;
    }

};

struct threadInfo{
    int threadID;
    ParallelMatrix &in1;
    ParallelMatrix &in2;
    ParallelMatrix &result;
    pthread_t thread;
};

//fancy function to make printing the matrix to the output stream eazy
ostream& operator << (ostream &os,ParallelMatrix &matrix) {

    for (int row=0;row<matrix.getRows();row++) {
        for (int col=0;col<matrix.getCols();col++) {
            cout << matrix[row][col] << " ";
        }
        cout << endl;
    }

    return os;
}

int main () {

    cout << "enter matrix 1 size (rows X cols):";
    int mat1rows;
    int mat1cols;
    cin >> mat1rows >> mat1cols;
    ParallelMatrix mat1("Enter matrix 1:",mat1rows,mat1cols);

    cout << "enter matrix 2 size (rows X cols):";
    int mat2rows;
    int mat2cols;
    cin >> mat2rows >> mat2cols;

    ParallelMatrix mat2("Enter matrix 2:",mat2rows,mat2cols);

    ParallelMatrix result = mat1 * mat2;

    cout << "result: " << endl << result;

    return EXIT_SUCCESS;
}

ParallelMatrix ParallelMatrix::operator*(ParallelMatrix &other) {
    if (cols !=  other.rows) {
        throw runtime_error("Matrix inner dimensions do not match!");
    }
    ParallelMatrix result(rows,other.cols);
    vector<threadInfo> threads;
    //create 1 thread per row
    threads.reserve(rows);
    //setup the data for each thread
    for (int i = 0; i < rows; i++) {
        threads.push_back({static_cast<int>(threads.size()),*this,other,result,{}});
    }
    //start each thread
    for (auto & thread : threads) {
        pthread_create(&thread.thread,nullptr,parallelMatrixThread,&thread);
    }

    //wait for each thread to finish
    for (auto & thread : threads) {
        pthread_join(thread.thread,nullptr);
    }

    return result;
}

void * parallelMatrixThread(void * in) {
    auto * info = static_cast<threadInfo *>(in);
    vector<float> localRowCache;
    localRowCache.resize(info->in1.getCols());
    int mat1Row = info ->threadID;
    for (int mat2Col = 0 ;mat2Col < info->in2.getCols();mat2Col++) {
        for (int cell = 0; cell < info->in2.getRows();cell++) {
            localRowCache[mat2Col] += info->in1[mat1Row][cell] * info->in2[cell][mat2Col];
        }
    }

    //copy the local cache into the result
    for (int i=0;i<localRowCache.size();i++) {
        info->result[mat1Row][i] = localRowCache[i];
    }


    return nullptr;
}
