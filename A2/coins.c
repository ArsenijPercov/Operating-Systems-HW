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

char coins[21] = "0000000000XXXXXXXXX\0";
pthread_mutex_t lockArr = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lockArr3[20];

int n=10000,p=100;

//method1
static void* person_str1(void *arg){
    int res = 0;
    if(pthread_mutex_lock(&lockArr)){
        fprintf(stderr, "Lock failed in %s %s \n", __func__, strerror(errno));
    }
    //printf("job started : %s\n",coins);
    for (int i = 0;i<n;i++){
        for (int j =0;j<20;j++){
            
            if (res = rand()%2){
                coins[j] = '0';
            }
            else{
                coins[j] = 'X';
            }
        }
    }
    //printf("job finished : %s\n",coins);
    if (pthread_mutex_unlock(&lockArr)){
        fprintf(stderr, "Unlock failed in %s %s \n", __func__, strerror(errno));
    }
    return NULL;
}
//method 2
static void* person_str2(void *arg){
    int res = 0;
    for (int i = 0;i<n;i++){
        if(pthread_mutex_lock(&lockArr)){
            fprintf(stderr, "Lock failed in %s %s \n", __func__, strerror(errno));
        }
        //printf("START %d",i);
        for (int j =0;j<20;j++){
            
            if (res = rand()%2){
                coins[j] = '0';
            }
            else{
                coins[j] = 'X';
            }
        }
        //printf("STOP %d\n",i);
        if (pthread_mutex_unlock(&lockArr)){
            fprintf(stderr, "Unlock failed in %s %s \n", __func__, strerror(errno));
        }
    }
    return NULL;
}
//method 3
static void* person_str3(void *arg){
    int res = 0;
    for (int i = 0;i<n;i++){
        
        for (int j =0;j<20;j++){
            if(pthread_mutex_lock(&lockArr3[j])){
                fprintf(stderr, "Lock failed in %s %s \n", __func__, strerror(errno));
            }
            //printf(" Start %d",j);
            if (res = rand()%2){
                coins[j] = '0';
            }
            else{
                coins[j] = 'X';
            }
            //printf(" STOP %d\n",j);
            if (pthread_mutex_unlock(&lockArr3[j])){
                 fprintf(stderr, "Unlock failed in %s %s \n", __func__, strerror(errno));
            }
        }
       
    }
    return NULL;
}
//method to run 1st option
int run_meth1(){
    int err;
    pthread_t thread[p];
    clock_t t1,t2;
    
    printf("coins: %s (start - global lock)\n",coins);
    t1 = clock();
    for (int i=0;i<p;i++){
        err = pthread_create(&thread[i],NULL,person_str1,NULL);
        if (err){
            fprintf(stderr,"failed to create thread: %s\n", strerror(err));
            return EXIT_FAILURE;
        }
    }
    for (int i=0;i<p;i++){
        if(thread[i]){
            err = pthread_join(thread[i],NULL);
            if (err){
                fprintf(stderr,"failed to join thread: %s\n", strerror(err));
            }
        }
    }
    t2 = clock();
    printf("coins: %s (end - global lock)\n",coins);
    printf("%d threads x %d flips: %f ms\n",p,n,(double)t2-(double)t1);
    return EXIT_SUCCESS;
}
//method to run 2nd option
int run_meth2(){
    int err;
    pthread_t thread[p];
    clock_t t1,t2;
    printf("coins: %s (start - iteration lock)\n",coins);
    t1 = clock();
    for (int i=0;i<p;i++){
        err = pthread_create(&thread[i],NULL,person_str2,NULL);
        if (err){
            fprintf(stderr,"failed to create thread: %s\n", strerror(err));
            return EXIT_FAILURE;
        }
    }
    for (int i=0;i<p;i++){
        if(thread[i]){
            err = pthread_join(thread[i],NULL);
            if (err){
                fprintf(stderr,"failed to join thread: %s\n", strerror(err));
            }
        }
    }
    t2 = clock();
    printf("coins: %s (end - table lock)\n",coins);
    printf("%d threads x %d flips: %f ms\n",p,n,(double)t2-(double)t1);
    return EXIT_SUCCESS;
}
//method to run 3rd option
int run_meth3(){
    for (int i =0;i<20;i++){
        pthread_mutex_init(&lockArr3[i],NULL);
    }

    int err;
    clock_t t1,t2;
    pthread_t thread[p];
    printf("coins: %s (start - coin lock)\n",coins);
    t1 = clock();
    for (int i=0;i<p;i++){
        err = pthread_create(&thread[i],NULL,person_str3,NULL);
        if (err){
            fprintf(stderr,"failed to create thread: %s\n", strerror(err));
            return EXIT_FAILURE;
        }
    }
    for (int i=0;i<p;i++){
        if(thread[i]){
            err = pthread_join(thread[i],NULL);
            if (err){
                fprintf(stderr,"failed to join thread: %s\n", strerror(err));
            }
        }
    }
    t2 = clock();
    printf("coins: %s (end - coin lock)\n",coins);
    printf("%d threads x %d flips: %f ms\n",p,n,(double)t2-(double)t1);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    int opt;
    srand(time(NULL));
    while((opt = getopt(argc, argv, "n:p:")) > 0){
        switch(opt){
            case 'n':
                if ((n = atoi(optarg)) <= 0) {
                    fprintf(stderr, "number of consumers must be > 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                if ((p = atoi(optarg)) <= 0) {
                    fprintf(stderr, "number of consumers must be > 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
        }
    }

    if (run_meth1() == EXIT_FAILURE){
        exit(EXIT_FAILURE);
    }
    if (run_meth2() == EXIT_FAILURE){
        exit(EXIT_FAILURE);
    }
    if (run_meth3() == EXIT_FAILURE){
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}