/**
 * @file miner_help.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Implementacion de la logica principal del minero.
 * @date 04-05-2026
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "miner_help.h"
#include "pow.h"
#include "miner_worker.h"
#include "voting.h"
#include "pid_registry.h"
#include "monitor_common.h"

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define SEM_MUTEX    "/sem_miner_mutex"
#define SEM_WINNER   "/sem_miner_winner"
#define SHM_NET_NAME "/shm_miner_network"

static volatile sig_atomic_t got_stop = 0;
static volatile sig_atomic_t got_usr1  = 0;
static volatile sig_atomic_t got_usr2  = 0;

static void handler_stop(int sig) {
    (void)sig;
    got_stop = 1;
}

static void handler_usr1(int sig) {
    (void)sig;
    got_usr1 = 1;
}

static void handler_usr2(int sig) {
    (void)sig;
    got_usr2 = 1;
}

static long read_target(MinerInfo *info) {
    return info->target;
}

static void write_target(MinerInfo *info, long target) {
    info->target = target;
}

int run_miner_network_logic(int n_secs, int n_threads, int pipe_write_fd) {
    
    (void)pipe_write_fd;

    struct sigaction act_stop, act_usr1, act_usr2;
    sigset_t set, oset;

    act_stop.sa_handler = handler_stop;
    sigfillset(&(act_stop.sa_mask));
    act_stop.sa_flags = 0;
    if (sigaction(SIGALRM, &act_stop, NULL) < 0 ||
        sigaction(SIGINT,  &act_stop, NULL) < 0 ||
        sigaction(SIGTERM, &act_stop, NULL) < 0) {
        perror("sigaction (stop)");
        return EXIT_FAILURE;
    }

    act_usr1.sa_handler = handler_usr1;
    sigfillset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;
    if (sigaction(SIGUSR1, &act_usr1, NULL) < 0) {
        perror("sigaction (SIGUSR1)");
        return EXIT_FAILURE;
    }

    act_usr2.sa_handler = handler_usr2;
    sigfillset(&(act_usr2.sa_mask));
    act_usr2.sa_flags = 0;
    if (sigaction(SIGUSR2, &act_usr2, NULL) < 0) {
        perror("sigaction (SIGUSR2)");
        return EXIT_FAILURE;
    }

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    
    if (sigprocmask(SIG_BLOCK, &set, &oset) < 0) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    /* Abrimos la memoria compartida */
    int shm_fd = shm_open(SHM_NET_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open (miner_info)");
        return EXIT_FAILURE;
    }

    /* Inicializamos el tamaño de la memoria compartida */
    if (ftruncate(shm_fd, sizeof(MinerInfo)) == -1) {
        perror("ftruncate");
        return EXIT_FAILURE;
    }

    /* Mapeamos la memoria compartida */
    MinerInfo *info = mmap(NULL, sizeof(MinerInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (info == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    close(shm_fd);

    sem_t *sem_winner = sem_open(SEM_WINNER, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_winner == SEM_FAILED) {
        perror("sem_open (winner)");
        return EXIT_FAILURE;
    }

    sem_t *sem_mutex = sem_open(SEM_MUTEX, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_mutex == SEM_FAILED) {
        perror("sem_open (mutex)");
        return EXIT_FAILURE;
    }

    int is_first = 0;
    long target = 0;

    sem_wait(sem_mutex);
    if (add_pid_to_network(info) < 0) {
        sem_post(sem_mutex);
        sem_close(sem_mutex);
        return EXIT_FAILURE;
    }

    if (get_number_pids(info) == 1) {
        is_first = 1;
        write_target(info, 0);
    } else {
        target = read_target(info);
    }
    sem_post(sem_mutex);

    alarm((unsigned int)n_secs);

    if (is_first) {
        while (!got_stop) {
            sem_wait(sem_mutex);
            int count = get_number_pids(info);
            sem_post(sem_mutex);
            if (count >= 2) break;
            usleep(100000);
        }

        if (!got_stop) {
            sem_wait(sem_mutex);
            int n_pids = info->num_mineros;
            pid_t pids_copy[MAX_MINERS];
            for(int i=0; i<n_pids; i++) pids_copy[i] = info->pids[i];
            sem_post(sem_mutex);
            
            for (int i = 0; i < n_pids; i++) {
                kill(pids_copy[i], SIGUSR1);
            }
        }
    }

    int round = 0;
    int coins = 0;

    while (!got_stop) {
        while (!got_usr1 && !got_usr2 && !got_stop) {
            sigsuspend(&oset);
        }
        if (got_stop) break;
        got_usr1 = 0;
        
        sem_wait(sem_mutex);
        int n_active = get_number_pids(info);
        sem_post(sem_mutex);

        if (n_active < 2) {
            while (!got_stop) {
                sem_wait(sem_mutex);
                n_active = get_number_pids(info);
                sem_post(sem_mutex);
                if (n_active >= 2) break;
                usleep(100000);
            }
            if (got_stop) break;
            
            sem_wait(sem_mutex);
            int n_pids = info->num_mineros;
            pid_t pids_copy[MAX_MINERS];
            for (int i = 0; i < n_pids; i++) {
                pids_copy[i] = info->pids[i];
            }
            sem_post(sem_mutex);
            
            for (int i = 0; i < n_pids; i++) {
                kill(pids_copy[i], SIGUSR1);
            }
            while (!got_usr1 && !got_usr2 && !got_stop) {
                sigsuspend(&oset);
            }
            if (got_stop) break;
            got_usr1 = 0;
        }

        round++;

        sem_wait(sem_mutex);
        target = read_target(info);
        sem_post(sem_mutex);

        long solution = -1;
        int status = miner_run(target, n_threads, &solution);

        if (status == EXIT_SUCCESS && !got_usr2) {
            if (sem_trywait(sem_winner) == 0) {
                
                candidate_run(info, target, solution, round, &coins, sem_mutex);
                sem_wait(sem_mutex);
                write_target(info, solution);
                sem_post(sem_mutex);

                sem_post(sem_winner);

                /* Enviamos el bloque a la cola de mensajes */
                Block bloque;
                memset(&bloque, 0, sizeof(Block));
                
                bloque.target = target;
                bloque.solution = solution;
                bloque.valid = 1;
                bloque.is_end = 0;
                bloque.miner_pid = getpid();
                bloque.coins = coins;
                
                sem_wait(sem_mutex);
                for (int i = 0; i < info->num_mineros; i++) {
                    if (info->pids[i] == getpid()) {
                        bloque.miner_id = i;
                        break;
                    }
                }
                sem_post(sem_mutex);

                if (write(pipe_write_fd, &bloque, sizeof(Block)) < 0) {
                    perror("write winner pipe");
                }

                /* Espera pequena para la salida */
                usleep(50000);

                sem_wait(sem_mutex);
                int n_pids = info->num_mineros;
                pid_t pids_copy[MAX_MINERS];
                for (int i = 0; i < n_pids; i++) {
                    pids_copy[i] = info->pids[i];
                }
                sem_post(sem_mutex);
                
                for (int i = 0; i < n_pids; i++) {
                    kill(pids_copy[i], SIGUSR1);
                }
            } else {
                while (!got_usr2 && !got_stop) sigsuspend(&oset);
                if (!got_stop) voter_run(info, target, sem_mutex);
            }
        } else {
            while (!got_usr2 && !got_stop) sigsuspend(&oset);
            if (!got_stop) voter_run(info, target, sem_mutex);
        }
        got_usr2 = 0;
    }
    
    sem_wait(sem_mutex);
    int remaining = remove_pid_from_network(info);

    if (remaining == 0) {
        sem_close(sem_mutex);
        sem_unlink(SEM_MUTEX);
        sem_close(sem_winner);
        sem_unlink(SEM_WINNER);
        shm_unlink(SHM_NET_NAME);
        
        /* Enviamos el bloque de finalización a la cola de mensajes */
        Block bloque_fin;
        bloque_fin.target = 0; 
        bloque_fin.solution = 0;
        bloque_fin.valid = 0;
        bloque_fin.is_end = 1;
        
        if (write(pipe_write_fd, &bloque_fin, sizeof(Block)) < 0) {
            perror("write sentinel pipe");
        }
    } else {
        sem_post(sem_mutex);
        sem_close(sem_mutex);
        sem_close(sem_winner);
    }

    printf("Miner %d final wallet: %d coins\n", (int)getpid(), coins);
    
    munmap(info, sizeof(MinerInfo));
    
    return EXIT_SUCCESS;
}