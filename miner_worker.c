/**
 * @file miner_worker.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Implementacion del minado multihilo de una prueba de esfuerzo por fuerza bruta.
 * @date 04-05-2026
 */

#include "pow.h"
#include "miner_worker.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>


struct _shared_data_t {
    long            target;   /* Target hash a encontrar en esta ronda */
    long            solution; /* Solucion encontrada por algun hilo */
    volatile int    found;    /* Flag que indica si ya se encontro la solucion */
    pthread_mutex_t mutex;    /* Mutex para proteger solution y found */
};

struct _thread_arg_t {
    long           start;  /* Primer valor (inclusive) del rango de busqueda */
    long           end;    /* Ultimo valor (exclusive) del rango de busqueda */
    shared_data_t *shared; /* Puntero al bloque de datos compartidos */
};


/**
 * @brief Busca la solucion POW en el rango [start, end) asignado al hilo.
 * @param arg Puntero a una estructura thread_arg_t con el rango y datos compartidos
 * @return void* Siempre NULL
 */
void *thread_mine(void *arg) {
    thread_arg_t  *targ   = (thread_arg_t *)arg;
    shared_data_t *shared = targ->shared;

    for (long i = targ->start; i < targ->end; i++) {

        if (shared->found)
            return NULL;

        if (pow_hash(i) == shared->target) {
            pthread_mutex_lock(&shared->mutex);
            if (!shared->found) {
                shared->found    = 1;
                shared->solution = i;
            }
            pthread_mutex_unlock(&shared->mutex);
            return NULL;
        }
    }

    return NULL;
}

/**
 * @brief Ejecuta una sola ronda de minado creando n_threads hilos en paralelo.
 *
 * @param target Target hash que se debe encontrar
 * @param n_threads Numero de hilos paralelos a crear
 * @param solution_out Parametro de salida donde se escribe la solucion encontrada
 * @return EXIT_SUCCESS si se encontro solucion, EXIT_FAILURE en caso de error
 */
int miner_run(long target, int n_threads, long *solution_out) {

    shared_data_t shared;
    shared.target   = target;
    shared.solution = -1;
    shared.found    = 0;

    if (pthread_mutex_init(&shared.mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        return EXIT_FAILURE;
    }

    pthread_t    *threads = malloc((size_t)n_threads * sizeof(pthread_t));
    thread_arg_t *args    = malloc((size_t)n_threads * sizeof(thread_arg_t));

    if (!threads || !args) {
        perror("malloc");
        free(threads);
        free(args);
        pthread_mutex_destroy(&shared.mutex);
        return EXIT_FAILURE;
    }

    long chunk = POW_LIMIT / n_threads;

    for (int t = 0; t < n_threads; t++) {
        args[t].start  = (long)t * chunk;
        args[t].end    = (t == n_threads - 1) ? POW_LIMIT : (long)(t + 1) * chunk;
        args[t].shared = &shared;

        if (pthread_create(&threads[t], NULL, thread_mine, &args[t]) != 0) {
            fprintf(stderr, "pthread_create hilo %d: %s\n", t, strerror(errno));
            for (int j = 0; j < t; j++)
                pthread_join(threads[j], NULL);
            free(threads);
            free(args);
            pthread_mutex_destroy(&shared.mutex);
            return EXIT_FAILURE;
        }
    }

    for (int t = 0; t < n_threads; t++)
        pthread_join(threads[t], NULL);

    free(threads);
    free(args);
    pthread_mutex_destroy(&shared.mutex);

    if (!shared.found) {
        fprintf(stderr, "Error: no se encontro solucion para target %08ld\n", target);
        return EXIT_FAILURE;
    }

    *solution_out = shared.solution;
    return EXIT_SUCCESS;
}
