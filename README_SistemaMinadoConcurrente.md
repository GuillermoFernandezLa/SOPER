# ⛏️ Sistema de Minado Concurrente

Sistema de minería de criptomonedas **multiproceso y multihilo** desarrollado en **C** para la asignatura de Sistemas Operativos, a lo largo de tres prácticas encadenadas (P1 → P2 → P3), cada una ampliando la funcionalidad de la anterior.

## 🚀 Descripción del proyecto

El sistema simula un entorno de minado distribuido en el que varios procesos colaboran y compiten entre sí:
- Los **mineros** compiten por resolver bloques válidos (proof of work).
- Un proceso **monitor** coordina el estado compartido del sistema.
- Un proceso **checker** valida los bloques ganadores recibidos.
- Un proceso **display** muestra el estado del sistema en tiempo real.
- Procesos adicionales (**candidate**, **voter**, **pid_registry**) gestionan la selección de candidatos y el registro de procesos activos.

Toda la comunicación entre procesos se implementa exclusivamente con mecanismos **POSIX**: memoria compartida, semáforos, colas de mensajes y señales.

## 🛠️ Tecnologías

- **Lenguaje**: C (estándar POSIX, `_POSIX_C_SOURCE 200809L`)
- **Concurrencia**: procesos (`fork`), hilos (`pthreads`)
- **Comunicación entre procesos (IPC)**:
  - Memoria compartida POSIX (`shm_open`) para el estado del sistema y un buffer circular productor-consumidor
  - Semáforos POSIX no nombrados (`sem_init`) para la sincronización
  - Cola de mensajes POSIX para el envío de bloques desde los mineros al *checker*

## 📂 Estructura principal

| Archivo | Responsabilidad |
|---|---|
| `monitor.c` | Gestión del estado compartido y coordinación general |
| `checker.c` | Validación de los bloques recibidos |
| `display.c` | Visualización del estado del sistema |
| `miner.c` / `miner_worker.c` | Lógica de minado (proof of work) |
| `candidate.c` | Gestión de procesos candidatos |
| `voter.c` | Votación/selección entre candidatos |
| `pid_registry.c` | Registro de procesos activos |
| `pow.c` | Cálculo del proof of work |

## 📦 Compilación y ejecución

1. **Clonar el repositorio**
   ```bash
   git clone https://github.com/tuusuario/sistema-minado-concurrente.git
   cd sistema-minado-concurrente
   ```

2. **Compilar**
   ```bash
   make
   ```

3. **Ejecutar el sistema**
   ```bash
   ./monitor
   ```
   (los procesos de minero, checker y display se lanzan según la configuración indicada en el enunciado de la práctica)

> Nota: los parámetros exactos de ejecución dependen de la configuración final definida en cada práctica (P1, P2, P3).
