/**
 * @file voting.h
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Implementacion de la logica de votacion de los mineros.
 * @date 04-05-2026
 */

#ifndef VOTING_H
#define VOTING_H

#include <semaphore.h>
#include "pid_registry.h"

/**
 * @brief Funcion ejecutada por el minero ganador para esperar los votos de los demas mineros y escribir el resultado en el log.
 * 
 * @param net Puntero a la estructura compartida con la informacion de los mineros y votos
 * @param target Target hash que se esta votando
 * @param solution Solucion propuesta por el candidato
 * @param round Numero de ronda de votacion
 * @param coins Puntero al numero de monedas del minero ganador, que se actualiza segun el resultado de la votacion
 * @param sem_mutex Semaforo para proteger el acceso a la estructura compartida
 * @param pipe_fd Descriptor de la tuberia para enviar el bloque con el resultado al monitor
 */
void candidate_run(MinerInfo *net, long target, long solution, int round, int *coins, sem_t *sem_mutex);

/**
 * @brief Funcion ejecutada por cada minero para votar sobre la solucion propuesta por un candidato.
 * 
 * @param net Puntero a la estructura compartida con la informacion de los mineros y votos
 * @param target Target hash que se esta votando
 * @param sem_mutex Semaforo para proteger el acceso a la estructura compartida
 */
void voter_run(MinerInfo *net, long target, sem_t *sem_mutex);

#endif /* VOTING_H */