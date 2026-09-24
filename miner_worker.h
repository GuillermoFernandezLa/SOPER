/**
 * @file miner_worker.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Implementacion del minado multihilo de una prueba de esfuerzo por fuerza bruta.
 * @date 04-05-2026
 */

#include <stdlib.h>
#include <sys/types.h>
#ifndef MINER_WORKER_H
#define MINER_WORKER_H

/* Opaque type aliases for the internal structures defined in process.c */
typedef struct _shared_data_t shared_data_t;
typedef struct _thread_arg_t  thread_arg_t;

/**
 * @brief Funcion ejecutada por cada hilo para buscar la solucion POW.
 *
 * @param arg Puntero a una estructura thread_arg_t con el rango de busqueda
 * @return void* Siempre NULL
 */
void *thread_mine(void *arg);

/**
 * @brief Ejecuta una sola ronda de minado usando n_threads hilos en paralelo.
 *
 * @param target Target hash que se debe encontrar en esta ronda
 * @param n_threads Numero de hilos paralelos a crear
 * @param solution_out Parametro de salida donde se escribe la solucion encontrada
 * @return EXIT_SUCCESS si se encontro solucion, EXIT_FAILURE en caso de error
 */
int miner_run(long target, int n_threads, long *solution_out);


#endif /* MINER_WORKER.H */