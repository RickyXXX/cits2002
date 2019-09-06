#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

/* CITS2002 Project 1 2019
   Name(s):             Haoran zhang 
   Student number(s):   22289211 
 */


//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c


//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES             4
#define MAX_DEVICE_NAME         20
#define MAX_PROCESSES           50
// DO NOT USE THIS - #define MAX_PROCESS_EVENTS      1000
#define MAX_EVENTS_PER_PROCESS	100

#define TIME_CONTEXT_SWITCH     5
#define TIME_ACQUIRE_BUS        5

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum                = 0;
int total_process_completion_time       = ((unsigned)(-1))>>1;

//  ----------------------------------------------------------------------

#define CHAR_COMMENT            '#'
#define MAXWORD                 20

//  ----------------------------------------------------------------------
// implement the priority queue in C
typedef struct Nodes {
    int index;
    int rate;
}PQNode;


void max_Heap_insert(PQNode *heap,int *n,PQNode item)
{
    int i,parent;
    if((*n)==MAX_PROCESSES)
    {
        printf("The heap is full\n");
        return;
    }
    i=++(*n);
    parent=i/2;
    while((i!=1) && (item.rate>heap[parent].rate))
    {
        heap[i]=heap[parent];
        i=parent;
        parent/=2;
    }
    heap[i]=item;
}

int max_Heap_delete(PQNode *heap,int *n)
{
    int item;
    PQNode temp;
    int child,parent;
    if(*n==0)
    {
        printf("The heap is empty.\n");
        exit(1);
    }
    item=heap[1].index;
    temp=heap[(*n)--];
    parent=1;
    child=2*parent;
    while(child<=(*n))
    {
        if(child<*n && heap[child].rate<heap[child+1].rate)
            child++;
        if(temp.rate>=heap[child].rate)break;
        else
        {
            heap[parent]=heap[child];
            parent=child;
            child*=2;
        }
    }
    heap[parent]=temp;
    return item;
}
//  ----------------------------------------------------------------------
// implement the 'queue' in C
int find(int* array){
    //int length = sizeof(array)/sizeof(int);
    int result=0;
    for(int i=0;i<MAX_PROCESSES;i++){
        if(array[i]==-1){
            result=i;
            break;
        }
    }
    return result;
}
void add(int* array, int val){
    int position=find(array);
    array[position]=val;
}
int push(int* array){
    //int length = sizeof(array)/sizeof(int);
    int result = array[0];
    for(int i=0;i<MAX_PROCESSES;i++){
        array[i]=array[i+1];
    }
    array[MAX_PROCESSES-1]=-1;
    return result;
}
void queueprint(int* array){
    int length = sizeof(array)/sizeof(int);
    for(int i=0;i<length;i++){
        if(array[i]!=-1){
            printf("  %d",array[i]);
        }
    }
}

//  ----------------------------------------------------------------------
//  use to store the device information
char deviceName[MAX_DEVICES][MAX_DEVICE_NAME];
int deviceRate[MAX_DEVICES];

//  use to store the process information
int processEventsTime[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int processReboot[MAX_PROCESSES];
int processEventsByte[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int processEventsDevice[MAX_PROCESSES][MAX_EVENTS_PER_PROCESS];
int processExit[MAX_PROCESSES];
int processID[MAX_PROCESSES];
int processNum = 0;

//  ----------------------------------------------------------------------
//  define the process stucture
struct Processes{
    int eventNum;
    int curEvent;
    int state; //1:new 2:ready 3:run 4:block
    //int CPUtime;
    int eventtimeleave;
    int readytorun; //the time from ready to run
    int datatrans;  // the time needed by data transformation
    int blocktodatabus; //the time from block to databus
    //int runtoready;
    //int runtoblock;
    int startruntime;
    //int blocktoready;
    int processinnertime;
}process[MAX_PROCESSES];

//  ----------------------------------------------------------------------
void parse_tracefile(char program[], char tracefile[])
{
    int n=0;
    int processEventsNum=0;

//  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp    = fopen(tracefile, "r");

    if(fp == NULL) {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int  lc     = 0;

//  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while(fgets(line, sizeof line, fp) != NULL) {
        ++lc;

//  COMMENT LINES ARE SIMPLY SKIPPED
        if(line[0] == CHAR_COMMENT) {
            continue;
        }

//  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char  word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

//      printf("%i = %s", nwords, line);

//  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if(nwords <= 0) {
            continue;
        }
//  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENTS
        // FOUND A DEVICE DEFINITION, WE'LL NEED TO STORE THIS SOMEWHERE
        if(nwords == 4 && strcmp(word0, "device") == 0) {
            for(int i = 0;i<MAXWORD;i++){
              deviceName[n][i]=word1[i];   
            }
            deviceRate[n]=atoi(word2); //bytes/sec
            n++;   
        }

        else if(nwords == 1 && strcmp(word0, "reboot") == 0) {
            // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
            continue;   
        }

        else if(nwords == 4 && strcmp(word0, "process") == 0) {
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            processReboot[processNum]=atoi(word2);  
            processID[processNum]=atoi(word1);  
        }

        else if(nwords == 4 && strcmp(word0, "i/o") == 0) {
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
            processEventsTime[processNum][processEventsNum]=atoi(word1); 
            processEventsByte[processNum][processEventsNum]=atoi(word3);
            for(int i=0;i<MAX_DEVICES;i++){
                if(strcmp(word2, deviceName[i]) == 0){
                    processEventsDevice[processNum][processEventsNum]=i;
                }
            }
            processEventsNum++;  
        }

        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
            //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
            processExit[processNum]=atoi(word1);   
        }

        else if(nwords == 1 && strcmp(word0, "}") == 0) {
            process[processNum].eventNum = processEventsNum-1;
            processNum++;   //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
            processEventsNum=0;
        }
        else {
            printf("%s: line %i of '%s' is unrecognized",
                        program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
}

#undef  MAXWORD
#undef  CHAR_COMMENT

//  ----------------------------------------------------------------------
//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM
void simulate_job_mix(int time_quantum)
{
    printf("running simulate_job_mix( time_quantum = %i usecs )\n",time_quantum);
    int mintime=((unsigned)(-1))>>1;

    for(int i=0;i<processNum;i++){
        process[i].state=0;
        process[i].curEvent=0;
        process[i].eventtimeleave=processEventsTime[i][0];       
        process[i].processinnertime=processExit[i]-processEventsTime[i][process[i].eventNum];
        
        if(processReboot[i]<mintime){
            mintime=processReboot[i];
        }
    }


    int n = 0;
    int time =0;
    int nexit =0;
    int running = -1;//indicate if have a process running in the CPU
    int databus = -1;//indicate if have a process using the data bus

    int readyqueue[MAX_PROCESSES];

    for(int i=0;i<MAX_PROCESSES;i++){ //initalize the ready queue
        readyqueue[i]=-1;
    }
    PQNode heap[MAX_PROCESSES]; //initalize the block priority queue
    
    while(nexit!=processNum){
        for(int i=0;i<processNum;i++){//check what processes are doing in each time
            
            if(process[i].state==0){ //new
                if(processReboot[i]==time){
                    process[i].state=1; //new->ready
                    add(readyqueue,i);
                    printf("%d  P%d.NEW->READY \n",time,processID[i]);
                    
                }
            }

            if(process[i].state==1){ //ready
                
                
                if(readyqueue[0]==i && running==-1){
                    
                    //push(readyqueue);
                    running=i;
                    process[i].readytorun=time+TIME_CONTEXT_SWITCH;
                    
                }
                
                if(running==i&&process[i].readytorun==time){
                    printf("%d  P%d.READY->RUNNING \n",time,processID[i]);
                    process[i].state=2; // ready->running
                    process[i].startruntime=time;
                    push(readyqueue);
                    
                }
               
            }

            if(process[i].state==2){ //run
               
                if(process[i].curEvent>process[i].eventNum){
                    
                    if(process[i].processinnertime>time_quantum){
                        if(time==time_quantum+process[i].startruntime){
                            //printf("%d\n",process[i].processinnertime);
                            process[i].processinnertime-=time_quantum;
                            running=-1;
                            
                            if(readyqueue[0]!=-1){ //ready queue is not empty
                                add(readyqueue,i);
                                printf("%d  P%d.expire ,P%d.RUNNING->READY \n",time,processID[i],processID[i]);
                                process[i].state=1;
                                
                                
                            
                                running=readyqueue[0];
                                process[running].readytorun=time+TIME_CONTEXT_SWITCH;
                                process[running].state=1;
                            }
                            if(readyqueue[0]==-1){ 
                                running=i;
                                process[i].startruntime=time;
                                printf("%d  P%d.freshTQ  \n",time,processID[i]);
                                
                                process[i].state=2;
                            }
                        }
                    }
                    //if(process[i].processinnertime<time_quantum){
                    else
                    {
                        if(time==process[i].processinnertime+process[i].startruntime){
                            process[i].state=4; //run to exit
                            nexit++;
                            printf("%d  P%d.RUNNING->EXIT \n",time,processID[i]);
                            
                            running=-1;
                            
                            if(readyqueue[0]!=-1){ //ready queue is not empty
                                //queueprint(readyqueue);
                                running=readyqueue[0];
                                process[running].readytorun=time+TIME_CONTEXT_SWITCH;
                                process[running].state=1;
                            }
                            
                        }
                    }
                    
                }
                else{

                    if(process[i].eventtimeleave<=time_quantum&&process[i].curEvent<=process[i].eventNum){
                        
                        if(time==(process[i].eventtimeleave+process[i].startruntime)){
                            process[i].state=3;//run to block
                            running=-1;
                            
                            PQNode a;
                            a.index=i;
                            a.rate=deviceRate[processEventsDevice[i][process[i].curEvent]];
                            max_Heap_insert(heap,&n,a);
                            //process[i].state=3;
                            printf("%d  P%d.RUNNING->BLOCKED(%s) \n",time,processID[i],deviceName[processEventsDevice[i][process[i].curEvent]]);
                            

                            if(readyqueue[0]!=-1){ //ready queue is not empty
                                running=readyqueue[0];
                                process[running].readytorun=time+TIME_CONTEXT_SWITCH;
                                //push(readyqueue);
                                process[running].state=1;
                            }
                        }
                    }
                    
                    if(process[i].eventtimeleave>time_quantum && process[i].curEvent<=process[i].eventNum){
                        if(time==time_quantum+process[i].startruntime){
                            process[i].eventtimeleave-=time_quantum;
                            running=-1;
                            
                            if(readyqueue[0]!=-1){ //ready queue is not empty
                                printf("%d  P%d.expire ,P%d.RUNNING->READY  \n",time,processID[i],processID[i]);
                                //process[i].runtoready=time+TIME_CONTEXT_SWITCH;
                                process[i].state=1;
                                add(readyqueue,i);
                                
                            
                                running=readyqueue[0];
                                process[running].readytorun=time+TIME_CONTEXT_SWITCH;
                                process[running].state=1;
                            }
                            if(readyqueue[0]==-1){
                                running=i;
                                process[i].startruntime=time;
                                printf("%d  P%d.freshTQ  \n",time,processID[i]);
                                
                                process[i].state=2;
                            }
                        }
                    }
                }
            }

            if(process[i].state==3){ //block
                int v1 = max_Heap_delete(heap,&n);
                //printf("%d \n",v1);
                PQNode val;
                val.index=v1;
                val.rate=deviceRate[processEventsDevice[i][process[i].curEvent]];
                max_Heap_insert(heap,&n,val);

                if(v1==i&&databus==-1){
                    databus=i; 
                    process[i].blocktodatabus=time+TIME_ACQUIRE_BUS;
                }
                if(databus==i&&process[i].blocktodatabus==time){
                    printf("%d  P%d.request_databus \n",time,processID[i]);
                    float q1 = deviceRate[processEventsDevice[i][process[i].curEvent]]/1000000.0;
                    float q2 = processEventsByte[i][process[i].curEvent]/q1;
                    process[i].datatrans=time+ceil(q2);
                    //printf("%f,%f,%d,%d\n",q1,q2,process[i].datatrans,deviceRate[processEventsDevice[i][process[i].curEvent]]);
                }
                if(databus==i&&time==process[i].datatrans){
                    printf("%d  P%d.release_databus , P%d.BLOCKED(%s)->READY  \n",time,processID[i],processID[i],deviceName[processEventsDevice[i][process[i].curEvent]]);
                    process[i].state=1;

                    max_Heap_delete(heap,&n);
                    add(readyqueue,i);
                    process[i].curEvent++;

                    if(process[i].curEvent>process[i].eventNum){
                        process[i].eventtimeleave=-1;
                    }
                    else{
                        process[i].eventtimeleave=processEventsTime[i][process[i].curEvent]-processEventsTime[i][process[i].curEvent-1];
                    }
                    
                    databus=-1;

                    if(readyqueue[0]==i&&running==-1){
                        running=i;
                        process[i].readytorun=time+TIME_CONTEXT_SWITCH;
                        
                    }

                    
                    if(n!=0){
                        int v2 = max_Heap_delete(heap,&n);
                
                        PQNode val1;
                        val1.index=v2;
                        val1.rate=deviceRate[processEventsDevice[i][process[i].curEvent]];
                        max_Heap_insert(heap,&n,val);
                        databus=v2; 
                        process[v2].blocktodatabus=time+TIME_ACQUIRE_BUS;
                    }
                }
            }

            if(process[i].state==4){ //exit
                continue;
            }
        }
        time++;
    }
    printf("total_process_completion_time %d %d\n",time_quantum,time-1-mintime);
    printf("\n");

    if(time-1-mintime <= total_process_completion_time){
        total_process_completion_time=time-1-mintime;
        optimal_time_quantum=time_quantum;
    }
    
    
}

//  ----------------------------------------------------------------------

void usage(char program[])
{
    printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    int TQ0 = 0, TQfinal = 0, TQinc = 0;

//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND THREE TIME VALUES
    if(argcount == 5) {
        TQ0     = atoi(argvalue[2]);
        TQfinal = atoi(argvalue[3]);
        TQinc   = atoi(argvalue[4]);

        if(TQ0 < 1 || TQfinal < TQ0 || TQinc < 1) {
            usage(argvalue[0]);
        }
    }
//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND ONE TIME VALUE
    else if(argcount == 3) {
        TQ0     = atoi(argvalue[2]);
        if(TQ0 < 1) {
            usage(argvalue[0]);
        }
        TQfinal = TQ0;
        TQinc   = 1;
    }
//  CALLED INCORRECTLY, REPORT THE ERROR AND TERMINATE
    else {
        usage(argvalue[0]);
    }

//  READ THE JOB-MIX FROM THE TRACEFILE, STORING INFORMATION IN DATA-STRUCTURES
    parse_tracefile(argvalue[0], argvalue[1]);
    
//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
//  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
//  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED

    for(int time_quantum=TQ0 ; time_quantum<=TQfinal ; time_quantum += TQinc) {
        simulate_job_mix(time_quantum);
    }

//  PRINT THE PROGRAM'S RESULT
    printf("best %i %i\n", optimal_time_quantum, total_process_completion_time);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
