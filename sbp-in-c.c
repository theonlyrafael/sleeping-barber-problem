#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define CHAIRS 2 // número de cadeiras na sala de espera

sem_t barberReady;        // semáforo que avisa o barbeiro que há um cliente
sem_t accessWRSeats;      // mutex (semáforo binário) que protege as variáveis compartilhadas
int waiting = 0;          // clientes esperando (não conta quem já está sendo atendido)
volatile int working = 1; // indica se o expediente ainda está aberto
int barberSleeping = 0;   // indica se o barbeiro está "dormindo"

void *barber(void *arg)
{
    while (1)
    {
        sem_wait(&accessWRSeats);
        int currentlyWaiting = waiting;

        if (currentlyWaiting == 0)
        {
            if (!working)
            {
                // não há clientes e o expediente já encerrou: o barbeiro pode ir embora
                sem_post(&accessWRSeats);
                break;
            }
            printf("Barbeiro esta dormindo...\n");
            barberSleeping = 1;
        }
        sem_post(&accessWRSeats);

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2; // acorda a cada 2s, no máximo, para checar se o expediente encerrou

        if (sem_timedwait(&barberReady, &ts) == -1)
        {
            // ninguém chegou nesses 2s; volta ao topo do loop e reavalia 'working'/'waiting'
            continue;
        }

        sem_wait(&accessWRSeats);
        waiting--;
        if (waiting < 0)
        {
            waiting = 0;
        }
        printf("Barbeiro esta cortando o cabelo do cliente. %d cliente(s) esperando\n", waiting);
        sem_post(&accessWRSeats);

        sleep(1); // simula o tempo do corte
        printf("Barbeiro terminou de cortar o cabelo do cliente.\n");
    }

    printf("Barbeiro foi para casa.\n");
    return NULL;
}

void *customer(void *arg)
{
    sem_wait(&accessWRSeats);
    if (waiting < CHAIRS)
    {
        waiting++;
        if (barberSleeping == 1)
        {
            printf("Cliente chegou e acordou o barbeiro. %d cliente(s) esperando.\n", waiting);
            barberSleeping = 0;
        }
        else
        {
            printf("Cliente chegou. %d cliente(s) esperando.\n", waiting);
        }
        sem_post(&barberReady);  // avisa o barbeiro
        sem_post(&accessWRSeats);
    }
    else
    {
        sem_post(&accessWRSeats);
        printf("Sala de espera cheia. Cliente foi embora.\n");
    }
    return NULL;
}

int main()
{
    srand(time(0));

    sem_init(&barberReady, 0, 0);
    sem_init(&accessWRSeats, 0, 1);

    pthread_t barberThread;
    int rc = pthread_create(&barberThread, NULL, barber, NULL);
    if (rc)
    {
        printf("ERRO: impossível criar a thread do barbeiro, código de erro = %d\n", rc);
        exit(-1);
    }

    time_t start_time = time(NULL);
    while (working)
    {
        if (rand() % 2 == 0)
        {
            pthread_t customerThread;
            pthread_create(&customerThread, NULL, customer, NULL);
            pthread_detach(customerThread); // a thread encerra sozinha, não precisamos dar join nela
        }
        else
        {
            sleep(1);
        }

        time_t current_time = time(NULL);
        double seconds_passed = difftime(current_time, start_time);
        if (seconds_passed > 20)
        {
            sem_wait(&accessWRSeats);
            working = 0; // encerra o expediente; o barbeiro ainda atende quem já está esperando
            sem_post(&accessWRSeats);
        }
    }

    pthread_join(barberThread, NULL); // espera o barbeiro terminar de atender a fila e ir embora

    sem_destroy(&barberReady);
    sem_destroy(&accessWRSeats);

    printf("Expediente acabou.\n");

    return 0;
}
