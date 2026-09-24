/**
 * @file miner.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Punto de entrada del sistema. Crea el proceso Registrador con fork y ejecuta
 * la logica del minero.
 * @date 04-05-2026
 */

#include "miner_help.h"
#include "monitor_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <fcntl.h>

/**
 * @brief Proceso hijo: lee de la tuberia y manda mensajes a la cola del monitor.
 * 
 * @param read_fd Descriptor de lectura de la tuberia conectada al minero.
 */
void logger_process(int read_fd) {
    
    /* Abrimos la cola de mensajes */
    mqd_t mq = mq_open(MQ_MONITOR_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open (Registrador)");
        close(read_fd);
        exit(EXIT_FAILURE);
    }

    Block bloque;
    
    /* Leemos de la tuberia tamaño exacto de un Block */
    while (read(read_fd, &bloque, sizeof(Block)) > 0) {
        
        /* Enviamos el bloque a la cola de mensajes */
        if (mq_send(mq, (const char *)&bloque, sizeof(Block), 0) == -1) {
            perror("mq_send (Registrador)");
        }

        if (bloque.is_end == 1) {
            break;
        }
    }
    
    mq_close(mq);
    close(read_fd);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N_SECS> <N_THREADS>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n_secs    = atoi(argv[1]);
    int n_threads = atoi(argv[2]);

    if (n_secs <= 0 || n_threads <= 0) {
        fprintf(stderr, "Error: N_SECS y N_THREADS deben ser > 0\n");
        return EXIT_FAILURE;
    }

    int pipe_m2l[2];
    if (pipe(pipe_m2l) < 0) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipe_m2l[0]);
        close(pipe_m2l[1]);
        return EXIT_FAILURE;
    }
  
    if (pid == 0) {
        close(pipe_m2l[1]);
        logger_process(pipe_m2l[0]); 
        return EXIT_SUCCESS; 
    }

    close(pipe_m2l[0]);

    /* Ejecutar logica del minero pasandole el fd de escritura */
    int status = run_miner_network_logic(n_secs, n_threads, pipe_m2l[1]);

    close(pipe_m2l[1]);
    waitpid(pid, NULL, 0);

    return status;
}