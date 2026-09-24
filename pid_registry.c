/**
 * @file pid_registry.c
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Punto de entrada del ejecutable monitor. Crea los recursos
 * @date 04-05-2026
 */

#include "pid_registry.h"
#include <stdio.h>
#include <unistd.h>

/* Función para agregar un PID a la red 
 * @param net Puntero a la estructura de información de la red
 * @return 0 si se agregó correctamente, -1 si se alcanzó el límite de mineros
 */
int add_pid_to_network(MinerInfo *net) {
    if (net->num_mineros >= MAX_MINERS) {
        fprintf(stderr, "Error: numero maximo de mineros alcanzado\n");
        return -1;
    }
    net->pids[net->num_mineros] = getpid();
    net->num_mineros++;

    printf("Miner %d added to system. Active miners: %d\n", (int)getpid(), net->num_mineros);
    return 0;
}

/* Función para eliminar un PID de la red 
 * @param net Puntero a la estructura de información de la red
 * @return Número de mineros activos después de la eliminación
 */
int remove_pid_from_network(MinerInfo *net) {
    pid_t my_pid = getpid();
    int pos = -1;

    for (int i = 0; i < net->num_mineros; i++) {
        if (net->pids[i] == my_pid) {
            pos = i;
            break;
        }
    }

    if (pos != -1) {
        net->num_mineros--;
        if (pos < net->num_mineros) {
            net->pids[pos] = net->pids[net->num_mineros];
        }
    }

    printf("Miner %d exited system. Active miners: %d\n", (int)my_pid, net->num_mineros);
    return net->num_mineros;
}

/* Función para obtener el número de PIDs en la red
 * @param net Puntero a la estructura de información de la red
 * @return Número de mineros activos
 */
int get_number_pids(MinerInfo *net) {
    return net->num_mineros;
}