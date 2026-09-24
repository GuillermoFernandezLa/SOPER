/**
 * @file miner_help.h
 * @author Guillermo Fernandez
 * @author Nicolò Giannini
 * @brief Cabecera de la logica principal del minero.
 * @date 04-05-2026
 */

#ifndef MINER_HELP_H
#define MINER_HELP_H

/**
 * @brief Ejecuta toda la logica de red, señales y minado.
 * 
 * @param n_secs Segundos que el minero estara activo.
 * @param n_threads Numero de hilos para la busqueda de la solucion.
 * @param pipe_write_fd Descriptor de escritura de la tuberia para mandar datos al Registrador.
 * @return EXIT_SUCCESS si termina correctamente, EXIT_FAILURE en caso de error.
 */
int run_miner_network_logic(int n_secs, int n_threads, int pipe_write_fd);

#endif /* MINER_HELP_H */