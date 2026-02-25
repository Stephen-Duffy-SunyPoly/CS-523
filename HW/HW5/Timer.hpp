//This class was created for the code timeing assignment in CS 330
#pragma once

#include <sys/time.h>
#include <iostream>

//a class to hold all the timer things
class Timer{//say helo to my good old fried the timer
    struct timeval starTime{};
    struct timeval endtTime{};
public:
    //prototype for required functions
    void startTimer(){
        //save the current time as the start time
        gettimeofday(&starTime,nullptr);
    }
    void endTimer(){
        //save the current time the end time
        gettimeofday(&endtTime,nullptr);
    }
    void printTimer(){
        //convert everything to micro seconds and store it in a 64bit integer to make sure we don't loose any data
        long long totalStartMicroSeconds = starTime.tv_usec;
        totalStartMicroSeconds += (long long)starTime.tv_sec * 1000000;

        long long totalEndMicroSeconds = endtTime.tv_usec;
        totalEndMicroSeconds += (long long)endtTime.tv_sec * 1000000;
        //calculate the difference
        long long totalElapsedMicroSeconds = totalEndMicroSeconds - totalStartMicroSeconds;
        //keep the seconds as 64bit incase you want to time something for over 65 years
        long long elapsedSeconds = totalElapsedMicroSeconds / 1000000;
        //micro seconds are bounded at 1000000 so they can say as 32 bit
        long elapsedMicroSeconds = (long)(totalElapsedMicroSeconds % 1000000);

        //print the values
        std::cout << "Time Elapsed: " << elapsedSeconds << "s " << elapsedMicroSeconds << "us " << std::endl;
    }
};