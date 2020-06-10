#include <stdlib.h>
#include <stdio.h>
#include<string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>

int c=1,d=1;
int clCount = 0, detCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t detQ = PTHREAD_COND_INITIALIZER;
pthread_cond_t clientQ = PTHREAD_COND_INITIALIZER;
pthread_cond_t leavesW = PTHREAD_COND_INITIALIZER;
pthread_cond_t print = PTHREAD_COND_INITIALIZER;
pthread_cond_t print2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t print3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t print4 = PTHREAD_COND_INITIALIZER;

int leaves = 0;


typedef struct person {
    int role; //0 for detective, 1 for client
    int id;
} person_t;

static void* clientThread(void* arg){
    person_t *pers = (person_t*) arg;
    pthread_mutex_lock(&mutex);
    if (leaves == 1){
        pthread_cond_wait(&leavesW,&mutex);
    }
    clCount++;
    printf("bar:    %d c %d d    c%d entering\n",clCount, detCount, pers->id);
    if (detCount>0){
        detCount--;
        clCount--;
        printf("bar:    %d c %d d    c%d picking detective: ",clCount, detCount, pers->id);
        
        pthread_cond_signal(&detQ);
        pthread_cond_wait(&print,&mutex);
        pthread_cond_signal(&print2);
    }
    else{
        
        printf("bar:    %d c %d d    c%d waiting...\n",clCount, detCount, pers->id);
        pthread_cond_wait(&clientQ,&mutex);
        printf("c%d\n",pers->id);
        pthread_cond_signal(&print3);
        pthread_cond_wait(&print4, &mutex);
        printf("bar:    %d c %d d    ... c%d waking up\n",clCount, detCount, pers->id);

    }
    if (clCount == 0){
        pthread_cond_broadcast(&leavesW);
        leaves = 0;
    }
    printf("bar:    %d c %d d    c%d leaving\n",clCount, detCount, pers->id);
    pthread_mutex_unlock(&mutex);

}

static void* detectiveThread(void* arg){
    person_t *pers = (person_t*) arg;

    pthread_mutex_lock(&mutex);
    if (leaves == 1){
        pthread_cond_wait(&leavesW,&mutex);
    }
    detCount++;
    printf("bar:    %d c %d d    d%d entering\n",clCount, detCount, pers->id);
    if (clCount>0){
        printf("bar:    %d c %d d    d%d picking all clients",clCount, detCount, pers->id);
        int tempCount = clCount;
        clCount = 0;
        detCount--;
        printf("bar:    %d c %d d    d%d leaving\n",clCount, detCount, pers->id);
        leaves = 1;
        pthread_cond_broadcast(&clientQ);
        for (int i =0;i<tempCount;i++){
            pthread_cond_wait(&print3,&mutex);
        
        }
        printf("!!!!");
        pthread_cond_broadcast(&print4);
    }
    else{
        printf("bar:    %d c %d d    d%d waiting\n",clCount, detCount, pers->id);
        pthread_cond_wait(&detQ,&mutex);
        printf("d%d\n",pers->id);
        pthread_cond_signal(&print);
        pthread_cond_wait(&print2,&mutex);
        printf("bar:    %d c %d d    ... d%d waking up\n",clCount, detCount, pers->id);
        printf("bar:    %d c %d d    d%d leaving\n",clCount, detCount, pers->id);
    }
    
    pthread_mutex_unlock(&mutex);
    
}
static void* enjoy_life(void *arg){
    person_t *pers = (person_t*) arg;
    //    printf("KKKKKKKKKKK");
    while(1)
    {
        switch (pers->role)
        {
        case 0:
            detectiveThread(arg);
            break;
        case 1:
            clientThread(arg);    
            break;
        default:
            break;
        }
        usleep(random()%100000);
    }
}
int run(){
    int err;
    pthread_t thread[c+d];
    struct person *detectives;
    detectives= malloc(d*sizeof(person_t));
    struct person *clients; 
    clients = malloc(c*sizeof(person_t));
    for (int i =0;i<d;i++){
        (detectives+i)->id = i;
        (detectives+i)->role = 0;

    }
    for (int i =0;i<c;i++){
        (clients+i)->id = i;
        (clients+i)->role = 1;
    }
    for (int i=0;i<c+d;i++){
        if (i<c) err = pthread_create(&thread[i],NULL,enjoy_life,(void*)(clients+i));
        else err = pthread_create(&thread[i],NULL,enjoy_life,(void*)(detectives+i-c));
        if (err){
            fprintf(stderr,"failed to create thread: %s\n", strerror(err));
            return EXIT_FAILURE;
        }
    }
    for (int i=0;i<c+d;i++){
        if(thread[i]){
            err = pthread_join(thread[i],NULL);
            if (err){
                fprintf(stderr,"failed to join thread: %s\n", strerror(err));
            }
        }
    }
    
    return EXIT_SUCCESS;
}
int main(int argc, char *argv[]){
    int opt;
    srand(time(NULL));
    while((opt = getopt(argc, argv, "c:d:")) > 0){
        switch(opt){
            case 'c':
                if ((c = atoi(optarg)) <= 0) {
                    fprintf(stderr, "number of clients must be > 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                if ((d = atoi(optarg)) <= 0) {
                    fprintf(stderr, "number of detectives must be > 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
        }
    }
    run();
    return EXIT_SUCCESS;
}