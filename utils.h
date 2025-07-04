#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>

// Mutex para garantir que as saídas no console não se sobreponham
extern pthread_mutex_t printf_mutex;

// Inicializa o gerador de números aleatórios
void init_random();

// Retorna um número aleatório entre 1 e max (inclusivo)
int random_int(int max);

// Função de log thread-safe com timestamp
void log_message(const char* level, const char* message, ...);

#endif // UTILS_H
