#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_CUSTOMERS 30

void *barber(void *arg);
void *customer(void *arg);

sem_t chair;
sem_t waitingroom;
sem_t pillow;
sem_t cuttingtime;

volatile int workdone = 0;
int total_customers = 0;

int main(void)
{
    pthread_t barber_thread;
    pthread_t cust_threads[MAX_CUSTOMERS];
    int cust_ids[MAX_CUSTOMERS];
    int num_customers, num_chairs;
    int i;

    srand((unsigned)time(NULL));

    printf("Enter number of customers (max %d): ", MAX_CUSTOMERS);
    if (scanf("%d", &num_customers) != 1) return 1;
    if (num_customers < 1) return 1;
    if (num_customers > MAX_CUSTOMERS) num_customers = MAX_CUSTOMERS;

    printf("Enter number of waiting room chairs (>=0): ");
    if (scanf("%d", &num_chairs) != 1) return 1;
    if (num_chairs < 0) return 1;

    total_customers = num_customers;

    sem_init(&chair, 0, 1);
    sem_init(&waitingroom, 0, num_chairs);
    sem_init(&pillow, 0, 0);
    sem_init(&cuttingtime, 0, 0);

    pthread_create(&barber_thread, NULL, barber, NULL);

    for (i = 0; i < num_customers; i++) {
        cust_ids[i] = i + 1;
        pthread_create(&cust_threads[i], NULL, customer, &cust_ids[i]);
        usleep(1000);
    }

    for (i = 0; i < num_customers; i++) {
        pthread_join(cust_threads[i], NULL);
    }

    workdone = 1;
    sem_post(&pillow);

    pthread_join(barber_thread, NULL);

    sem_destroy(&chair);
    sem_destroy(&waitingroom);
    sem_destroy(&pillow);
    sem_destroy(&cuttingtime);

    printf("Shop is closed. All done.\n");
    return 0;
}

void *barber(void *arg)
{
    (void)arg;
    while (1) {
        printf("Barber: sleeping (waiting for a customer)...\n");
        sem_wait(&pillow);

        if (workdone) {
            printf("Barber: received closing signal. Going home.\n");
            break;
        }

        printf("Barber: cutting hair...\n");
        sleep(2);
        printf("Barber: finished cutting hair.\n");

        sem_post(&cuttingtime);
    }

    return NULL;
}

void *customer(void *arg)
{
    int id = *(int *)arg;
    int travel = rand() % 4 + 1;
    printf("Customer %d: traveling to barber shop (arrives in %d s).\n", id, travel);
    sleep(travel);

    printf("Customer %d: arrived at barber shop.\n", id);

    if (sem_trywait(&waitingroom) != 0) {
        printf("Customer %d: waiting room full. Leaving.\n", id);
        return NULL;
    }

    printf("Customer %d: sitting in waiting room.\n", id);

    sem_wait(&chair);
    sem_post(&waitingroom);

    printf("Customer %d: waking the barber.\n", id);
    sem_post(&pillow);

    sem_wait(&cuttingtime);

    sem_post(&chair);
    printf("Customer %d: leaving barber shop after haircut.\n", id);
    return NULL;
}