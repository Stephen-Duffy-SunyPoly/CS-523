//Author: Stephen Duffy duffysd
//General assignment utility
//CS523 4:00pm tr
#pragma once

#include <mpi.h>

class ProcessInfo {
    //volatile so c++ will not think they never change
    volatile int rank = -1;
    volatile int numProcesses = -1;
public:
    ProcessInfo() {
        MPI_Comm_size( MPI_COMM_WORLD, const_cast<int *>(&numProcesses) );//get the number of parallel processes running
        MPI_Comm_rank( MPI_COMM_WORLD, const_cast<int *>(&rank) );//get the id of this process
    }

    [[nodiscard]] int getRank() const {
        return rank;
    }

    [[nodiscard]] int getNumProcesses() const {
        return numProcesses;
    }

    [[nodiscard]] bool isMain() const{
        return getRank() == 0;
    }
};