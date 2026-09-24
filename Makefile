# Makefile – Practica 3 SOPER 2025-26
# Autores: Guillermo Fernandez, Nicolò Giannini

CC     = gcc
CFLAGS = -g -Wall -Wextra
LDLIBS = -pthread -lrt

TARGETS      = miner monitor
MINER_OBJS   = miner.o pow.o miner_help.o miner_worker.o pid_registry.o voting.o
MONITOR_OBJS = monitor.o pow.o

all: $(TARGETS)

miner: $(MINER_OBJS)
	$(CC) $(CFLAGS) -o miner $(MINER_OBJS) $(LDLIBS)

monitor: $(MONITOR_OBJS)
	$(CC) $(CFLAGS) -o monitor $(MONITOR_OBJS) $(LDLIBS)

miner.o: miner.c
	$(CC) $(CFLAGS) -c miner.c

miner_help.o: miner_help.c
	$(CC) $(CFLAGS) -c miner_help.c

miner_worker.o: miner_worker.c
	$(CC) $(CFLAGS) -c miner_worker.c

pid_registry.o: pid_registry.c
	$(CC) $(CFLAGS) -c pid_registry.c

voting.o: voting.c
	$(CC) $(CFLAGS) -c voting.c

pow.o: pow.c
	$(CC) $(CFLAGS) -c pow.c

monitor.o: monitor.c
	$(CC) $(CFLAGS) -c monitor.c

clean:
	rm -f *.o $(TARGETS) *.pid *.tgt *.vot *.txt
	rm -f /dev/shm/sem.sem_miner_*
	rm -f /dev/mqueue/mq_miner_monitor
	rm -f /dev/shm/shm_miner_monitor /dev/shm/shm_miner_network

.PHONY: all clean