/*
 ============================================================================
Description : The forklift and the carts

Consider a box factory where a robotic forklift waits for motorized carts to 
bring boxes that the forklift then has to place. In particular, there are 10 
motorized carts which can unload their boxes to the floor in a narrow alley 
near the forklift. However, the narrow alley can only accommodate 5 boxes 
at a time and the alley initially is empty, i.e. there is room for 5 boxes. 
Instead, the forklift's behavior is: as soon as there's a box to place, 
the forklift takes a box and goes to place it; after placing the box, 
the fork lift returns to the alley.

The main request of the assignment consists in the synchronization of 
carts and forklift's actions: the carts have to decide whether the box 
can be unloaded on the alley floor or should be returned without, 
whereas the forklift must determine if one or more boxes are effectively
placed on the alley and can be taken or not. Regarding boxes' management,
the goal is to prevent data inconsistency problems. 
============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#define FOREVER while(!time_ended)
#define N_THREADS 2
#define PREFIX "cart-"

/* the following values are just examples of the possible duration
 * of each action and of the simulation: feel free to change them */
#define MAX_DELAY_PLACE_BOX 3
#define MAX_DELAY_PICK_UP_BOX 3
#define MAX_DELAY_UNLOAD_BOX 3
#define MAX_DELAY_BRING_BOX_BACK 3
#define END_OF_TIME 40
#define TRUE 1
#define FALSE 0

typedef char name_t[20];
typedef int boolean;

time_t big_bang;
/**
 * start_forklift and start_cart are used, as explained in the text file,
 * to release as soon as possible the semaphore. In this way we don't waste time
 * doing the actions in the critical section
 */
boolean time_ended=FALSE, start_forklift, start_cart[N_THREADS];

/**
 * box_cart = number of boxes seen by the carts
 * box_forklift = number of boxes seen by the forklift
 * placed_counter = number of boxes placed by the forklift
 * alley_counter = number of boxes placed in the alley
 * back_counter = number of boxes carried back by the carts
 * check_1 and check_2 are two variables only used to control if the sum of the counter is
 *		  done in the right way
 */
int box_cart=0, box_forklift=0, placed_counter=0, back_counter=0, alley_counter=0, check_1=0, check_2=0;

/**
 * semaphores
 * cart_sem is the semaphore linked to the carts
 * forklift_sem is the semaphore linked to the forklift
 */
sem_t cart_sem, forklift_sem;


void do_action(char *thread_name, char *action_name, int max_delay) {
    int delay=rand()%max_delay+1;
    printf("[%4.0f]\t%s: %s (%d) started\n", difftime(time(NULL),big_bang), thread_name, action_name, delay);
    sleep(delay);
    printf("[%4.0f]\t%s: %s (%d) ended\n", difftime(time(NULL),big_bang), thread_name, action_name, delay);
}

void *forklift(void *arg) {
    FOREVER {

        sem_wait(&forklift_sem);
        if(box_forklift>0) {
			start_forklift=TRUE;
			box_forklift--;
			check_1++; //variable used only to verify the correctness of the counters results
		}
        sem_post(&forklift_sem);

        if (start_forklift) {
			do_action("fork lift", "place box and return to alley", MAX_DELAY_PLACE_BOX);
			puts("Terminating fork lift.");
			start_forklift=FALSE;
			placed_counter++;
			sem_wait(&cart_sem);
			box_cart--;
			sem_post(&cart_sem);
			}
        }

    pthread_exit(NULL);
}

void *cart(void *thread_name) {
    int id=atoi(thread_name+strlen(PREFIX));
    FOREVER {
        do_action(thread_name, "pick up box and go to alley", MAX_DELAY_PICK_UP_BOX);
        sem_wait(&cart_sem);
        if (box_cart<5) {
			start_cart[id]=TRUE;
			box_cart++;
		}
        check_2++; //variable used only to verify the correctness of the counters results
        sem_post(&cart_sem);
        if (start_cart[id]) {
            do_action(thread_name, "unload box", MAX_DELAY_UNLOAD_BOX);
            alley_counter++;
            start_cart[id]=FALSE;
            sem_wait(&forklift_sem);
            box_forklift++;
            sem_post(&forklift_sem);
        }
        else {
            do_action(thread_name, "bring box back", MAX_DELAY_BRING_BOX_BACK);
            back_counter++;
        }
    }
    printf("Terminating cart %d.\n", id);
    pthread_exit(NULL);
}

int main(void) {

	//initialization of the semaphores
    sem_init(&cart_sem,0,1);
    sem_init(&forklift_sem,0,1);

    pthread_t fork_id, cart_id[N_THREADS];
    name_t cart_name[N_THREADS];
    int i;

    time(&big_bang);

    pthread_create(&fork_id, NULL, forklift, NULL);
    for(i=0;i<N_THREADS;i++) {
        sprintf(cart_name[i],"%s%d", PREFIX, i);
        pthread_create(cart_id+i,NULL,cart,cart_name+i);
    }

    sleep(END_OF_TIME);
    time_ended=TRUE;

    pthread_join(fork_id,NULL);
    for(i=0;i<N_THREADS;i++) {
        pthread_join(cart_id[i],NULL);
    }
    //destruction of the semaphores
    sem_destroy(&cart_sem);
    sem_destroy(&forklift_sem);

    printf("Number of boxes placed in the alley=%d\n", alley_counter);
    printf("Number of boxes returned by carts=%d\n", back_counter);
    printf("Number of boxes placed by the forklift=%d\n", placed_counter);
    printf("Number of boxes seen by the carts=%d\n", box_cart);
    printf("Number of boxes seen by the forklift=%d\n", box_forklift);
    printf("Check:%d\n",check_1+check_2); //used only to verify the correctness of the counters results

    return EXIT_SUCCESS;
}
