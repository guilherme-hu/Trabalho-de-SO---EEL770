// main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "barbershop.h"
#include "utils.h"

void load_config(const char* filename, Config* cfg) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir arquivo de configuração");
        exit(EXIT_FAILURE);
    }
    char line[100], key[50];
    int value;

    // Valores padrão
    cfg->MAX_TEMPO_CHEGADA_CLIENTE = 2;
    cfg->MAX_TEMPO_CORTE = 5;
    cfg->MAX_TEMPO_PAGAMENTO = 2;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
            // CORREÇÃO 4: Usa nomes de chaves sem sufixo _S
            if (strcmp(key, "TOTAL_CLIENTES_A_GERAR") == 0) cfg->TOTAL_CLIENTES_A_GERAR = value;
            else if (strcmp(key, "NUM_BARBEIROS") == 0) cfg->NUM_BARBEIROS = value;
            else if (strcmp(key, "CAPACIDADE_MAX_LOJA") == 0) cfg->CAPACIDADE_MAX_LOJA = value;
            else if (strcmp(key, "CAPACIDADE_SOFA") == 0) cfg->CAPACIDADE_SOFA = value;
            else if (strcmp(key, "MAX_TEMPO_CHEGADA_CLIENTE") == 0) cfg->MAX_TEMPO_CHEGADA_CLIENTE = value;
            else if (strcmp(key, "MAX_TEMPO_CORTE") == 0) cfg->MAX_TEMPO_CORTE = value;
            else if (strcmp(key, "MAX_TEMPO_PAGAMENTO") == 0) cfg->MAX_TEMPO_PAGAMENTO = value;
        }
    }
    fclose(file);
}

int main() {
    init_random();
    Config cfg;
    load_config("barbearia.conf", &cfg);
    Barbershop shop;
    init_barbershop(&shop, cfg);
    
    log_message("SISTEMA", "Barbearia aberta com %d barbeiros.", cfg.NUM_BARBEIROS);

    pthread_t barber_threads[cfg.NUM_BARBEIROS];
    pthread_t customer_threads[cfg.TOTAL_CLIENTES_A_GERAR];

    for (int i = 0; i < cfg.NUM_BARBEIROS; i++) {
        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->id = i;
        args->shop = &shop;
        pthread_create(&barber_threads[i], NULL, barber_thread, args);
    }

    for (int i = 0; i < cfg.TOTAL_CLIENTES_A_GERAR; i++) {
        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->id = i;
        args->shop = &shop;
        pthread_create(&customer_threads[i], NULL, customer_thread, args);
        if (cfg.MAX_TEMPO_CHEGADA_CLIENTE > 0) {
            sleep(random_int(cfg.MAX_TEMPO_CHEGADA_CLIENTE));
        }
    }
    
    for (int i = 0; i < cfg.NUM_BARBEIROS; i++) {
        pthread_join(barber_threads[i], NULL);
    }

    log_message("SISTEMA", "Simulação concluída. Barbearia fechada.");
    log_message("SISTEMA", "Resumo: %d de %d clientes foram atendidos (não desistiram).", shop.clientes_atendidos, shop.config.TOTAL_CLIENTES_A_GERAR);

    destroy_barbershop(&shop);
    return 0;
}
