#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define CHAIRS 2 // número de cadeiras na sala de espera

sem_t barberReady;      // semáforo para indicar se o barbeiro está pronto para cortar o cabelo
sem_t accessWRSeats;    // semáforo para controlar o acesso às cadeiras na sala de espera
int waiting = 0;        // número de clientes esperando (não inclui o cliente que está sendo atendido)
int working = 1;        // indica se o barbeiro está trabalhando
int barberSleeping = 0; // indica se o barbeiro está dormindo ou não
sem_t endOfDay;         // semáforo para indicar o fim do expediente

void *barber(void *arg)
{
    while (working)
    {
        sem_wait(&accessWRSeats); // solicita acesso às cadeiras
        if (waiting == 0)         // se não houver clientes esperando
        {
            printf("Barbeiro esta dormindo...\n");
            barberSleeping = 1; /* barbeiro está dormindo*/
        }
        sem_post(&accessWRSeats); // libera o acesso às cadeiras

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2; // espera por um cliente por no máximo 2 segundos

        // espera por um cliente ou pelo fim do expediente
        if (sem_timedwait(&barberReady, &ts) == -1)
        {
            if (errno == ETIMEDOUT)
            {
                sem_wait(&endOfDay); // espera pelo fim do expediente
                break;
            }
        }

        sem_wait(&accessWRSeats); // solicita acesso às cadeiras
        waiting--;                // atende um cliente
        if (waiting < 0)
        {
            waiting = 0;
        }
        printf("Barbeiro esta cortando o cabelo do cliente. %d cliente(s) esperando\n", waiting);
        sem_post(&accessWRSeats); // libera o acesso às cadeiras
        sleep(1);                 // leva algum tempo para cortar o cabelo
        printf("Barbeiro terminou de cortar o cabelo do cliente.\n");
    }
}

/*void *barber(void *arg)
{
    while (working)
    {
        sem_wait(&accessWRSeats); // solicita acesso às cadeiras
        if (waiting == 0)         // se não houver clientes esperando
        {
            printf("Barbeiro esta dormindo...\n");
            barberSleeping = 1; /* barbeiro está dormindo
        }
        sem_post(&accessWRSeats); // libera o acesso às cadeiras
        if (working == 0)
        {
            break;
        }
        sem_wait(&barberReady);   // espera por um cliente
        sem_wait(&accessWRSeats); // solicita acesso às cadeiras
        waiting--;                // atende um cliente
        if(waiting < 0){
            waiting = 0;
        }
        printf("Barbeiro esta cortando o cabelo de um cliente. %d cliente(s) esperando\n", waiting);
        sem_post(&accessWRSeats); // libera o acesso às cadeiras
        sleep(1);                 // leva algum tempo para cortar o cabelo
        printf("Barbeiro terminou de cortar o cabelo de um cliente.\n");
    }
    /*return NULL;
}*/

void *customer(void *arg)
{
    sem_wait(&accessWRSeats); // solicita acesso às cadeiras
    if (waiting < CHAIRS)
    {              // se houver cadeiras vazias
        waiting++; // ocupa uma cadeira
        if (waiting == 1 && barberSleeping == 1)
        { /* se não houver outros clientes esperando e o barbeiro estiver dormindo*/
            printf("Cliente chegou e acordou o barbeiro\n");
            barberSleeping = 0; /* barbeiro está acordado*/
            /*sem_post(&barberReady);*/
        }
        else
        {
            /*waiting++;*/ // ocupa uma cadeira
            printf("Cliente chegou. %d cliente(s) esperando.\n", waiting);
        }
        sem_post(&barberReady);   // notifica o barbeiro
        sem_post(&accessWRSeats); // libera o acesso às cadeiras
    }
    else
    {
        sem_post(&accessWRSeats); // libera o acesso às cadeiras
        printf("Sala de espera cheia. Cliente foi embora.\n");
    }
    return NULL; // cliente foi embora
}

int main()
{
    srand(time(0));

    // inicializa os semáforos
    sem_init(&barberReady, 0, 0);
    sem_init(&accessWRSeats, 0, 1);
    sem_init(&endOfDay, 0, 0); // inicializa o semáforo do fim do expediente

    pthread_t barberThread, customerThreads;
    int rc;

    // inicia a thread do barbeiro
    rc = pthread_create(&barberThread, NULL, barber, NULL);
    if (rc)
    {
        printf("ERRO: impossível criar a thread do barbeiro, código de erro = %d\n", rc);
        exit(-1);
    }

    time_t start_time = time(NULL);
    int customerCount = 0; // contador para o número de clientes
    while (working)
    {
        if (rand() % 2 == 0)
        { // a cada segundo, há uma chance de 50% de um cliente chegar
            pthread_create(&customerThreads, NULL, customer, NULL);
        }
        else
        {
            sleep(1); // nenhum cliente chegou, espera três segundos
        }
        time_t current_time = time(NULL);                           // obtém o tempo atual
        double seconds_passed = difftime(current_time, start_time); // calcula o número de segundos que se passaram
        if (seconds_passed > 20)
        {                        // se passaram mais de 30 segundos
            working = 0;         // o barbeiro termina de trabalhar
            sem_post(&endOfDay); // sinaliza o fim do expediente
        }
    }

    // aguarda todas as threads dos clientes terminarem
    for (int i = 0; i < customerCount; i++)
    {
        pthread_join(customerThreads, NULL);
    }

    pthread_join(barberThread, NULL);

    // Destrói os semáforos
    sem_destroy(&barberReady);
    sem_destroy(&accessWRSeats);

    printf("Expediente acabou.\n");

    return 0;
}