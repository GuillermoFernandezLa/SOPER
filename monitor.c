/**
 * @file monitor.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Punto de entrada del ejecutable monitor.
 * @date 04-05-2026
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "monitor_common.h"
#include "pow.h"

#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <LAG_COMPROBADOR> <LAG_MONITOR>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int lag_comprobador = atoi(argv[1]);
    int lag_monitor     = atoi(argv[2]);

    if (lag_comprobador < 0 || lag_monitor < 0) {
        fprintf(stderr, "Error: los lags deben ser enteros no negativos\n");
        return EXIT_FAILURE;
    }

    /* Creamos la memoria compartida */
    int fd_shm = shm_open(SHM_MONITOR_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm == -1) {
        if (errno == EEXIST) fprintf(stderr, "Error: ya hay un monitor en ejecucion\n");
        else perror("shm_open");
        return EXIT_FAILURE;
    }

    /* Inicializamos el tamaño de la memoria compartida */
    if (ftruncate(fd_shm, sizeof(MonitorShm)) == -1) {
        perror("ftruncate");
        close(fd_shm);
        shm_unlink(SHM_MONITOR_NAME);
        return EXIT_FAILURE;
    }

    /* Mapeamos la memoria compartida */
    MonitorShm *shm = mmap(NULL, sizeof(MonitorShm), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (shm == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_MONITOR_NAME);
        return EXIT_FAILURE;
    }

    shm->in = 0;
    shm->out = 0;
    shm->num_usuarios = 0;
    
    /* Inicializamos el buffer circular */
    for (int i = 0; i < MAX_MINERS; i++) {
        shm->carteras[i] = 0;
        shm->usuarios[i] = 0;
    }

    /* Inicializamos los semáforos */
    if (sem_init(&(shm->sem_empty), 1, BUFFER_SIZE) == -1 ||
        sem_init(&(shm->sem_fill),  1, 0)           == -1 ||
        sem_init(&(shm->sem_mutex), 1, 1)           == -1) {
        perror("sem_init");
        munmap(shm, sizeof(MonitorShm));
        shm_unlink(SHM_MONITOR_NAME);
        return EXIT_FAILURE;
    }

    /* Creamos la cola de mensajes */
    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = MQ_CAPACITY;
    attr.mq_msgsize = sizeof(Block);
    attr.mq_curmsgs = 0;

    mqd_t mq = mq_open(MQ_MONITOR_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        munmap(shm, sizeof(MonitorShm));
        shm_unlink(SHM_MONITOR_NAME);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(shm, sizeof(MonitorShm));
        shm_unlink(SHM_MONITOR_NAME);
        mq_unlink(MQ_MONITOR_NAME);
        return EXIT_FAILURE;
    }

    /* Proceso hijo: Monitor */
    if (pid == 0) {
        printf("[%d] Monitor iniciado. Esperando bloques...\n", (int)getpid());
        
        while (1) {
            Block b;

            /* Esperamos a que haya bloques disponibles */
            sem_wait(&(shm->sem_fill));
            sem_wait(&(shm->sem_mutex));

            /* Extraemos del buffer circular */
            b = shm->buffer[shm->out];
            shm->out = (shm->out + 1) % BUFFER_SIZE;

            /* Liberamos el mutex y el espacio vacío */
            sem_post(&(shm->sem_mutex));
            sem_post(&(shm->sem_empty));

            /* Verificamos si es el bloque de finalización */
            if (b.is_end) {
                break;
            }

            /* Mostramos por pantalla con la sintaxis del manual */
            if (b.valid) {
                printf("Solution accepted: %08ld --> %08ld\n", b.target, b.solution);
            } else {
                printf("Solution rejected: %08ld !-> %08ld\n", b.target, b.solution);
            }
            
            usleep(lag_monitor * 1000);
        }

        /* Liberamos la memoria compartida */
        munmap(shm, sizeof(MonitorShm));
        exit(EXIT_SUCCESS);
    }

    /* Proceso padre: Comprobador */
    while (1) {
        Block b;
        
        /* Recibimos el bloque de la cola de mensajes */
        if (mq_receive(mq, (char *)&b, sizeof(Block), NULL) < 0) {
            continue;
        }

        /* Validamos del bloque */
        if (!b.is_end) {
            b.valid = (pow_hash(b.solution) == b.target);
        }

        /* Insertamos el bloque en el buffer circular */
        sem_wait(&(shm->sem_empty));
        sem_wait(&(shm->sem_mutex));

        /* Actualizamos las informaciones del bloque */
        if (!b.is_end && b.miner_id >= 0 && b.miner_id < MAX_MINERS) {
            shm->carteras[b.miner_id] = b.coins;
            shm->usuarios[b.miner_id] = b.miner_pid;
            
            /* Cuenta usuarios activos */
            int activos = 0;
            for (int i = 0; i < MAX_MINERS; i++) {
                if (shm->usuarios[i] > 0) {
                    activos++;
                }
            }
            shm->num_usuarios = activos;
        }

        /* Escribimos el bloque validado en el buffer circular */
        shm->buffer[shm->in] = b;
        shm->in = (shm->in + 1) % BUFFER_SIZE;

        /* Liberamos el mutex y el espacio lleno */
        sem_post(&(shm->sem_mutex));
        sem_post(&(shm->sem_fill));

        /* Verificamos si es el bloque de finalización */
        if (b.is_end) {
            break;
        }

        usleep(lag_comprobador * 1000);
    }

    waitpid(pid, NULL, 0);

    mq_close(mq);
    sem_destroy(&(shm->sem_empty));
    sem_destroy(&(shm->sem_fill));
    sem_destroy(&(shm->sem_mutex));
    munmap(shm, sizeof(MonitorShm));
    shm_unlink(SHM_MONITOR_NAME);
    mq_unlink(MQ_MONITOR_NAME);

    printf("[%d] Finishing\n", (int)getpid());
    return EXIT_SUCCESS;
}