/****************************************************************************
 * Problema clássico do produtor e consumidor, no caso de concorrência      *
 *  de multiprocessamento produtores e consumidores acessam a mesma         *
 *  memoria compartilhada, porém sem operações atômicas por causa da        *
 *  mudança de contexto o programa pode tomar rumos e valores indefinidos   *
 *                                                                          *
 * Nessa implementação será utilizado mutex para manipular a sessão critica *
 *  no caso o vetor de produção, no qual através de dois índices faz a      *
 *  leitura (consumo) e escrita (produção), após cada produtores produzirem *
 *  'LIMIT_PROD' eles encerram, os consumidores consumem o resto de produto *
 *  se existir e finalizam, assim esse programa é finalizado.               *
 *                                                                          *
 * ** GCC incluir a biblioteca pthread através do parâmetro '-lpthread'       *
 * ** Este Programa Finaliza.                                               *
 *************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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

/* Um elemento será inutilizável para distinguir vazio e cheio.  */
#define MAX_PROD    21 /* Número de slots disponíveis pra produzir (buffer size)  */
#define LIMIT_PROD  10 /* Limite de produtos produzidos por cada produtor (produção por thread antes de morrer) */

#define NUM_PROD    4  /* Número de Thread rodando função 'void *produtor(void)'       */
#define NUM_CONS    12 /* Número de Thread rodando função 'void *consumidor(void)'     */

pthread_mutex_t mutex_m, fim_m;      /* Sessão Critica acesso ao vetor 'produtos' e variáveis de índices */
pthread_cond_t prod_cond, cons_cond; /* Índices de controle dos produtores e consumidores sobre o vetor 'produtos' */

size_t len_cons = 0; /* Índice de consumo no vetor 'produtos' (sessão critica) */
size_t len_prod = 0; /* Índice de produção no vetor 'produtos' (sessão critica) */

size_t fim_flag = 0; /* Flag para encerrar consumidores (fim de todo consumo e fim dos produtores) */

size_t produtos[MAX_PROD]; /* Vetor de produção (sessão critica) */

void *produtor(void *num_thread)
{
    srand((size_t)time(NULL) + (*(size_t *)num_thread + 1) * 60);
    size_t prod_cont = 0;
    while (1)
    {
        sleep((rand() % 3 + 1) * 100);

        /* Sessão critica (Exclusão Mútua)*/
        pthread_mutex_lock(&mutex_m);

        /* Vetor cheio aguardando por pelo menos um consumidor */
        while ((len_prod + 1) % MAX_PROD == len_cons)
            pthread_cond_wait(&prod_cond, &mutex_m);

        /* Inserindo um valor aleatório entre 1 e 99 (simulando a produção) */
        produtos[len_prod] = rand() % 99 + 1;
        prod_cont++;
        printf("Produzindo: %02d, Pos: %02d, Thread: %02ld (%02d/%02d)\n", produtos[len_prod],
               len_prod + 1, *(size_t *)num_thread + 1, prod_cont, LIMIT_PROD);
        len_prod = (len_prod + 1) % MAX_PROD;

        /* Produção inserida (libera pelo menos um consumidor) */
        pthread_cond_signal(&cons_cond);

        /* Verifica limite de produção */
        if (prod_cont == LIMIT_PROD)
        {
            pthread_mutex_unlock(&mutex_m);
            printf("Fim do produtor: %02ld\n", *(size_t *)num_thread + 1);
            pthread_exit(NULL);
            return NULL; /*opcional*/
        }

        /* Fim da sessão critica (Exclusão Mútua)*/
        pthread_mutex_unlock(&mutex_m);
    }
}

void *consumidor(void *num_thread)
{
    srand((size_t)time(NULL) - (*(size_t *)num_thread + 1) * 60);
    size_t cons_cont = 0;
    while (1)
    {
        sleep((rand() % 4 + 2) * 100);

        /* Sessão critica (Exclusão Mútua)*/
        pthread_mutex_lock(&mutex_m);

        /* Vetor vazio aguardando por pelo menos um produtor */
        while (len_cons == len_prod)
        {
            /* Verifica encerramento dos produtores */
            pthread_mutex_lock(&fim_m);
            if (fim_flag)
            {
                pthread_mutex_unlock(&fim_m);
                pthread_mutex_unlock(&mutex_m);
                printf("Fim do consumidor: %02ld (%02ld)\n", *(size_t *)num_thread + 1, cons_cont);
                pthread_exit(NULL);
                return NULL; /*opcional*/
            }
            printf("Consumidor %02ld travado\n", *(size_t *)num_thread + 1);
            pthread_mutex_unlock(&fim_m);
            /* Aguarda produção (Produtores existentes ainda) */
            pthread_cond_wait(&cons_cond, &mutex_m);
        }

        /* Consumindo (simulando o consumo) */
        cons_cont++;
        printf("Consumindo: %02d, pos: %02d, Thread: %02ld (%02ld)\n", produtos[len_cons],
                len_cons, *(size_t *)num_thread + 1, cons_cont);
        len_cons = (len_cons + 1) % MAX_PROD;

        /* Consumido (libera um produtor caso esses já tenham enchido o vetor) */
        pthread_cond_signal(&prod_cond);

        /* Fim sessão critica (Exclusão Mútua)*/
        pthread_mutex_unlock(&mutex_m);
    }
}

int main(int argc, char const *argv[])
{
    /* Threads da produção e consumidores */
    pthread_t prodT[NUM_PROD], consT[NUM_CONS];

    /* Enumera cada Thread produtora pra contar produção de cada */
    size_t num_prod_thread[NUM_PROD], num_cons_thread[NUM_CONS];

    /* Semente aleatória (clock cpu) */
    srand(time(NULL));

    /* Inicialização da Mutex e Mutex condicionais */
    pthread_mutex_init(&mutex_m, NULL);
    pthread_mutex_init(&fim_m, NULL);
    pthread_cond_init(&prod_cond, NULL);
    pthread_cond_init(&cons_cond, NULL);

    printf("Inicia...\n\n");

    /* Inicialização das Threads (inicia condições de corrida) */
    for (size_t i = 0; i < NUM_CONS; i++)
    {
        num_cons_thread[i] = i;
        pthread_create((consT + i), NULL, (void *)&consumidor, (void *)(num_cons_thread + i));
    }
    for (size_t i = 0; i < NUM_PROD; i++)
    {
        num_prod_thread[i] = i;
        pthread_create((prodT + i), NULL, (void *)&produtor, (void *)(num_prod_thread + i));
    }

    /* Aguarda fim das Threads produtoras */
    for (size_t i = 0; i < NUM_PROD; i++)
        pthread_join(prodT[i], NULL);

    /* Sinaliza fim da produção para consumidores */
    pthread_mutex_lock(&fim_m);
    fim_flag = 1;
    pthread_mutex_unlock(&fim_m);

    /*
        Contexto: Elas podem ser liberada pois existe um while 'while (len_cons == len_prod)'
        no qual irá mater elas sem consumir o vetor, somente consultando a flag 'fim_flag',
        as que estiverem mantendo o wait condicional 'cons_cond'.
    */
    /* Livra possíveis Thread consumidoras do bloqueio de falta de produtos, necessário para finalizarem */
    pthread_mutex_lock(&mutex_m);
    // Possível condição de corrida (Thread consumidores chegam no wait antes da main setar fim_flag = 1)
    pthread_cond_broadcast(&cons_cond);
    // Caso alguma não tenha chegado ainda no wait, ela irá finalizar antes no if do fim_flag, não travando mais no wait
    pthread_mutex_unlock(&mutex_m);

    /* Aguarda fim das Threads consumidoras */
    for (size_t i = 0; i < NUM_CONS; i++)
        pthread_join(consT[i], NULL);

    printf("\nFim\n");

    pthread_mutex_destroy(&mutex_m);
    pthread_mutex_destroy(&fim_m);
    pthread_cond_destroy(&prod_cond);
    pthread_cond_destroy(&cons_cond);

    return 0;
}
