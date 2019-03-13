/*
 ============================================================================
 
Description : The philosophers problem - Two chopsticks between each philosopher,
 each philosopher eats with 3 chopsticks

Consider a modified version of the dining philosophers, where as usual there are 5 philosophers sitting
at a table, who are engaged in a think/eat everlasting cycle. However, there are 10 chopsticks on the
table, which are placed in pairs among the philosophers, and the philosopher can eat only using three
of them (either one chopstick on the left and two on the right, or one on the right and two on the left).
The main request of the assignment consists in the synchronization of the philosophers’ actions by the
design of a monitor. Obviously, the critical issues are associated to the resources’ management: the
program must avoid the deadlock problems, whereas it have to prevent the concurrency. 
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>

// CONSTANTS AND MACROS

#define FOREVER for(;;)
#define LONG_TIME 100000000
#define SHORT_TIME 50000
#define N_THREADS 5
#define TRUE 1
#define FALSE 0
typedef int boolean;

// DEFINITIONS OF NEW DATA TYPES

typedef char thread_name_t[10];
typedef enum {THINKING, HUNGRY, EATING} state_t;

typedef struct {
       state_t state;
       boolean chopstick_left;
       boolean available;
    } philosopher;

typedef struct{
    philosopher phi[N_THREADS];
    int chop[N_THREADS];
    pthread_cond_t cv[N_THREADS];
    pthread_mutex_t mutex;
} monitor_t;

// GLOBAL VARIABLES

pthread_mutex_t om;
monitor_t mon;

//  MONITOR API

void monitor_init(monitor_t *mon);
void test(monitor_t *mon, int philo);
void pick_up (monitor_t *mon, int philo);
double do_something(int philo, char *what, long cycles);
void put_down (monitor_t *mon, int philo);
void monitor_destroy(monitor_t *mon);

// OTHER FUNCTIONS

void tabs(int n);
void *philo(void *arg);


//FUNCTION DECLARATIONS

void tabs(int n) {
    while(n-->0)
    putchar('\t');
}

void *philo(void *arg) {

    int philo=atoi((char*)arg);
    int k=0;
    FOREVER {
        do_something(philo,"THINKING", LONG_TIME);

        pthread_mutex_lock(&om);
           tabs(philo);
           printf("%d: HUNGRY!!\n", philo);
        pthread_mutex_unlock(&om);

        pick_up(&mon, philo);
        do_something( philo,"EATING", LONG_TIME);
        put_down(&mon, philo);
        k++;
        printf("\nPHILO %d : eaten %d times\n\n\n",philo,k);
    }
    return NULL;
}

/**
 * monitor initialization function
 * initialization of locks and condvars
 * initialization of values of number of chopsticks available
 * initialization of boolean vectors chopstick_left and available
 */
void monitor_init(monitor_t *mon) {
    pthread_mutex_init (&om,NULL);
    pthread_mutex_init (&mon->mutex,NULL);
    for (int i=0;i<N_THREADS;i++) {
        mon->chop[i]=2;
        mon->phi[i].chopstick_left=0;
        mon->phi[i].available=0;
        pthread_cond_init(&mon->cv[i],NULL);
    }
}

/**
 * Tests if the philosopher can eat
 * If yes signals the philosopher condvar and sets to 1 the available boolean variable
 * If no sets the available variable to 0
 */
void test(monitor_t *mon, int philo) {
    int right=(philo+1)%N_THREADS;
    int sum=mon->chop[right]+mon->chop[philo];
    if (mon->phi[philo].state==HUNGRY && (sum>2)) {
        mon->phi[philo].state=EATING;
        mon->phi[philo].available=1;
        pthread_cond_signal(&mon->cv[philo]);
    }
    else {
        mon->phi[philo].available=0;
        if (mon->phi[philo].state==HUNGRY) {
            pthread_mutex_lock(&om);
              tabs(philo);
              printf("%d: Waiting, only %d available\n\n", philo, sum);
            pthread_mutex_unlock(&om);
        }
    }
}

/**
 * After checking if the philospher can eat, puts the philosopher in the condvar queue if he can't eat
 * else picks up the chopsticks
 */
void pick_up(monitor_t *mon, int philo) {
    pthread_mutex_lock(&mon->mutex);
        int right=(philo+1)%N_THREADS;
        mon->phi[philo].state=HUNGRY;
        test(mon, philo);
        while (mon->phi[philo].available==0) {
            pthread_cond_wait(&mon->cv[philo],&mon->mutex);
        }
        if ((mon->chop[philo]==2)&&(mon->phi[philo].state==EATING)) {
            mon->phi[philo].chopstick_left=1;
            mon->chop[philo]=mon->chop[philo]-2;
            mon->chop[right]--;
        }
        else if(mon->phi[philo].state==EATING) {
                mon->phi[philo].chopstick_left=0;
                mon->chop[right]=mon->chop[right]-2;
                mon->chop[philo]--;
             }
     pthread_mutex_unlock(&mon->mutex);

     pthread_mutex_lock(&om);
        tabs(philo);
        if(mon->phi[philo].chopstick_left==1){
            printf("%d: picking up 2 chopsticks n°%d and 1 chopstick n°%d\n", philo, philo, (philo+1)%N_THREADS);
        }
        else {
            printf("%d: picking up 1 chopsticks n°%d and 2 chopstick n°%d\n", philo, philo, (philo+1)%N_THREADS);
        }
     pthread_mutex_unlock(&om);
}

/**
 * do the action passed with "what" string
 */
double do_something(int philo, char *what, long cycles) {
    double x, sum=0.0, step;
    long i, N_STEPS=rand()%cycles;

    pthread_mutex_lock(&om);
       tabs(philo);
       printf("%d: %s ...\n", philo, what);
    pthread_mutex_unlock(&om);

    step = 1/(double)N_STEPS;
    for(i=0; i<N_STEPS; i++) {
        x = (i+0.5)*step;
        sum+=4.0/(1.0+x*x);
    }

    pthread_mutex_lock(&om);
       tabs(philo);
       printf("%d: ... DONE %s\n", philo, what);
    pthread_mutex_unlock(&om);

    return step*sum;
}

/**
 * Chopsticks are put down in the places they were picked up
 * Then is checked if the near neighbors can eat
 */
void put_down(monitor_t *mon, int philo) {
    pthread_mutex_lock(&mon->mutex);
       int right=(philo+1)%N_THREADS;
       int left=(philo+4)%N_THREADS;

       pthread_mutex_lock(&om);
          tabs(philo);
          if(mon->phi[philo].chopstick_left==1){
              printf("%d: putting down 2 chopsticks n°%d and 1 chopstick n°%d\n\n", philo, philo, right);
          }
          else {
              printf("%d: putting down 1 chopstick n°%d and 2 chopsticks n°%d\n\n", philo, philo, right);
          }
       pthread_mutex_unlock(&om);

       if ((mon->phi[philo].chopstick_left)==1){
           mon->phi[philo].chopstick_left=0;
           mon->chop[philo]=mon->chop[philo]+2;
           mon->chop[right]++;
       }
       else {
           mon->chop[right]=mon->chop[right]+2;
           mon->chop[philo]++;
       }
       mon->phi[philo].available=0;
       mon->phi[philo].state=THINKING;
       test(mon, right);
       test(mon, left);
    pthread_mutex_unlock(&mon->mutex);
}

/**
 * monitor destruction function
 * destruction of locks and condvars
 */
void monitor_destroy(monitor_t *mon) {
     pthread_mutex_destroy(&om);
     pthread_mutex_destroy(&mon->mutex);
     for(int i=0;i<N_THREADS;i++) {
         pthread_cond_destroy(&mon->cv[i]);
     }
}

// MAIN FUNCTION

int main(void) {
    pthread_t my_threads[N_THREADS];
    thread_name_t my_thread_names[N_THREADS];

    //monitor initialization
    monitor_init(&mon);

    //initialization of threads
    for (int i=0;i<N_THREADS;i++) {
        sprintf(my_thread_names[i],"%d",i);
        pthread_create(&my_threads[i], NULL, philo, my_thread_names[i]);
    }
    for (int i=0;i<N_THREADS;i++) {
        pthread_join(my_threads[i], NULL);
    }

    //monitor destruction
    monitor_destroy(&mon);

    return EXIT_SUCCESS;
}
