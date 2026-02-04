//Author: Stephen Duffy duffysd
//HW2 pthread count 3s
//CS523 4:00pm tr
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>
#include <pthread.h>
#include "Mutex.h"

#define arraySize 1000000000

using namespace std;

struct ThreadInfo {
    int threadId;
    size_t numberOfThreads;
};

vector<int> inputArray;//use a vector for this to allow much greater ammount of content
int count=0;
Mutex countLock;

//function to generate a random array containing all the numbers to check
void initArray() {
    inputArray.reserve(arraySize);
    srand (time(nullptr));
    for (int i = 0; i < arraySize; i++) {
        inputArray.push_back(rand() % 100);
    }
}

void * countingThread(void *);

int main() {
    cout << "generating numbers" << endl;
    initArray();
    //uncomment if you want to se what gets generated
    // cout << "generated array: ";
    // for (const int i : inputArray) {
    //     cout << i << " ";
    // }
    // cout << endl;

    const size_t numThreads = 4;// the number of threads to use
    cout << "Using " << numThreads << " threads" << endl;
    cout.flush();

    vector<pthread_t> threads;
    vector<ThreadInfo> threadInfos;
    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; i++) {//initalize the vectors with pthread objects and info objects
        threads.push_back(pthread_t());
        threadInfos.push_back({i,numThreads});
    }

    //start each thread
    for (size_t i = 0; i < threads.size(); i++) {
        if (int error = pthread_create(&threads[i],nullptr,countingThread,&threadInfos[i]); error != 0) {
            cerr << "Failed to create thread: " << strerror(error) << endl;
            throw runtime_error("Failed to create thread");
        }
    }

    //wait for each thread to finish
    for (const pthread_t thread : threads) {
        pthread_join(thread,nullptr);
    }

    //print the number of 3s found
    cout << "Total 3s found: "<< count << endl;
    cout << "Total numbers checked: "<< arraySize << endl;


    return EXIT_SUCCESS;
}

void *countingThread(void * infov) {
    const ThreadInfo * info = (ThreadInfo *) infov;
    //calculate the start index for this thread
    const int start = arraySize / info->numberOfThreads * info->threadId;
    //calculae the end index for this thread
    int end = start + (int)(arraySize/info->numberOfThreads);
    if (info->threadId == info->numberOfThreads-1) {
        end = arraySize;
    }
    int localCount = 0;
    for (int i = start; i < end; i++) {//counr the 3s in this section
        if (inputArray[i] == 3) {
            localCount++;//store the count in a local varible
        }
    }
    countLock.lock();//obtian the mutex lock so only this thread can wright to the var
    count += localCount;//add my total to the overall total
    countLock.unlock();//relaese the loc
    return nullptr;
}
