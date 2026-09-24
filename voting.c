/**
 * @file voting.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Implementacion de la logica de votacion de los mineros.
 * @date 04-05-2026
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "voting.h"
#include "pow.h"
#include "monitor_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define MAX_ATTEMPTS  50
#define WAIT_USEC     100000

/**
 * @brief Funcion auxiliar para escribir el log de votacion de un minero.
 * 
 * @param round Numero de ronda de votacion
 * @param target Target hash que se esta votando
 * @param solution Solucion propuesta por el candidato
 * @param yes Numero de votos a favor
 * @param total Numero total de votos
 * @param coins Numero de monedas del minero despues de la votacion
 * @param accepted Flag que indica si la solucion fue aceptada o no
 */
static void write_log(int round, long target, long solution, int yes, int total, int coins, int accepted) {
    FILE *f;
    char filename[64];
    snprintf(filename, sizeof(filename), "%d.txt", (int)getpid());

    f = fopen(filename, "a");
    if (!f) return;

    fprintf(f, "Round :    %d\n", round);
    fprintf(f, "Target :   %08ld\n", target);
    fprintf(f, "Solution : %08ld (%s)\n", solution, accepted ? "Accepted" : "Rejected");
    fprintf(f, "Votes :    %d/%d\n", yes, total);
    fprintf(f, "Wallet :   %d\n\n", coins);
    fclose(f);
}

/**
 * @brief Funcion ejecutada por cada minero para votar sobre la solucion propuesta por un candidato.
 * 
 * @param net Puntero a la estructura compartida con la informacion de los mineros y votos
 * @param target Target hash que se esta votando
 * @param sem_mutex Semaforo para proteger el acceso a la estructura compartida
 */
void voter_run(MinerInfo *net, long target, sem_t *sem_mutex) {
    sem_wait(sem_mutex);
    
    long solution = net->target;
    
    if (pow_hash(solution) == target) {
        net->votes_accept++;
    } else {
        net->votes_reject++;
    }
    
    sem_post(sem_mutex);
}

/**
 * @brief Funcion ejecutada por el minero ganador para esperar los votos de los demas mineros y escribir el resultado en el log.
 * 
 * @param net Puntero a la estructura compartida con la informacion de los mineros y votos
 * @param target Target hash que se esta votando
 * @param solution Solucion propuesta por el candidato
 * @param round Numero de ronda de votacion
 * @param coins Puntero al numero de monedas del minero ganador, que se actualiza segun el resultado de la votacion
 * @param sem_mutex Semaforo para proteger el acceso a la estructura compartida
 */
void candidate_run(MinerInfo *net, long target, long solution, int round, int *coins, sem_t *sem_mutex) {
    pid_t pids_copy[MAX_MINERS];
    int n_pids = 0;

    sem_wait(sem_mutex);
    net->target = solution;
    net->votes_accept = 0;
    net->votes_reject = 0;
    
    n_pids = net->num_mineros;
    for(int i = 0; i < n_pids; i++) pids_copy[i] = net->pids[i];
    sem_post(sem_mutex);

    for (int i = 0; i < n_pids; i++) {
        if (pids_copy[i] != getpid()) kill(pids_copy[i], SIGUSR2);
    }

    int expected = n_pids - 1;
    int yes = 0, no = 0, total = 0;

    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        sem_wait(sem_mutex);
        yes = net->votes_accept;
        no = net->votes_reject;
        sem_post(sem_mutex);

        total = yes + no;
        if (total >= expected && expected > 0) break;
        usleep(WAIT_USEC);
    }

    int accepted = (yes >= no && total > 0);
    if (accepted) (*coins)++;

    printf("Winner %d => [", (int)getpid());
    for (int i = 0; i < yes; i++) {
        printf(" Y");
    }
    for (int i = 0; i < no; i++) {
        printf(" N");
    }
    printf(" ] => %s\n", accepted ? "Accepted" : "Rejected");
    fflush(stdout);

    write_log(round, target, solution, yes, total, *coins, accepted);
}
