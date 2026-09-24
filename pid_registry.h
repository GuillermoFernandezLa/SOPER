/**
 * @file pid_registry.h
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Punto de entrada del ejecutable monitor. Crea los recursos
 * @date 04-05-2026
 */

#ifndef PID_REGISTRY_H
#define PID_REGISTRY_H

#include <sys/types.h>

#define MAX_MINERS 100

typedef struct {
    long  target;
    int   num_mineros;
    pid_t pids[MAX_MINERS];
    int   votes_accept;
    int   votes_reject;
} MinerInfo;

/* Función para agregar un PID a la red 
 * @param net Puntero a la estructura de información de la red
 * @return 0 si se agregó correctamente, -1 si se alcanzó el límite de mineros
 */
int add_pid_to_network(MinerInfo *net);

/* Función para eliminar un PID de la red 
 * @param net Puntero a la estructura de información de la red
 * @return Número de mineros activos después de la eliminación
 */
int remove_pid_from_network(MinerInfo *net);

/* Función para obtener el número de PIDs en la red
 * @param net Puntero a la estructura de información de la red
 * @return Número de mineros activos
 */
int get_number_pids(MinerInfo *net);

#endif /* PID_REGISTRY_H */