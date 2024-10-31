#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymemory.h"

// Função para inicializar o pool de memória
mymemory_t* mymemory_init(size_t size, AllocationStrategy strategy) {
    mymemory_t *memory = (mymemory_t*)malloc(sizeof(mymemory_t));
    memory->pool = malloc(size);
    memory->total_size = size;
    memory->head = (allocation_t*)malloc(sizeof(allocation_t));
    memory->head->start = memory->pool;
    memory->head->size = size;
    memory->head->next = NULL;
    memory->strategy = strategy;
    return memory;
}

// Função para alocar um bloco de memória usando a estratégia definida
void* mymemory_alloc(mymemory_t *memory, size_t size) {
    allocation_t *current = memory->head;
    allocation_t *best_block = NULL;
    allocation_t *worst_block = NULL;
    void *allocated_start = NULL;

    // Busca do bloco com base na estratégia
    switch (memory->strategy) {
        case FIRST_FIT:
            while (current) {
                if (current->size >= size) {
                    allocated_start = current->start;
                    current->start += size;
                    current->size -= size;
                    return allocated_start;
                }
                current = current->next;
            }
            break;

        case BEST_FIT:
            while (current) {
                if (current->size >= size && (!best_block || current->size < best_block->size)) {
                    best_block = current;
                }
                current = current->next;
            }
            if (best_block) {
                allocated_start = best_block->start;
                best_block->start += size;
                best_block->size -= size;
                return allocated_start;
            }
            break;

        case WORST_FIT:
            while (current) {
                if (current->size >= size && (!worst_block || current->size > worst_block->size)) {
                    worst_block = current;
                }
                current = current->next;
            }
            if (worst_block) {
                allocated_start = worst_block->start;
                worst_block->start += size;
                worst_block->size -= size;
                return allocated_start;
            }
            break;
    }

    return NULL;
}

// Função para liberar um bloco de memória e marcar o espaço como disponível
void mymemory_free(mymemory_t *memory, void *ptr) {
    allocation_t *current = memory->head;
    allocation_t *prev = NULL;

    // Procura o bloco pelo ponteiro de início
    while (current && current->start != ptr) {
        prev = current;
        current = current->next;
    }

    // Se encontramos o bloco, devemos restaurar o espaço liberado e atualizar o tamanho
    if (current) {
        if (prev) {
            prev->next = current->next; // Remove o bloco da lista de alocações
        } else {
            memory->head = current->next; // Se o bloco for o primeiro, atualiza o head
        }

        // Atualiza o bloco como um espaço não utilizado e coloca-o de volta no pool
        current->start = ptr;
        current->size += current->size;
        current->next = memory->head;
        memory->head = current;  // Reinsere o bloco liberado na lista principal

        free(current);
    }
}

// Função para exibir todas as alocações atuais
void mymemory_display(mymemory_t *memory) {
    allocation_t *current = memory->head;
    printf("Alocações atuais:\n");

    while (current) {
        printf("Início: %p, Tamanho: %lu\n", current->start, (unsigned long)current->size);
        current = current->next;
    }
}

// Função para exibir estatísticas gerais da memória
void mymemory_stats(mymemory_t *memory) {
    allocation_t *current = memory->head;
    unsigned long total_allocated = 0, total_free = memory->total_size;
    int alloc_count = 0;

    while (current) {
        total_allocated += current->size;
        total_free -= current->size;
        alloc_count++;
        current = current->next;
    }

    printf("Estatísticas de memória:\n");
    printf("Total de alocações: %d\n", alloc_count);
    printf("Memória total alocada: %lu bytes\n", total_allocated);
    printf("Memória total livre: %lu bytes\n", total_free);
}

// Função para liberar o pool de memória e estruturas de controle
void mymemory_cleanup(mymemory_t *memory) {
    allocation_t *current = memory->head;
    while (current) {
        allocation_t *temp = current;
        current = current->next;
        free(temp);
    }
    free(memory->pool);
    free(memory);
}

void display_menu() {
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
    mymemory_t *memory = NULL;
    AllocationStrategy strategy;
    size_t pool_size, alloc_size;
    int option;
    void *allocated_ptr;

    while (1) {
        display_menu();
        scanf("%d", &option);

        switch (option) {
            case 1:
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

            case 2:
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

            case 3:
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }

                printf("Digite o endereco (em hexadecimal) do bloco a ser liberado: ");
                scanf("%p", &allocated_ptr);
                mymemory_free(memory, allocated_ptr);
                printf("Memoria no endereco %p liberada.\n", allocated_ptr);
                break;

            case 4:
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }

                mymemory_display(memory);
                break;

            case 5:
                if (memory == NULL) {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }

                mymemory_stats(memory);
                break;

            case 6:
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