#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymemory.h"

// Função para inicializar o pool de memória
mymemory_t* mymemory_init(size_t size, AllocationStrategy strategy) {
    mymemory_t *memory = (mymemory_t*)malloc(sizeof(mymemory_t)); // Aloca a estrutura principal de controle de memória
    memory->pool = malloc(size); // Aloca o pool de memória total disponível para alocações
    memory->total_size = size; // Define o tamanho total do pool de memória
    memory->head = (allocation_t*)malloc(sizeof(allocation_t)); // Inicializa o primeiro nó da lista de alocações
    memory->head->start = memory->pool; // Define o início do bloco como o início do pool
    memory->head->size = size; // Define o tamanho inicial do bloco como o tamanho total do pool
    memory->head->next = NULL; // Não há outro bloco após o primeiro, então o próximo é NULL
    memory->strategy = strategy; // Define a estratégia de alocação (First Fit, Best Fit, Worst Fit)
    return memory; // Retorna o ponteiro para a estrutura principal de controle de memória
}

// Função para alocar um bloco de memória usando a estratégia definida
void* mymemory_alloc(mymemory_t *memory, size_t size) {
    allocation_t *current = memory->head; // Ponteiro para iterar sobre os blocos da lista de alocação
    allocation_t *best_block = NULL; // Ponteiro para armazenar o melhor bloco encontrado (Best Fit)
    allocation_t *worst_block = NULL; // Ponteiro para armazenar o pior bloco encontrado (Worst Fit)
    void *allocated_start = NULL; // Ponteiro que armazenará o endereço inicial da memória alocada

    // Escolhe a estratégia de busca de bloco
    switch (memory->strategy) {
        case FIRST_FIT: // Estratégia de First Fit
            while (current) { // Percorre a lista de blocos
                if (current->size >= size) { // Se o bloco tem tamanho suficiente
                    allocated_start = current->start; // Define o início do bloco alocado
                    current->start += size; // Move o início do bloco para a próxima posição livre
                    current->size -= size; // Reduz o tamanho do bloco pelo tamanho alocado
                    return allocated_start; // Retorna o ponteiro para o bloco alocado
                }
                current = current->next; // Avança para o próximo bloco
            }
            break;

        case BEST_FIT: // Estratégia de Best Fit
            while (current) {
                if (current->size >= size && (!best_block || current->size < best_block->size)) {
                    best_block = current; // Armazena o bloco com o tamanho mais próximo do solicitado
                }
                current = current->next; // Avança para o próximo bloco
            }
            if (best_block) { // Se um bloco adequado foi encontrado
                allocated_start = best_block->start; // Define o início do bloco alocado
                best_block->start += size; // Move o início do bloco para a próxima posição livre
                best_block->size -= size; // Reduz o tamanho do bloco pelo tamanho alocado
                return allocated_start; // Retorna o ponteiro para o bloco alocado
            }
            break;

        case WORST_FIT: // Estratégia de Worst Fit
            while (current) {
                if (current->size >= size && (!worst_block || current->size > worst_block->size)) {
                    worst_block = current; // Armazena o maior bloco disponível
                }
                current = current->next; // Avança para o próximo bloco
            }
            if (worst_block) { // Se um bloco adequado foi encontrado
                allocated_start = worst_block->start; // Define o início do bloco alocado
                worst_block->start += size; // Move o início do bloco para a próxima posição livre
                worst_block->size -= size; // Reduz o tamanho do bloco pelo tamanho alocado
                return allocated_start; // Retorna o ponteiro para o bloco alocado
            }
            break;
    }

    return NULL; // Retorna NULL se não encontrar um bloco adequado
}

// Função para liberar um bloco de memória e marcar o espaço como disponível
void mymemory_free(mymemory_t *memory, void *ptr) {
    allocation_t *current = memory->head; // Ponteiro para iterar sobre os blocos
    allocation_t *prev = NULL; // Armazena o bloco anterior para facilitar a remoção

    // Procura o bloco pelo ponteiro de início
    while (current && current->start != ptr) { // Percorre a lista até encontrar o bloco
        prev = current;
        current = current->next;
    }

    // Se o bloco foi encontrado, remove-o e marca o espaço como disponível
    if (current) {
        if (prev) { // Se há um bloco anterior
            prev->next = current->next; // Remove o bloco atual da lista
        } else {
            memory->head = current->next; // Atualiza o head se o bloco era o primeiro
        }

        // Restaura o bloco liberado e coloca-o de volta na lista de memória livre
        current->start = ptr;
        current->size += current->size; // Libera o tamanho total do bloco
        current->next = memory->head; // Insere o bloco de volta no início da lista
        memory->head = current; // Atualiza o head para incluir o bloco liberado

        free(current); // Libera a estrutura de controle do bloco
    }
}

// Função para exibir todas as alocações atuais
void mymemory_display(mymemory_t *memory) {
    allocation_t *current = memory->head; // Ponteiro para iterar sobre os blocos
    printf("Alocações atuais:\n"); // Imprime o cabeçalho

    while (current) { // Percorre a lista de blocos
        printf("Início: %p, Tamanho: %lu\n", current->start, (unsigned long)current->size); // Imprime o endereço e o tamanho do bloco
        current = current->next; // Avança para o próximo bloco
    }
}

// Função para exibir estatísticas gerais da memória
void mymemory_stats(mymemory_t *memory) {
    allocation_t *current = memory->head; // Ponteiro para iterar sobre os blocos
    unsigned long total_allocated = 0, total_free = memory->total_size; // Inicializa as variáveis de estatísticas
    int alloc_count = 0; // Contador de alocações

    while (current) { // Percorre a lista de blocos
        total_allocated += current->size; // Soma o tamanho dos blocos alocados
        total_free -= current->size; // Subtrai do total de memória livre
        alloc_count++; // Incrementa o contador de alocações
        current = current->next; // Avança para o próximo bloco
    }

    // Exibe as estatísticas de memória
    printf("Estatísticas de memória:\n");
    printf("Total de alocações: %d\n", alloc_count);
    printf("Memória total alocada: %lu bytes\n", total_allocated);
    printf("Memória total livre: %lu bytes\n", total_free);
}

// Função para liberar o pool de memória e estruturas de controle
void mymemory_cleanup(mymemory_t *memory) {
    allocation_t *current = memory->head; // Ponteiro para iterar sobre os blocos
    while (current) { // Libera cada bloco na lista de alocação
        allocation_t *temp = current; // Armazena o bloco atual
        current = current->next; // Avança para o próximo bloco
        free(temp); // Libera o bloco atual
    }
    free(memory->pool); // Libera o pool de memória
    free(memory); // Libera a estrutura de controle principal
}

void display_menu() {
    // Exibe o menu principal de opções
    printf("\n--- Menu de Gerenciamento de Memoria ---\n");
    printf("1. Inicializar memoria\n");
    printf("2. Alocar memoria\n");
    printf("3. Liberar memoria\n");
    printf("4. Exibir alocacoes atuais\n");
    printf("5. Exibir estatisticas de memoria\n");
    printf("6. Limpar memoria e sair\n");
    printf("Escolha uma opcao: ");
}

int main() {
    mymemory_t *memory = NULL; // Ponteiro para a estrutura principal de controle de memória
    AllocationStrategy strategy; // Variável para armazenar a estratégia de alocação
    size_t pool_size, alloc_size; // Variáveis para o tamanho do pool e do bloco a alocar
    int option; // Opção escolhida pelo usuário
    void *allocated_ptr; // Ponteiro para armazenar o endereço do bloco alocado

    while (1) { // Loop principal do menu
        display_menu(); // Exibe o menu
        scanf("%d", &option); // Lê a opção do usuário

        switch (option) {
            case 1: // Inicializar memória
                if (memory != NULL) {
                    printf("Memoria ja foi inicializada. Limpe antes de reinicializar.\n");
                    break;
                }
                
                printf("Digite o tamanho do pool de memoria: ");
                scanf("%zu", &pool_size);
                printf("Escolha a estrategia de alocacao (0: First Fit, 1: Best Fit, 2: Worst Fit): ");
                int strat_option;
                scanf("%d", &strat_option);
                
                strategy = (strat_option == 1) ? BEST_FIT : (strat_option == 2) ? WORST_FIT : FIRST_FIT;
                memory = mymemory_init(pool_size, strategy);
                printf("Memoria inicializada com %zu bytes usando a estrategia %s.\n",
                       pool_size, (strategy == BEST_FIT ? "Best Fit" : (strategy == WORST_FIT ? "Worst Fit" : "First Fit")));
                break;

            case 2: // Alocar memória
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }
                
                printf("Digite o tamanho do bloco a ser alocado: ");
                scanf("%zu", &alloc_size);
                allocated_ptr = mymemory_alloc(memory, alloc_size);

                if (allocated_ptr != NULL) {
                    printf("Memoria alocada no endereco %p.\n", allocated_ptr);
                } else {
                    printf("Falha ao alocar memoria. Tente um tamanho menor ou verifique o pool disponivel.\n");
                }
                break;

            case 3: // Liberar memória
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }

                printf("Digite o endereco (em hexadecimal) do bloco a ser liberado: ");
                scanf("%p", &allocated_ptr);
                mymemory_free(memory, allocated_ptr);
                printf("Memoria no endereco %p liberada.\n", allocated_ptr);
                break;

            case 4: // Exibir alocações atuais
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }

                mymemory_display(memory);
                break;

            case 5: // Exibir estatísticas de memória
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }

                mymemory_stats(memory);
                break;

            case 6: // Limpar memória e sair
                if (memory != NULL) {
                    mymemory_cleanup(memory);
                    printf("Memoria limpa e todos os recursos liberados.\n");
                }
                printf("Encerrando programa.\n");
                return 0;

            default:
                printf("Opcao invalida. Tente novamente.\n");
                break;
        }
    }
    return 0;
}
