/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include <mpi.h>
#include <mpe.h>
#include <math.h>
#include <stdio.h>

double f( double );
double f( double a )
{
    return (4.0 / (1.0 + a*a));
}

int main( int argc, char *argv[] )
{
    int  n, myid, numprocs, ii, jj;
    double PI25DT = 3.141592653589793238462643;
    double mypi, pi, h, sum, x;
    double startwtime = 0.0, endwtime;
    int namelen;
    int event1a, event1b, event2a, event2b,
        event3a, event3b, event4a, event4b;
    int event1, event2, event3;
    char processor_name[ MPI_MAX_PROCESSOR_NAME ];

    MPI_Init( &argc, &argv ); //initialize MPI passing the commandline args

    MPI_Pcontrol( 0 );//set the profiling control level to 0 this value is sent to the profiling library

    //determines the size of the group associated with the communicator.
    MPI_Comm_size( MPI_COMM_WORLD, &numprocs );//get the number of parallel processes running
    MPI_Comm_rank( MPI_COMM_WORLD, &myid );//get the id of this process

    MPI_Get_processor_name( processor_name, &namelen );//get the name of this host (useful if using many machines at once)
    fprintf( stderr, "Process %d running on %s num procs: %d\n", myid, processor_name,numprocs );

    MPE_Init_log();//initialize the logging for jumpshot

    /*
        user should NOT assign eventIDs directly in MPE_Describe_state()
        Get the eventIDs for user-defined STATES(rectangles) from
        MPE_Log_get_state_eventIDs() instead of the deprecated function
        MPE_Log_get_event_number().
    */
    //create the event types
    MPE_Log_get_state_eventIDs( &event1a, &event1b );
    MPE_Log_get_state_eventIDs( &event2a, &event2b );
    MPE_Log_get_state_eventIDs( &event3a, &event3b );
    MPE_Log_get_state_eventIDs( &event4a, &event4b );

    if ( myid == 0 ) {//only do this on process 0
        //set the name and color of each segment that gets displayed on jump shot
        MPE_Describe_state( event1a, event1b, "Broadcast", "red" );
        MPE_Describe_state( event2a, event2b, "Sync", "orange" );
        MPE_Describe_state( event3a, event3b, "Compute", "blue" );
        MPE_Describe_state( event4a, event4b, "Reduce", "green" );
    }

    /* Get event ID for Solo-Event(single timestamp object) from MPE */
    MPE_Log_get_solo_eventID( &event1 );//events that  only happen at specific instances
    MPE_Log_get_solo_eventID( &event2 );
    MPE_Log_get_solo_eventID( &event3 );

    if ( myid == 0 ) {//only on process 0
        //set the names and colors for how each instant event will be displayed in jumpshot
       MPE_Describe_event( event1, "Broadcast Post", "white" );
       MPE_Describe_event( event2, "Compute Start", "purple" );
       MPE_Describe_event( event3, "Compute End", "navy" );
    }

    if ( myid == 0 ) {//on process 0
        n = 1000000;
        startwtime = MPI_Wtime();//get the elapsed time on the calling processor
    }
    MPI_Barrier( MPI_COMM_WORLD );//blocks on all processes until they all reach this point

    MPI_Pcontrol( 1 );//set the profiling level to 1
    MPE_Start_log();//start logging events for jump shot
    /*
    */

    //who made this and made theese horrible desisions
    for ( jj = 0; jj < 5; jj++ ) {
        MPE_Log_event( event1a, 0, NULL );//start event 1a the null here might be a comment string
        /**Broadcasts a message from the process with rank "root" to all other processes of the communicator
         @param buffer A pointer to the message data
         @param count the number of entries in the buffer
         @param datatype The data type of the buffer (handle)
         @param root The rank of the board cast root (the rank of the process that sends the message)
         @param comm The communcator (handle) what "universe" the message is being sent in
         */
        MPI_Bcast( &n, 1, MPI_INT, 0, MPI_COMM_WORLD );
        MPE_Log_event( event1b, 0, NULL );//mark end of event 1

        MPE_Log_event( event1, 0, NULL );//mark instant event 1

        MPE_Log_event( event2a, 0, NULL );//mark start of event 2
        MPI_Barrier( MPI_COMM_WORLD );//sync event
        MPE_Log_event( event2b, 0, NULL );//mark end of event 2

        MPE_Log_event( event2, 0, NULL );//mark instant event 2
        MPE_Log_event( event3a, 0, NULL );//mark start of event 3
        //do math
        h   = 1.0 / (double) n;
        sum = 0.0;
        for ( ii = myid + 1; ii <= n; ii += numprocs ) {
            x = h * ((double)ii - 0.5);
            sum += f(x);
        }
        mypi = h * sum;
        MPE_Log_event( event3b, 0, NULL );//mark end of event 3
        MPE_Log_event( event3, 0, NULL );//mark instant event 3

        pi = 0.0;
        MPE_Log_event( event4a, 0, NULL );//mark start of event 4
        /**Reduces values on all processes to a single value
        @param sendbuf address of the buffer to send
        @param recvbuf address of the buffer to store the result in
        @param count number of elements in the send buffer
        @param datatype data type of the elements in the send buffer (handle)
        @param op The reduce opperation to preform (handle)
        @param root The rank of the process the result will be deliverd to
        @param comm The communcator (handle) what "universe" the message is being sent in
         */
        MPI_Reduce( &mypi, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );
        MPE_Log_event( event4b, 0, NULL );//mark end of event 4

        MPE_Log_sync_clocks();//sync all the clocks for the processes
    }
    if ( argv != NULL )
        MPE_Finish_log( argv[0] );//save the log file to the file specified in arg 0 ... so just the program name? when would argv be null?????
    else
        MPE_Finish_log( "cpilog" );


    if ( myid == 0 ) {//if process 0
        endwtime = MPI_Wtime();//save the end time
        printf( "pi is approximately %.16f, Error is %.16f\n",
                pi, fabs(pi - PI25DT) );
        printf( "wall clock time = %f\n", endwtime-startwtime );
    }
    MPI_Finalize();//terminate the MPI execution enviorment
    return( 0 );
}