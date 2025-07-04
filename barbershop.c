// barbershop.c

#include "barbershop.h"
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// --- FUNÇÕES DE FILA (sem alterações) ---
void init_queue(Queue* q, int capacity) {
    q->items = malloc(sizeof(int) * capacity);
    if (!q->items) { perror("malloc"); exit(EXIT_FAILURE); }
    q->capacity = capacity; q->front = 0; q->rear = 0; q->size = 0;
}
void destroy_queue(Queue* q) { free(q->items); }
void enqueue(Queue* q, int item) {
    if (q->size == q->capacity) return;
    q->items[q->rear] = item;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
}
int dequeue(Queue* q) {
    if (q->size == 0) return -1;
    int item = q->items[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return item;
}
int peek_front(Queue* q) {
    if (q->size == 0) return -1;
    return q->items[q->front];
}


// --- FUNÇÕES DE INICIALIZAÇÃO E DESTRUIÇÃO (adicionado inicialização do novo contador) ---
void init_barbershop(Barbershop* shop, Config cfg) {
    shop->config = cfg;
    pthread_mutex_init(&shop->lock, NULL);
    pthread_cond_init(&shop->sofa_tem_lugar, NULL);
    pthread_cond_init(&shop->barbeiro_esta_livre, NULL);
    pthread_cond_init(&shop->trabalho_disponivel, NULL);
    pthread_cond_init(&shop->caixa_registradora_livre, NULL);

    shop->corte_finalizado = malloc(sizeof(pthread_cond_t) * cfg.NUM_BARBEIROS);
    shop->pagamento_finalizado = malloc(sizeof(pthread_cond_t) * cfg.TOTAL_CLIENTES_A_GERAR);
    shop->barbeiro_atende_cliente = malloc(sizeof(int) * cfg.NUM_BARBEIROS);

    for (int i = 0; i < cfg.NUM_BARBEIROS; i++) {
        pthread_cond_init(&shop->corte_finalizado[i], NULL);
        shop->barbeiro_atende_cliente[i] = -1;
    }
    for (int i = 0; i < cfg.TOTAL_CLIENTES_A_GERAR; i++) {
        pthread_cond_init(&shop->pagamento_finalizado[i], NULL);
    }
    init_queue(&shop->fila_sofa, cfg.CAPACIDADE_SOFA);
    init_queue(&shop->fila_em_pe, cfg.CAPACIDADE_MAX_LOJA);
    init_queue(&shop->fila_pagamento, cfg.CAPACIDADE_MAX_LOJA);
    shop->clientes_na_loja = 0;
    shop->barbeiros_livres = cfg.NUM_BARBEIROS;
    shop->caixa_em_uso = false;
    shop->loja_fechada = false;
    shop->clientes_processados = 0;
    shop->clientes_atendidos = 0;
    shop->proxima_tarefa_eh_corte = true;
}

void destroy_barbershop(Barbershop* shop) {
    pthread_mutex_destroy(&shop->lock);
    pthread_cond_destroy(&shop->sofa_tem_lugar);
    pthread_cond_destroy(&shop->barbeiro_esta_livre);
    pthread_cond_destroy(&shop->trabalho_disponivel);
    pthread_cond_destroy(&shop->caixa_registradora_livre);
    for (int i = 0; i < shop->config.NUM_BARBEIROS; i++) pthread_cond_destroy(&shop->corte_finalizado[i]);
    for (int i = 0; i < shop->config.TOTAL_CLIENTES_A_GERAR; i++) pthread_cond_destroy(&shop->pagamento_finalizado[i]);
    free(shop->corte_finalizado);
    free(shop->pagamento_finalizado);
    free(shop->barbeiro_atende_cliente);
    destroy_queue(&shop->fila_sofa);
    destroy_queue(&shop->fila_em_pe);
    destroy_queue(&shop->fila_pagamento);
}


// --- LÓGICA DAS THREADS (COMPLETA E CORRIGIDA) ---

void verificar_e_fechar_loja(Barbershop* shop) {
    pthread_mutex_lock(&shop->lock);
    if (shop->clientes_processados >= shop->config.TOTAL_CLIENTES_A_GERAR && !shop->loja_fechada) {
        shop->loja_fechada = true;
        log_message("LOJA", "TODOS OS CLIENTES FORAM PROCESSADOS! FECHANDO AS PORTAS.");
        pthread_cond_broadcast(&shop->trabalho_disponivel);
    }
    pthread_mutex_unlock(&shop->lock);
}

void* customer_thread(void* args) {
    ThreadArgs* thread_args = (ThreadArgs*)args;
    Barbershop* shop = thread_args->shop;
    int id = thread_args->id;
    free(args);

    pthread_mutex_lock(&shop->lock);

    if (shop->loja_fechada || shop->clientes_na_loja >= shop->config.CAPACIDADE_MAX_LOJA) {
        log_message("AVISO", "*** Cliente %d DESISTIU (Loja cheia ou fechada). ***", id + 1);
        shop->clientes_processados++;
        pthread_mutex_unlock(&shop->lock);
        verificar_e_fechar_loja(shop);
        return NULL;
    }
    shop->clientes_na_loja++;
    // LOG MELHORADO
    log_message("CLIENTE", "Cliente %d entrou. (Total: %d | Sofá: %d/%d)", 
        id + 1, shop->clientes_na_loja, shop->fila_sofa.size, shop->config.CAPACIDADE_SOFA);

    if (shop->fila_sofa.size < shop->config.CAPACIDADE_SOFA) {
        enqueue(&shop->fila_sofa, id);
        log_message("CLIENTE", "Cliente %d sentou direto no sofá.", id + 1);
        pthread_cond_signal(&shop->trabalho_disponivel);
    } else {
        enqueue(&shop->fila_em_pe, id);
        log_message("CLIENTE", "Cliente %d está em pé, aguardando vaga no sofá.", id + 1);
        while (peek_front(&shop->fila_em_pe) != id || shop->fila_sofa.size >= shop->config.CAPACIDADE_SOFA) {
            pthread_cond_wait(&shop->sofa_tem_lugar, &shop->lock);
        }
        dequeue(&shop->fila_em_pe);
        enqueue(&shop->fila_sofa, id);
        log_message("CLIENTE", "Cliente %d moveu-se da fila de pé para o sofá.", id + 1);
        pthread_cond_signal(&shop->trabalho_disponivel);
    }

    int meu_barbeiro_id = -1;
    log_message("CLIENTE", "Cliente %d aguarda na poltrona do sofá por um barbeiro.", id + 1);
    while (meu_barbeiro_id == -1) {
        // CORREÇÃO: A lógica de espera do cliente foi simplificada para aguardar a reivindicação do barbeiro
        // A condição do `while` no barbeiro é o que realmente controla quem é atendido
        pthread_cond_wait(&shop->barbeiro_esta_livre, &shop->lock);
        for(int i=0; i<shop->config.NUM_BARBEIROS; ++i) {
            if(shop->barbeiro_atende_cliente[i] == id) {
                meu_barbeiro_id = i;
                break;
            }
        }
    }
    
    // O cliente foi pego pelo barbeiro, então ele sai da fila do sofá. O barbeiro faz o dequeue.
    log_message("CLIENTE", "Cliente %d está indo cortar o cabelo com o Barbeiro %d.", id + 1, meu_barbeiro_id + 1);
    pthread_cond_wait(&shop->corte_finalizado[meu_barbeiro_id], &shop->lock);
    
    log_message("CLIENTE", "Cliente %d terminou o corte.", id + 1);

    enqueue(&shop->fila_pagamento, id);
    log_message("CLIENTE", "Cliente %d entrou na fila de pagamento. (Fila pag.: %d)", id + 1, shop->fila_pagamento.size);
    pthread_cond_signal(&shop->trabalho_disponivel);
    
    pthread_cond_wait(&shop->pagamento_finalizado[id], &shop->lock);
    
    shop->clientes_na_loja--;
    shop->clientes_processados++;
    shop->clientes_atendidos++;
    log_message("CLIENTE", "Cliente %d PAGOU E SAIU. (Total na loja: %d)", id + 1, shop->clientes_na_loja);
    
    pthread_mutex_unlock(&shop->lock);
    verificar_e_fechar_loja(shop);
    return NULL;
}

void* barber_thread(void* args) {
    ThreadArgs* thread_args = (ThreadArgs*)args;
    Barbershop* shop = thread_args->shop;
    int id = thread_args->id;
    free(args);

    log_message("BARBEIRO", "Barbeiro %d começou o expediente.", id + 1);

    while (true) {
        pthread_mutex_lock(&shop->lock);

        while (shop->fila_sofa.size == 0 && shop->fila_pagamento.size == 0 && !shop->loja_fechada) {
            log_message("BARBEIRO", "Barbeiro %d não vê trabalho e vai dormir...", id + 1);
            pthread_cond_wait(&shop->trabalho_disponivel, &shop->lock);
        }

        if (shop->fila_sofa.size == 0 && shop->fila_pagamento.size == 0 && shop->loja_fechada) {
            pthread_mutex_unlock(&shop->lock);
            break; 
        }

        if(shop->fila_sofa.size > 0 || shop->fila_pagamento.size > 0) {
            log_message("BARBEIRO", "Barbeiro %d procura tarefa... (Sofá: %d, Pagamento: %d)",
                id + 1, shop->fila_sofa.size, shop->fila_pagamento.size);
            
            bool ha_cliente_para_corte = (shop->fila_sofa.size > 0);
            bool ha_cliente_para_pagar = (shop->fila_pagamento.size > 0);
            
            if ((shop->proxima_tarefa_eh_corte && ha_cliente_para_corte) || (!ha_cliente_para_pagar && ha_cliente_para_corte)) {
                shop->proxima_tarefa_eh_corte = false;
                
                int cliente_id = dequeue(&shop->fila_sofa);
                // CORREÇÃO DO DEADLOCK: Sinaliza que uma vaga no sofá foi liberada
                pthread_cond_broadcast(&shop->sofa_tem_lugar);

                shop->barbeiros_livres--;
                shop->barbeiro_atende_cliente[id] = cliente_id;
                
                pthread_cond_broadcast(&shop->barbeiro_esta_livre);
                pthread_mutex_unlock(&shop->lock);

                log_message("BARBEIRO", "Barbeiro %d está cortando cabelo do Cliente %d.", id + 1, cliente_id + 1);
                sleep(random_int(shop->config.MAX_TEMPO_CORTE));
                
                pthread_mutex_lock(&shop->lock);
                log_message("BARBEIRO", "Barbeiro %d terminou o corte do Cliente %d.", id + 1, cliente_id + 1);
                pthread_cond_signal(&shop->corte_finalizado[id]);
                shop->barbeiro_atende_cliente[id] = -1;

            } else if (ha_cliente_para_pagar) {
                shop->proxima_tarefa_eh_corte = true;
                int cliente_id = dequeue(&shop->fila_pagamento);

                while (shop->caixa_em_uso) {
                    pthread_cond_wait(&shop->caixa_registradora_livre, &shop->lock);
                }
                shop->caixa_em_uso = true;
                
                pthread_mutex_unlock(&shop->lock);

                log_message("BARBEIRO", "Barbeiro %d está recebendo pagamento do Cliente %d.", id + 1, cliente_id + 1);
                sleep(random_int(shop->config.MAX_TEMPO_PAGAMENTO));

                pthread_mutex_lock(&shop->lock);
                shop->caixa_em_uso = false;
                pthread_cond_signal(&shop->caixa_registradora_livre);
                pthread_cond_signal(&shop->pagamento_finalizado[cliente_id]);
                log_message("BARBEIRO", "Barbeiro %d finalizou pagamento do Cliente %d.", id + 1, cliente_id + 1);
                shop->barbeiros_livres++; 
            }
        }
        pthread_mutex_unlock(&shop->lock);
    }

    log_message("BARBEIRO", "Barbeiro %d encerrou o expediente.", id + 1);
    return NULL;
}
