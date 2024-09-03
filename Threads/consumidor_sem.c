/****************************************************************************
 * Problema clássico do produtor e consumidor, no caso de concorrência      *
 *  de multiprocessamento produtores e consumidores acessam a mesma         *
 *  memoria compartilhada, porém sem operações atômicas por causa da        *
 *  mudança de contexto o programa pode tomar rumos e valores indefinidos   *
 *                                                                          *
 * Nessa implementação será utilizado mutex para manipular a sessão critica *
 *  no caso o vetor de produção, no qual através de dois índices faz a      *
 *  leitura (consumo) e escrita (produção), porém a sincronização entre     *
 *  entre produtores e consumidores será feita por semaforos                *
 *                                                                          *
 * ** GCC incluir a biblioteca pthread através do parâmetro '-lpthread'     *
 * ** Este Programa Finaliza.                                               *
 *************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include <Windows.h>
#define sleep(ms) Sleep(ms)
#elif __linux__
#include <unistd.h>
#else
#error "OS Not Supported"
#endif


/* Número de slots disponíveis para produzir (buffer size)  */
#define MAX_PROD    20
/* Limite de produtos produzidos por cada produtor (produção necessária antes de morrer) */
#define LIMIT_PROD  10

/* Número de Thread rodando função 'void *produtor(void)'       */
#define NUM_PROD    4
/* Número de Thread rodando função 'void *consumidor(void)'     */
#define NUM_CONS    12


pthread_mutex_t mutex_m;    /* Sessão critica acesso ao vetor 'produtos' e variáveis de índices */
sem_t prod_s, cons_s;       /* Semáforo para controlar a produção e consumo */

size_t produtos[MAX_PROD];  /* Vetor de produção (sessão critica) */
size_t len_cons = 0;        /* Índice de consumo no vetor 'produtos' (sessão critica) */
size_t len_prod = 0;        /* Índice de produção no vetor 'produtos' (sessão critica) */

size_t fim_flag = 0;        /* Flag para encerrar consumidores (fim de todo consumo e fim dos produtores) */


void *produtor(void *num_thread)
{
    /* Semente aleatória para essa Thread (horas mais múltiplos de 60) */
    srand((size_t)time(NULL) + (*(size_t *)num_thread + 1) * 60);
    size_t prod_cont = 0;
    while (1)
    {
        sleep((rand() % 3 + 1) * 100);

        /* Vetor cheio aguardando por pelo menos um consumidor */
        sem_wait(&prod_s);

        /* Sessão critica (Exclusão Mútua)*/
        pthread_mutex_lock(&mutex_m);

        /* Inserindo um valor aleatório entre 1 e 99 (simulando a produção) */
        produtos[len_prod] = rand() % 99 + 1;
        prod_cont++;
        printf("Produzindo: %02d, Pos: %02d, Thread: %02ld (%02d/%02d)\n", produtos[len_prod],
               len_prod + 1, *(size_t *)num_thread + 1, prod_cont, LIMIT_PROD);
        len_prod = (len_prod + 1) % MAX_PROD;

        /* Produção inserida (libera pelo menos um consumidor) */
        sem_post(&cons_s);

        /* Fim da sessão critica (Exclusão Mútua)*/
        pthread_mutex_unlock(&mutex_m);

        
        /* Verifica limite de produção */
        if (prod_cont == LIMIT_PROD)
        {
            printf("Fim do produtor: %02ld\n", *(size_t *)num_thread + 1);
            pthread_exit(NULL);
            return NULL; /*opcional*/
        }
    }
}

void *consumidor(void *num_thread)
{
    /* Semente aleatória para essa Thread (horas menos múltiplos de 60) */
    srand((size_t)time(NULL) - (*(size_t *)num_thread + 1) * 60);
    size_t cons_cont = 0;
    while (1)
    {
        sleep((rand() % 4 + 2) * 100);

        /* Vetor cheio aguardando por pelo menos um consumidor */
        sem_wait(&cons_s);

        /* Sessão critica (Exclusão Mútua)*/
        pthread_mutex_lock(&mutex_m);

        /* Vetor vazio aguardando por pelo menos um produtor */
        if (len_cons == len_prod)
        {
            /* Verifica encerramento dos produtores */
            if (fim_flag)
            {
                pthread_mutex_unlock(&mutex_m);
                printf("Fim do consumidor: %02ld (%02ld)\n", *(size_t *)num_thread + 1, cons_cont);
                pthread_exit(NULL);
                return NULL; /*opcional*/
            }
        }

        /* Consumindo (simulando o consumo) */
        cons_cont++;
        printf("Consumindo: %02d, pos: %02d, Thread: %02ld (%02ld)\n", produtos[len_cons],
                len_cons + 1, *(size_t *)num_thread + 1, cons_cont);
        len_cons = (len_cons + 1) % MAX_PROD;

        /* Fim sessão critica (Exclusão Mútua)*/
        pthread_mutex_unlock(&mutex_m);

        /* Consumido (libera um produtor caso esses já tenham enchido o vetor) */
        sem_post(&prod_s);
    }
}

int main(int argc, char const *argv[])
{
    /* Variável para iterações com FOR */
    size_t i;
    /* Threads da produção e consumidores */
    pthread_t prodT[NUM_PROD], consT[NUM_CONS];

    /* Enumera cada Thread produtora pra contar produção de cada */
    size_t num_prod_thread[NUM_PROD], num_cons_thread[NUM_CONS];

    /* Inicialização da Mutex e Semáforos */
    pthread_mutex_init(&mutex_m, NULL);

    sem_init(&prod_s, 0, MAX_PROD);
    sem_init(&cons_s, 0, 0);


    printf("Inicia...\n\n");

    /* Inicialização das Threads (inicia condições de corrida) */
    for (i = 0; i < NUM_CONS; i++)
    {
        num_cons_thread[i] = i;
        pthread_create((consT + i), NULL, (void *)&consumidor, (void *)(num_cons_thread + i));
    }
    for (i = 0; i < NUM_PROD; i++)
    {
        num_prod_thread[i] = i;
        pthread_create((prodT + i), NULL, (void *)&produtor, (void *)(num_prod_thread + i));
    }

    /* Aguarda fim das Threads produtoras */
    for (i = 0; i < NUM_PROD; i++)
        pthread_join(prodT[i], NULL);


    /* Livra possíveis Thread consumidoras do bloqueio de falta de produtos, necessário para finalizarem */
    pthread_mutex_lock(&mutex_m);
     /* Sinaliza fim da produção para consumidores */
    fim_flag = 1;
    /* Caso alguma não tenha chegado ainda no wait, ela irá finalizar antes no if do fim_flag, não travando mais no wait */
    pthread_mutex_unlock(&mutex_m);

    /* Possibilita que todas as threads consumidoras possam saírem e finalizar
    
    obs: O for abaixo deve está somente após o join dos produtores, para que a main não 
        interfira na produção e consumo. Também deve estar após a fim_flag para que não
        tenha preempção e consumidores ficam presos.
    */
    for (i = 0; i < NUM_CONS; i++)
        sem_post(&cons_s);

    /* Aguarda fim das Threads consumidoras */
    for (i = 0; i < NUM_CONS; i++)
    {
        pthread_join(consT[i], NULL);
    }
    printf("\nFim\n");

    pthread_mutex_destroy(&mutex_m);
    sem_destroy(&prod_s);
    sem_destroy(&cons_s);

    return 0;
}
