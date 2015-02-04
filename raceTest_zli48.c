/*
This is homework 5 for CS361, on semaphores
This is done by Ze Li, */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
//#if defined(_GNU_LIBRARY_) && !defined(_SEM_SEMUM_UNDEFINED)
//#else
//union semun{
//    int val;
//    struct semid_ds *buf;
//    unsigned short *array;
//    struct seminfo *_buf;
//}argument;
//#endif
#define N 200
#define M 31

typedef struct {
    int nBuffers;
    int workerID;
    double sleepTime;
    int semID;//initial -1
    int mutexID;//initial -1
    int *buffers;
    int nReadErrors;
} workerStruct;
//forward declarations
void initializeIntArray(int*,int);
void initializeDbArray(double*,int,unsigned,double,double);
void initializeStructs(workerStruct*,int*,double*,int,int);
void printWorker(double*,int);
void printInt(int*,int);
void printStruct(workerStruct*,int);
int checkInt(int*,int,int);
void showbits(unsigned int,int*);
void readOp(workerStruct*,int);
void writeOp(workerStruct*,int,int);
void* worker(void*);
//end of forward declarations

int main(int argc, char** argv){
    pthread_t *threads;
    int nBuffers,nWorkers,*intArray,rc,semid,mutexid,i,s,locked=0;
    unsigned short *semints;
    unsigned int randSeed = 0;
    double sleepMin,sleepMax,*workerArray;
    char *locks;
    workerStruct *structs;
    union semun argument;
       if(argc < 5 ){
        printf("raceTest nBuffers nWorkers sleepMin sleepMax [randSeed] [ -lock | -nolock ]\n");
        exit(0);
    }
    printf("***********************\n");
    printf("Ze Li\n");
    printf("raceTest_zli48.c\n");
    printf("***********************\n");
    nBuffers = atoi(argv[1]);
    if(nBuffers > 31){
        printf("Sorry. nBuffers should be within 1-31.\n");
        exit(0);
    }
    intArray = calloc(nBuffers,sizeof(int));
    nWorkers = atoi(argv[2]);
    if(nWorkers > nBuffers){
        printf("Sorry. nWorkers should be less than nBuffers.\n");
        exit(0);
    }sleepMin = atof(argv[3]);
    if(sleepMin <= 0){
        printf("Sorry. Sleepmin should be a positive value.\n");
        exit(0);
    }sleepMax = atof(argv[4]);
    if(sleepMax <= 0){
        printf("Sorry. Sleepmax should be a positive value.\n");
        exit(0);
    }if(sleepMax <= sleepMin){
        printf("Sorry. Sleepmax should be bigger than sleepMin.\n");
        exit(0);
    }if(argc == 6){
        locks = malloc(sizeof(argv[5]));
        strcpy(locks,argv[5]);
        if(strcmp(locks,"-lock") == 0){//in case the lock is set in front
            printf("Lock set.\n");
            locked = 1;
        }if(strcmp(locks,"-nolock")==0){
//            printf("No lock set.\n");
        }else//read randSeed
        randSeed = atoi(argv[5]);
    }workerArray = malloc(sizeof(double)*nWorkers);//M double arrays
    initializeDbArray(workerArray,nWorkers,randSeed,sleepMin,sleepMax);
    structs = malloc(sizeof(workerStruct)*nWorkers);//M workers
    initializeStructs(structs,intArray,workerArray,nWorkers,nBuffers);
    if (argc == 7){
        locks = malloc(sizeof(argv[6]));
        strcpy(locks,argv[6]);
        printf("%s\n",locks);
        if(strcmp(locks,"-lock") == 0){
            printf("Lock set.\n");
            locked = 1;
        }else{
//            printf("No lock set.\n");
        }
        free(locks);
    }
    if(!locked) printf("No lock is set.\n");
    if((mutexid = semget(IPC_PRIVATE,1,IPC_CREAT|0600))==-1){
        perror("semget failed");
        exit(1);
    } argument.val = 1;
    semctl(mutexid,0,SETVAL,argument);//initialize this semaphore

    if((semid = semget(IPC_PRIVATE,nBuffers,IPC_CREAT|0600))==-1){
        perror("semget failed");
        exit(1);
    }
    semints = malloc(sizeof(unsigned short)*(nBuffers));
    for(s = 0; s < nBuffers;s++){
        semints[s] = 1;
    }
    argument.array = semints;
    semctl(semid,0,SETALL,argument);//initialize buffer semaphores
    threads = malloc(sizeof(pthread_t)*nBuffers);
    for ( i = 0; i < nWorkers; i++ ){
        structs[i].mutexID = mutexid;
        if(locked){
            structs[i].semID = semid;
        }
        rc = pthread_create(&threads[i],NULL,worker,&structs[i]);
        if(rc){
            printf("return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
    }for ( i = 0; i < nWorkers; i++ ){
        rc = pthread_join(threads[i],NULL);
    }
        printf("from main:\n");
    int count = checkInt(intArray,nBuffers,nWorkers);
    printf("total write errors: %d\n",count);
    int total=0;
    for( i = 0; i < nWorkers; i++){
        total+=structs[i].nReadErrors;
    }printf("total read errors: %d\n",total);
    free(threads);
    free(intArray);
    free(workerArray);
    free(structs);
    free(semints);
    printf("done.\n");
    pthread_exit(NULL);
    semctl(semid,0,IPC_RMID,NULL);//get rid of semaphores
    semctl(mutexid,0,IPC_RMID,NULL);

    return 0;
}
//to check int array
void printInt(int *array, int n){
    int i;
    for( i = 0 ; i < n; i++){
        printf("%dth: %d\n",i,array[i]);
    }
}
void printWorker(double *array, int n){
    int i;
    for( i = 0 ; i < n; i++){
        printf("%dth: %f\n",i,array[i]);
    }
}


//initialize a struct
void initializeStructs(workerStruct *structs,int *intArray, double *dbArray,int n, int nBuffers){
    int i;
    for (i = 0; i < n; i++){
        structs[i].nBuffers = nBuffers;
        structs[i].workerID = i+1;
        structs[i].sleepTime = dbArray[i];
        structs[i].semID = -1;
        structs[i].mutexID = -1;
        structs[i].buffers = intArray;
        structs[i].nReadErrors = 0;
    }
}
//initialize double array
void initializeDbArray(double *array, int n, unsigned int seed, double min, double max){
    int i;
    if(seed) srand(seed);
    else    srand(time(NULL));
    for(i = 0; i < n; i++)
        array[i] = min + (double)rand()/RAND_MAX*(max-min);
}

//initialize int array
void initializeIntArray(int *array, int n){
    int i;
    for ( i = 0; i < n; i++){
        array[i] = 0;
    }
}

void showbits(unsigned int x,int *result){
    int i,count = 0;
    for( i = M-1; i>=0; i--){
        (x&(1<<i))?(result[i] =1):(result[i] = 0);
    }
}
//check main output
int checkInt(int *array,int nBuffers, int nWorkers){
    int i,count = 0, target = (1<<nWorkers)-1;
    printf("The value is expected to be: %d\n",target);
    for (i = 0; i < nBuffers; i++ ){
        printf("Buffer %d holds %d ",i,array[i]);
        if(array[i]!= target){
            int ored = target^array[i];
            int bitwised[M],j;
            char toshow[N];
            showbits(ored,bitwised);
            printf("Bad bits = ");
            for(j = 0; j < M; j++){
                int inc = 0;
                if(bitwised[j] == 1){
                    printf("%d ",j);
                    count++;
                }
            }
        }
        printf("\n");
    }
    return count;
}
//read function in worker function
void readOp(workerStruct *worker,int currentid){
    struct sembuf mutexlock={0,-1,0},mutexunlock={0,1,0},//mutex semaphore
    semlock = {currentid,-1,0}, semunlock = {currentid,1,0};
    int i,j,count = 0,semid = worker->semID;
    if(semid>=0){
        semop(semid,&semlock,1);//lock the currentidth semaphore
    }
    unsigned int tempbuffer = worker->buffers[currentid];//store the buffer
    usleep(1000000*worker->sleepTime);//usecond to second
    if(tempbuffer!=worker->buffers[currentid]){
        worker->nReadErrors++;
        semop(worker->mutexID,&mutexlock,1);
        printf("Child number %d reported change from %d to %d in buffer %d. Bad bits = ",worker->workerID,tempbuffer,worker->buffers[currentid],currentid);
        unsigned int badbit = worker->buffers[currentid]^tempbuffer;
        int targetbit[M];
        showbits(tempbuffer,targetbit);
        int bitwised[M];
        showbits(badbit,bitwised);
        for ( i = 0; i < M;i++){
            if(bitwised[i]==1){
                if(targetbit[i]==1){
                    printf("%d ",-i);
                }if(targetbit[i]==0){
                    printf("+%d ",i);
                }
            }
        }printf("\n");
        semop(worker->mutexID,&mutexunlock,1);//unlock mutex
    }
    if(semid>=0)
        semop(semid,&semunlock,1);//unlock the currentidth semaphore
}
//write operations
void writeOp(workerStruct *worker,int id,int workerid){
    struct sembuf semlock = {id,-1,0}, semunlock = {id,1,0};
    int tempbuffer,semid=worker->semID;
    if(semid>=0){
        semop(semid,&semlock,1);
    }
    tempbuffer = worker->buffers[id];
    usleep(1000000*worker->sleepTime);
    tempbuffer+=(1<<(workerid-1));
    worker->buffers[id] = tempbuffer;
    if(semid>=0)
        semop(semid,&semunlock,1);
}
//the main method in worker function
void performAccessing(workerStruct *worker, int workerid){
    int i,currentid = workerid;//store the current id
    int nBuffers = worker->nBuffers;
    for ( i = 0; i < nBuffers; i++){
        readOp(worker,currentid);
        currentid = (currentid+workerid)%nBuffers;
        readOp(worker,currentid);
        currentid = (currentid+workerid)%nBuffers;
        writeOp(worker,currentid,workerid);
        currentid = (currentid+workerid)%nBuffers;
    }
}
//work function
void* worker(void *work){
    workerStruct *workerstruct;
    workerstruct = (workerStruct *)work;
    performAccessing(workerstruct,workerstruct->workerID);
    pthread_exit(NULL);
}






