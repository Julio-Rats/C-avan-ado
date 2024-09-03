/********************************************************************************
 * Problema clássico do jantar dos filósofos, consiste na concorrência de       *
 *  multiprocessamento o acesso e posse de itens, nos quais são compartilhados  *
 *  e é necessários mais de um para produzir algo, no caso do jantar dos        *
 *  filósofos os artefatos são os hashis no qual devem cada filosofo pegar       *
 *  ambos (par de hashi) para de fato comer e devolver os hashis.               *
 *                                                                              *
 * Nessa implementação os hashis são um vetor binário, no qual '0' o hashi não  *
 *  está disponível e '1' está disponível, assim o vetor no índice 0 é o hashi  *
 *  da esquerda do primeiro filosofo e o índice 1 é o hashi da direita, e assim *
 *  por diante. Após 'LIMIT_JANTAS' o filosofo encerra.                         *
 *                                                                              *
 * ** GCC incluir a biblioteca pthread através do parâmetro '-lpthread'         *
 * ** Este Programa Finaliza.                                                   *
 ******************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#define sleep(ms) Sleep(ms)
#elif __linux__
#include <unistd.h>
#else
#error "OS Not Supported"
#endif


/* Número de Filósofos na mesa */
#define NUM_FILOSOFOS  5
/* "Jantares" executadas por cada Thread antes de finalizar */
#define LIMIT_JANTAS  10
/* Tempo que gasta para "comer" em ms */
#define TEMPO_COMER   70


pthread_mutex_t mutex_m;                  /* Sessão Critica acesso as hashis */
pthread_cond_t hashi_cond[NUM_FILOSOFOS]; /* Condicional para travar e devolver a mutex na falha de pegar hashi */

size_t hashi[NUM_FILOSOFOS]; /* vetor binário simulando disponibilidade dos hashis */


/* Verifica se o hashi está disponível, caso sim reserva ("pega") e retorna 1, caso nao retorna 0 */
size_t pega_hashi(size_t pos_filosofo)
{
    if (hashi[pos_filosofo])
    {
        hashi[pos_filosofo] = 0;
        return 1;
    }
    return 0;
}

/* Configura Hashi para disponível */
void devolver_hashi(size_t pos_filosofo)
{
    hashi[pos_filosofo] = 1;
}

/* Toma um tempo (pensando...) */
void pensar(void)
{
    sleep((rand() % 5 + 1) * 100);
}

/* Condições de corrida (Func Threads) */
void *jantar(void *num_filosofo)
{
    size_t jantares = 0;
    /* Semente aleatória para essa Thread (horas mais múltiplos de 60) */
    srand((size_t)time(NULL) + (*(size_t *)num_filosofo + 1) * 60);
    while (1)
    {
        /* Pensa (Delay) */
        pensar();

        /* Sessão critica acesso aos hashis */
        pthread_mutex_lock(&mutex_m);
        /*
            Tenta pegar o hashi da esquerda, caso não consiga entra em
            espera (wait) do mesmo até ser liberado pelo filosofo que
            está utilizando.
        */
        while (pega_hashi(*(size_t *)num_filosofo) == 0)
        {
            /* Hashi ocupado fica esperando o Filosofo devolver o hashi a "mesa" */
            pthread_cond_wait(&hashi_cond[*(size_t *)num_filosofo], &mutex_m);
        }
        /* Fim sessão critica acesso aos hashis */
        pthread_mutex_unlock(&mutex_m);

        /* tempo para preempção */

        /* Sessão critica acesso aos hashis */
        pthread_mutex_lock(&mutex_m);
        /*
            Ao chegar aqui o Filosofo tem posse do hashi da esquerda
            irá tentar pegar o da direita, caso consiga ele come,
            caso não ele devolve o da direita e volta a tentar novamente
            a pegar ambos hashi (Necessário a devolução para evitar deadlock).
        */
        if (pega_hashi((*(size_t *)num_filosofo + 1) % NUM_FILOSOFOS) == 0)
        {
            /* Falhou em pegar Hashi da direita, devolve o da esquerda */
            devolver_hashi(*(size_t *)num_filosofo);
            /* Livra a mutex e volta a pensar */
            pthread_mutex_unlock(&mutex_m);
            continue;
        }
        /* Fim sessão critica acesso aos hashis */
        pthread_mutex_unlock(&mutex_m);

        /* Incremento de jantares */
        jantares++;
        /* Filosofo Comendo*/
        printf("Filosofo %02ld comendo pela %02ld vez\n", *(size_t *)num_filosofo + 1, jantares);
        /* Delay simulando o consumo da thread */
        sleep(TEMPO_COMER);

        /* Sessão critica acesso aos hashis */
        pthread_mutex_lock(&mutex_m);
        /* Devolve hashi da esquerda*/
        devolver_hashi(*(size_t *)num_filosofo);
        /* Fim sessão critica acesso aos hashis */
        pthread_mutex_unlock(&mutex_m);

        /* tempo para preempção */

        /* Sessão critica acesso aos hashis */
        pthread_mutex_lock(&mutex_m);
        /* Devolve hashi da direita */
        devolver_hashi((*(size_t *)num_filosofo + 1) % NUM_FILOSOFOS);
        /* Sinaliza que o hashi da esquerda do filosofo a direita do atual está livre agora */
        pthread_cond_signal(&hashi_cond[(*(size_t *)num_filosofo + 1) % NUM_FILOSOFOS]);
        /* Fim sessão critica acesso aos hashis */
        pthread_mutex_unlock(&mutex_m);

        /* Caso tenha atingido o limite de jantares encerra */
        if (jantares == LIMIT_JANTAS)
            break;
    }
    /* Printa antes de sair que está satisfeito (chegou ao limite de jantares) */
    printf("Filosofo %02ld esta satisfeito !\n", *(size_t *)num_filosofo + 1);
}

int main(int argc, char const *argv[])
{
    /* Variável para iterações com FOR */
    size_t i;
    /* Thread dos filósofos */
    pthread_t filosofo_thread[NUM_FILOSOFOS];
    /* Enumerador das Threas Filósofos */
    size_t pos_filosofo[NUM_FILOSOFOS];

    /* Semente aleatória (clock cpu) */
    srand(time(NULL));

    /* Configurando os hashis como disponíveis */
    memset(&hashi, 1, sizeof(hashi));

    /* Inicialização da Mutex  */
    pthread_mutex_init(&mutex_m, NULL);

    /* Inicialização das Mutex condicionais */
    for (i = 0; i < NUM_FILOSOFOS; i++)
        pthread_cond_init((hashi_cond + i), NULL);

    printf("O jantar esta servido...\n\n");

    /* Inicialização das Threads (inicia condições de corrida) */
    for (i = 0; i < NUM_FILOSOFOS; i++)
    {
        pos_filosofo[i] = i;
        pthread_create((filosofo_thread + i), NULL, (void *)&jantar, (void *)(pos_filosofo + i));
    }

    /* Aguardando o retorno das Thread */
    for (i = 0; i < NUM_FILOSOFOS; i++)
        pthread_join(filosofo_thread[i], NULL);

    printf("\nFim\n");

    return 0;
}
