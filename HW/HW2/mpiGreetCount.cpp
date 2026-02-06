//Author: Stephen Duffy duffysd
//HW2_Greet_Count
//CS523 4:00pm tr

#include <iostream>
#include <mpi.h>
#include <mpe.h>
#include "MpeHelper.hpp"

using namespace std;

int main(int argc, char **argv) {

    int number_of_processors;
    int thisProcessor;
    int nameLength;
    char processor_name_c[ MPI_MAX_PROCESSOR_NAME ];

    MPI_Init( &argc, &argv ); //initialize MPI passing the commandline args

    MPI_Pcontrol( 0 );//set the profiling control level to 0 this value is sent to the profiling library

    MPI_Comm_size( MPI_COMM_WORLD, &number_of_processors );//get the number of parallel processes running
    MPI_Comm_rank( MPI_COMM_WORLD, &thisProcessor );//get the id of this process

    if( thisProcessor == 0 ) {
        // cerr << "Running with " << number_of_processors << " processors." << endl;
    }

    MPI_Get_processor_name( processor_name_c, &nameLength );
    //create the much better c++ version of the name string
    string processor_name = processor_name_c;

    // cerr << "Process " << thisProcessor << " running on " << processor_name << endl;

    initMPE();
    MpeTimedEvent sendingEvent(thisProcessor,"sending",GREEN);
    MpeTimedEvent receiveEvent(thisProcessor,"receiving",RED);
    MpeTimedEvent syncEvent(thisProcessor,"sync",ORANGE);


    string greetingMessage = "Greetings from rank ";
    greetingMessage += to_string(thisProcessor);
    int destRank = thisProcessor+1;
    if (destRank == number_of_processors) {
        destRank = 0;
    }

    int mpiBufferSize =MPI_BSEND_OVERHEAD+greetingMessage.length()+1;
    char * mpiAttachedBuffer = static_cast<char *>(malloc(mpiBufferSize));
    char receiveBuffer[100]{};
    MPI_Buffer_attach(mpiAttachedBuffer, mpiBufferSize);

    MPI_Barrier( MPI_COMM_WORLD );//sync all the processes here
    startMPE();


    //if process 0 send the message to process 1
    if( thisProcessor == 0 ) {
        sendingEvent.start();
        //no idea what the tag should be or what it is for
        MPI_Bsend(greetingMessage.c_str(), static_cast<int>(greetingMessage.length()), MPI_CHAR,destRank,0,MPI_COMM_WORLD);//buffered send

        sendingEvent.end();
        //then wait for a message from the last processor
        //MPI_Recv(revieveBuffer,100, MPI_CHAR,MPI_ANY_SOURCE , MPI_ANY_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        receiveEvent.start();
        MPI_Recv(receiveBuffer,100, MPI_CHAR,number_of_processors-1 , MPI_ANY_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        string receivedMessage = receiveBuffer;
        cout << receivedMessage << ", received by rank "<<thisProcessor<<endl;
        receiveEvent.end();
    } else {
        //if not process 0 wait to revieve a message from the prevous processor
        receiveEvent.start();
        MPI_Recv(receiveBuffer,100, MPI_CHAR,thisProcessor-1 , MPI_ANY_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        string receivedMessage = receiveBuffer;
        cout << receivedMessage << ", received by rank "<<thisProcessor<<endl;
        receiveEvent.end();
        //then send a message to the next processor

        sendingEvent.start();
        MPI_Bsend(greetingMessage.c_str(), static_cast<int>(greetingMessage.length()), MPI_CHAR,destRank,0,MPI_COMM_WORLD);
        sendingEvent.end();
    }

    syncEvent.start();
    MPI_Barrier( MPI_COMM_WORLD );
    syncEvent.end();

    MpeTimedEvent::sync();//sync clocks

    MPE_Finish_log( argv[0] );

    MPI_Buffer_detach(mpiAttachedBuffer, &mpiBufferSize);

    free(mpiAttachedBuffer);

    MPI_Finalize();

    return EXIT_SUCCESS;
}