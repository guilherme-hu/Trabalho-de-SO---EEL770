// barbershop.h

#ifndef BARBERSHOP_H
#define BARBERSHOP_H

#include <pthread.h>
#include <stdbool.h>

// --- ESTRUTURAS DE CONFIGURAÇÃO E FILA ---
typedef struct {
    int TOTAL_CLIENTES_A_GERAR;
    int NUM_BARBEIROS;
    int CAPACIDADE_MAX_LOJA;
    int CAPACIDADE_SOFA;
    
    // Nomes de variáveis atualizados, sem sufixo _S
    int MAX_TEMPO_CHEGADA_CLIENTE;
    int MAX_TEMPO_CORTE;
    int MAX_TEMPO_PAGAMENTO;
    
} Config;

typedef struct {
    int* items;
    int capacity;
    int front;
    int rear;
    int size;
} Queue;

// --- ESTRUTURA PRINCIPAL DA BARBEARIA (MONITOR) ---
typedef struct {
    Config config;
    pthread_mutex_t lock;
    pthread_cond_t sofa_tem_lugar;
    pthread_cond_t barbeiro_esta_livre;
    pthread_cond_t trabalho_disponivel;
    pthread_cond_t caixa_registradora_livre;
    pthread_cond_t* corte_finalizado;
    pthread_cond_t* pagamento_finalizado;
    Queue fila_sofa;
    Queue fila_em_pe;
    Queue fila_pagamento;
    int clientes_na_loja;
    int barbeiros_livres;
    bool caixa_em_uso;
    int* barbeiro_atende_cliente;
    bool loja_fechada;
    int clientes_processados;
    int clientes_atendidos; // NOVO: Contador para clientes que não desistiram
    bool proxima_tarefa_eh_corte;
} Barbershop;

// --- ESTRUTURAS PARA ARGUMENTOS DE THREADS ---
typedef struct {
    int id;
    Barbershop* shop;
} ThreadArgs;

// --- PROTÓTIPOS DE FUNÇÕES ---
void* customer_thread(void* args);
void* barber_thread(void* args);
void verificar_e_fechar_loja(Barbershop* shop);
void init_barbershop(Barbershop* shop, Config cfg);
void destroy_barbershop(Barbershop* shop);

#endif // BARBERSHOP_H
