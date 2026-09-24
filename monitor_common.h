/**
 * @file monitor_common.h
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Definiciones comunes del sistema de monitorizacion.
 * @date 04-05-2026
 */

#ifndef MONITOR_COMMON_H
#define MONITOR_COMMON_H

#include <semaphore.h>
#include <sys/types.h>

#define SHM_MONITOR_NAME "/shm_miner_monitor"
#define MQ_MONITOR_NAME  "/mq_miner_monitor"

#define BUFFER_SIZE 6
#define MQ_CAPACITY 7
#define MAX_MINERS 100

/**
 * @brief Bloque de informacion enviado por un minero ganador.
 */
typedef struct {
    long target;
    long solution;
    int valid;
    int is_end;
    int miner_id;
    pid_t miner_pid;
    int coins;
    int votes_accept;
    int votes_reject;
} Block;

/**
 * @brief Segmento de memoria compartida completo.
 */
typedef struct {
    Block buffer[BUFFER_SIZE];
    int in;
    int out;
    sem_t sem_empty;
    sem_t sem_fill;
    sem_t sem_mutex;
    int carteras[MAX_MINERS];
    pid_t usuarios[MAX_MINERS];
    int num_usuarios;
} MonitorShm;

#endif /* MONITOR_COMMON_H */