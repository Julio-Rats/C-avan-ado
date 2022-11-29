/***************************************************************************
 * Problema classico do Leitores e Escritores, no caso em concorrência     *
 *  de multiprocessamento leitores podem acessar livremente sem bloquear   *
 *  ninguém, porém um unico escritor irá bloquear todos os leitores        *
 *  e escritores.                                                          *
 *                                                                         *
 * Nessa implementação usando mutex leitores acessam livremente e mantem   *
 *  o acesso de escritores bloqueado enquanto existir pelo menos um leitor.*
 *  Caso um escritor queira acessar ele bloqueia novos leitores e aguarda  *
 *  até os leitores já ativos terminarem, e assim por sua vez acessa a     *
 *  escrita.                                                               *
 *                                                                         *
 * ** GCC incluir a biblioteca pthread através do comando '-lpthread'      *
 * ** Este Programa *NÃO* Finaliza.                                        *
 *************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>   /* time() */
#include <unistd.h> /* usleep() | sleep() */

#define NUM_LEIT 20 /* Número de Threads de Leitura */
#define NUM_ESCR 4  /* Número de Threads de Escrita */

pthread_mutex_t mutex_m, leitura_m, inanicao_m; /* Mutexs para sessões*/
pthread_cond_t anti_inanicao_cond;              /* Variável condiconal da mutex */

unsigned int num_leitores = 0; /* Número de leitores ativos */
unsigned int locket_flag = 0;  /* Trava para priorizar escritores */
unsigned int critico = 0;      /* Simulando memoria critica compartilhada */

void *leitor(void *num_thread)
{
    for (;;)
    {
        usleep(((rand() % 5) + 1) * 100000);

        /* Sessão critica da variavel locket_flag */
        pthread_mutex_lock(&inanicao_m);
        /*
            Caso exista um novo escritor, trava a entrada de novos leitores
            Aqui irá manter novos leitores esperando, enquanto as ativas
            continuem até sair.
        */
        /*
            While para evitar falsos Signals e possíveis novos escritores após último sair, e
            leitores não conseguirem sair do wait a tempo (conflito de preempção).
        */
        while (locket_flag)
        {
            /* Leitores esperam até não ter mais escritores ativos */
            pthread_cond_wait(&anti_inanicao_cond, &inanicao_m);
        }
        /* Fim sessão critica da variavel locket_flag */
        pthread_mutex_unlock(&inanicao_m);

        /* Sessão critica da variavel num_leitores */
        pthread_mutex_lock(&leitura_m);
        /* Caso seja o primeiro leitor bloqueia a mutex de escrita */
        if (++num_leitores == 1)
        {
            /* Bloqueia escritores */
            pthread_mutex_lock(&mutex_m);
        }
        /* Fim sessão critica da variavel num_leitores */
        pthread_mutex_unlock(&leitura_m);

        /* Simulando Leitura */
        printf("Ler critico: %d (%ld)\n", critico, *((size_t *)num_thread));

        /* Sessão critica da variavel num_leitores */
        pthread_mutex_lock(&leitura_m);
        /* Último leitor ativo ao sair libera os escritores */
        if (--num_leitores == 0)
        {
            /* Destrava os escritores */
            pthread_mutex_unlock(&mutex_m);
        }
        /* Fim sessão critica da variavel num_leitores */
        pthread_mutex_unlock(&leitura_m);
    }
}

void *escritor(void *num_thread)
{
    for (;;)
    {
        usleep(((rand() % 5) + 1) * 100000);

        /* Sessão critica da variavel locket_flag */
        pthread_mutex_lock(&inanicao_m);
        /*
            Flag não binaria (0 == Falso, caso contrario é Verdadeiro)
            Incrementa igualmente ao número de escritores ativos assim
            leitores esperam até todos escritores deixar sessão critica.
        */
        locket_flag++;
        /* Fim sessão critica da variavel locket_flag */
        pthread_mutex_unlock(&inanicao_m);

        /* Sessão critica da variavel critico */
        pthread_mutex_lock(&mutex_m);
        /* Simula escrita com número aleatorio de 1 a 100 */
        critico = (rand() % 100) + 1;
        printf("Escreve critico: %d (%ld)\n", critico, *((size_t *)num_thread));
        /* Fim sessão critica da variavel critico */
        pthread_mutex_unlock(&mutex_m);

        /* Sessão critica da variavel locket_flag */
        pthread_mutex_lock(&inanicao_m);
        /*
            Decrementa variavel locket_flag, caso
            seja o último escritor ativo, libera variável
            condicional da mutex (Trava de novos leitores).
        */
        if (--locket_flag == 0)
        {
            /* Libera todos leitores esperando em fila */
            pthread_cond_broadcast(&anti_inanicao_cond);
        }
        /* Fim sessão critica da variavel locket_flag */
        pthread_mutex_unlock(&inanicao_m);
    }
}

int main(int argc, char const *argv[])
{
    /* Threads de escritores e leitores */
    pthread_t esc_trd[NUM_ESCR], lei_trd[NUM_LEIT];
    /* enumerador das Threas */
    size_t num_esc[NUM_ESCR], num_lei[NUM_LEIT];

    /* Semente aleatória (clock cpu) */
    srand(time(NULL));

    /* Inicialização da Mutex e Mutex condicional */
    pthread_mutex_init(&leitura_m, NULL);
    pthread_mutex_init(&mutex_m, NULL);
    pthread_mutex_init(&inanicao_m, NULL);
    pthread_cond_init(&anti_inanicao_cond, NULL);

    printf("Começo\n");

    /* Inicialização das Threads (inicia condições de corrida) */
    for (size_t i = 0; i < NUM_ESCR; i++)
    {
        /* somente para enumerar cada Thread */
        num_esc[i] = i + 1;
        pthread_create((esc_trd + i), NULL, (void *)&escritor, (void *)(num_esc + i));
    }
    for (size_t i = 0; i < NUM_LEIT; i++)
    {
        /* somente para enumerar cada Thread */
        num_lei[i] = i + 1;
        pthread_create((lei_trd + i), NULL, (void *)&leitor, (void *)(num_lei + i));
    }

    /* Aguardando Threas (NÃO ALCANÇÁVEIS NESSE EXEMPLO)*/
    for (size_t i = 0; i < NUM_ESCR; i++)
        pthread_join(esc_trd[i], NULL);
    for (size_t i = 0; i < NUM_LEIT; i++)
        pthread_join(lei_trd[i], NULL);

    printf("Fim\n"); // Nunca será alcançável.

    return 0;
}
