#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_random() {
    srand(time(NULL));
}

int random_int(int max) {
    if (max <= 0) return 1;
    return (rand() % max) + 1;
}

void log_message(const char* level, const char* message, ...) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    pthread_mutex_lock(&printf_mutex);
    
    printf("[%02d:%02d:%02d] [%s] ", t->tm_hour, t->tm_min, t->tm_sec, level);
    
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    
    printf("\n");
    
    pthread_mutex_unlock(&printf_mutex);
}
