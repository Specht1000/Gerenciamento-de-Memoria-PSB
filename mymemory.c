// Trabalho 2 da disciplina de Programação de Software Básico
// Eduardo Camana, Guilherme Specht e Isabella Cunha
// Última atualização: 05/11/2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymemory.h"

// Função auxiliar para imprimir o estado atual da memória em ordem de endereços
void print_memory_layout(mymemory_t *memory) 
{
    allocation_t *allocated = memory->allocated_blocks;  // Ponteiro para a lista de blocos alocados
    allocation_t *free_blocks = memory->free_blocks;  // Ponteiro para a lista de blocos livres

    // Criar uma lista de todos os blocos (alocados e livres) em ordem de endereços
    allocation_t *all_blocks = NULL;  // Inicializa a lista combinada de blocos
    allocation_t **tail = &all_blocks;  // Ponteiro para o final da lista combinada

    while (allocated || free_blocks) 
    {  
        allocation_t **next_block = &free_blocks;  // Por padrão, escolhe um bloco livre
        if (allocated && (!free_blocks || allocated->start < free_blocks->start)) 
        {  
            next_block = &allocated;  // Seleciona o bloco alocado
        }

        *tail = (allocation_t *)malloc(sizeof(allocation_t));  // Aloca um novo bloco na lista combinada
        **tail = **next_block;  // Copia os dados do bloco selecionado para a nova entrada
        (*tail)->next = NULL;  // Define o próximo do novo bloco como NULL

        tail = &(*tail)->next;  // Move o ponteiro de cauda para o próximo
        *next_block = (*next_block)->next;  // Avança para o próximo bloco na lista de origem
    }

    printf("\nEstado atual da memoria:\n");
    printf("+----------------------+----------------------+----------+---------+\n");
    printf("| Inicio               | Fim                  | Tamanho  | Estado  |\n");
    printf("+----------------------+----------------------+----------+---------+\n");

    allocation_t *current = all_blocks;  // Inicia a impressão da lista combinada
    allocation_t *previous_free = NULL;  // Armazena o último bloco livre
    unsigned long combined_size = 0;  // Tamanho combinado para blocos livres adjacentes
    void *combined_start = NULL;  // Início combinado para blocos livres adjacentes

    while (current) // Percorre todos os blocos
    {  
        const char *estado = "Livre";  // Assume que o bloco é livre
        for (allocation_t *iter = memory->allocated_blocks; iter != NULL; iter = iter->next) 
        {  
            if (iter->start == current->start) 
            {  
                estado = "Alocado";  // Marca o bloco como alocado
                break;
            }
        }

        if (strcmp(estado, "Livre") == 0) // Se o bloco atual é livre
        {  
            if (previous_free && (char *)previous_free->start + previous_free->size == current->start) // Se o bloco livre atual é adjacente ao anterior
            {  
                combined_size += current->size;  // Atualiza o tamanho combinado
            } 
            else 
            {  
                if (previous_free) 
                {  
                    // Imprime o bloco livre combinado anterior
                    printf("| 0x%-18p | 0x%-18p | %8lu | %-7s |\n",  
                           combined_start,
                           (char *)combined_start + combined_size - 1,
                           combined_size,
                           "Livre");
                    printf("+----------------------+----------------------+----------+---------+\n");
                }
                // Inicia um novo bloco livre combinado
                combined_start = current->start;
                combined_size = current->size;
            }
            previous_free = current;  // Atualiza o bloco livre anterior
        } 
        else 
        {  
            if (previous_free) // Imprime o bloco livre combinado antes de mudar para alocado
            {  
                printf("| 0x%-18p | 0x%-18p | %8lu | %-7s |\n",  
                       combined_start,
                       (char *)combined_start + combined_size - 1,
                       combined_size,
                       "Livre");
                printf("+----------------------+----------------------+----------+---------+\n");
                previous_free = NULL;  // Reseta o bloco livre anterior
            }
            // Imprime o bloco alocado
            printf("| 0x%-18p | 0x%-18p | %8lu | %-7s |\n",  
                   current->start,
                   (char *)current->start + current->size - 1,
                   (unsigned long)current->size,
                   estado);
            printf("+----------------------+----------------------+----------+---------+\n");
        }
        current = current->next;  // Avança para o próximo bloco
    }

    if (previous_free) // Imprime o último bloco livre combinado, se existir
    {  
        printf("| 0x%-18p | 0x%-18p | %8lu | %-7s |\n",  
               combined_start,
               (char *)combined_start + combined_size - 1,
               combined_size,
               "Livre");
        printf("+----------------------+----------------------+----------+---------+\n");
    }

    // Liberar a lista temporária
    current = all_blocks;  // Reinicia o ponteiro para liberar os blocos combinados
    while (current) // Libera cada bloco na lista combinada
    {  
        allocation_t *temp = current;
        current = current->next;
        free(temp);
    }
}

// Estrutura para o pool de memória
mymemory_t* mymemory_init(size_t size, AllocationStrategy strategy)
{
    mymemory_t *memory = (mymemory_t*)malloc(sizeof(mymemory_t)); // Aloca a estrutura de controle principal de memória
    memory->pool = malloc(size); // Aloca o pool de memória total para uso
    memory->total_size = size; // Define o tamanho total do pool de memória
    memory->free_blocks = (allocation_t*)malloc(sizeof(allocation_t)); // Inicializa o primeiro bloco da lista de blocos livres
    memory->free_blocks->start = memory->pool; // Define o início do bloco livre como o início do pool
    memory->free_blocks->size = size; // Define o tamanho do bloco livre
    memory->free_blocks->next = NULL; // Não há blocos livres adicionais, então o próximo é NULL
    memory->allocated_blocks = NULL; // Inicialmente, não há blocos alocados
    memory->strategy = strategy; // Define a estratégia de alocação (First Fit, Best Fit, Worst Fit)
    return memory; // Retorna o ponteiro para a estrutura de controle de memória
}

// Função para alocar um bloco de memória
void* mymemory_alloc(mymemory_t *memory, size_t size)
{
    allocation_t *current = memory->free_blocks;  // Ponteiro para percorrer os blocos livres
    allocation_t *best_block = NULL;  // Melhor bloco na estratégia Best Fit
    allocation_t **prev_best = NULL;  // Ponteiro para o bloco anterior ao melhor bloco
    allocation_t **prev = &memory->free_blocks;  // Ponteiro para o bloco anterior na lista de blocos livres
    void *allocated_start = NULL;  // Endereço do início do bloco alocado

    switch (memory->strategy) // Seleciona a estratégia de alocação
    {  
        case FIRST_FIT: 
            while (current) // Percorre a lista de blocos livres
            {  
                if (current->size >= size) // Verifica se o bloco atual é grande o suficiente
                {  
                    allocated_start = current->start;  // Armazena o início do bloco alocado
                    *prev = current->next;  // Atualiza o ponteiro para o próximo bloco livre
                    allocation_t *new_alloc = (allocation_t*)malloc(sizeof(allocation_t));  // Cria nova entrada para o bloco alocado
                    new_alloc->start = allocated_start;  // Define o início do bloco alocado
                    new_alloc->size = size;  // Define o tamanho do bloco alocado
                    new_alloc->next = memory->allocated_blocks;  // Insere o bloco na lista de alocados
                    memory->allocated_blocks = new_alloc;  // Atualiza o início da lista de alocados

                    if (current->size > size) // Se o bloco livre é maior que o necessário
                    {  
                        current->start = (char*)current->start + size;  // Atualiza o início do bloco livre restante
                        current->size -= size;  // Ajusta o tamanho do bloco livre restante
                        current->next = memory->free_blocks;  // Reinsere o bloco restante na lista de livres
                        memory->free_blocks = current;  // Atualiza o início da lista de livres
                    } 
                    else 
                    {
                        free(current);  // Libera o bloco se o tamanho for exatamente o necessário
                    }
                    return allocated_start;  // Retorna o endereço do início do bloco alocado
                }
                prev = &current->next;  // Avança para o próximo bloco
                current = current->next;  // Avança o ponteiro
            }
            break;

        case BEST_FIT: 
            while (current) // Percorre a lista de blocos livres
            {  
                if (current->size >= size && (!best_block || current->size < best_block->size)) // Encontra o menor bloco adequado
                {  
                    best_block = current;  // Armazena o melhor bloco encontrado
                    prev_best = prev;  // Atualiza o ponteiro para o bloco anterior
                }
                prev = &current->next;  // Avança o ponteiro para o próximo bloco
                current = current->next;  // Avança o ponteiro
            }
            if (best_block) // Se um bloco adequado foi encontrado
            {  
                allocated_start = best_block->start;  // Define o início do bloco alocado
                *prev_best = best_block->next;  // Atualiza o ponteiro do bloco anterior
                allocation_t *new_alloc = (allocation_t*)malloc(sizeof(allocation_t));  // Cria novo bloco alocado
                new_alloc->start = allocated_start;  // Define o início do bloco alocado
                new_alloc->size = size;  // Define o tamanho do bloco alocado
                new_alloc->next = memory->allocated_blocks;  // Insere o bloco na lista de alocados
                memory->allocated_blocks = new_alloc;  // Atualiza o início da lista de alocados

                if (best_block->size > size) // Se o bloco é maior que o necessário
                {  
                    best_block->start = (char*)best_block->start + size;  // Atualiza o início do bloco restante
                    best_block->size -= size;  // Ajusta o tamanho do bloco restante
                    best_block->next = memory->free_blocks;  // Reinsere o bloco restante na lista de livres
                    memory->free_blocks = best_block;  // Atualiza o início da lista de livres
                } 
                else 
                {
                    free(best_block);  // Libera o bloco se o tamanho for exatamente o necessário
                }
                return allocated_start;  // Retorna o endereço do início do bloco alocado
            }
            break;

        case WORST_FIT: {
            allocation_t *worst_block = NULL;  // Maior bloco na estratégia Worst Fit
            allocation_t **prev_worst = NULL;  // Ponteiro para o bloco anterior ao maior bloco
            current = memory->free_blocks;  // Recomeça a percorrer os blocos livres
            prev = &memory->free_blocks;  // Reinicia o ponteiro para o bloco anterior

            while (current) // Percorre a lista de blocos livres
            {  
                if (current->size >= size && (!worst_block || current->size > worst_block->size)) // Encontra o maior bloco adequado
                {  
                    worst_block = current;  // Armazena o pior bloco encontrado
                    prev_worst = prev;  // Atualiza o ponteiro para o bloco anterior
                }
                prev = &current->next;  // Avança o ponteiro para o próximo bloco
                current = current->next;  // Avança o ponteiro
            }
            if (worst_block) // Se um bloco adequado foi encontrado
            {  
                allocated_start = worst_block->start;  // Define o início do bloco alocado
                *prev_worst = worst_block->next;  // Atualiza o ponteiro do bloco anterior
                allocation_t *new_alloc = (allocation_t*)malloc(sizeof(allocation_t));  // Cria novo bloco alocado
                new_alloc->start = allocated_start;  // Define o início do bloco alocado
                new_alloc->size = size;  // Define o tamanho do bloco alocado
                new_alloc->next = memory->allocated_blocks;  // Insere o bloco na lista de alocados
                memory->allocated_blocks = new_alloc;  // Atualiza o início da lista de alocados

                if (worst_block->size > size) // Se o bloco é maior que o necessário
                {  
                    worst_block->start = (char*)worst_block->start + size;  // Atualiza o início do bloco restante
                    worst_block->size -= size;  // Ajusta o tamanho do bloco restante
                    worst_block->next = memory->free_blocks;  // Reinsere o bloco restante na lista de livres
                    memory->free_blocks = worst_block;  // Atualiza o início da lista de livres
                } 
                else 
                {
                    free(worst_block);  // Libera o bloco se o tamanho for exatamente o necessário
                }
                return allocated_start;  // Retorna o endereço do início do bloco alocado
            }
            break;
        }
    }
    return NULL;  // Retorna NULL se nenhum bloco adequado foi encontrado
}


// Função para liberar memória
void mymemory_free(mymemory_t *memory, void *ptr) 
{
    allocation_t *current = memory->allocated_blocks; // Ponteiro para percorrer a lista de blocos alocados
    allocation_t *prev = NULL; // Armazena o bloco anterior

    while (current && current->start != ptr) // Percorre a lista até encontrar o bloco a ser liberado
    { 
        prev = current;
        current = current->next;
    }

    if (current) // Se o bloco foi encontrado
    { 
        if (prev) 
        {
            prev->next = current->next; // Remove o bloco da lista de alocados
        } 
        else 
        {
            memory->allocated_blocks = current->next; // Atualiza o head se o bloco era o primeiro
        }

        current->next = memory->free_blocks; // Reinsere o bloco liberado na lista de livres
        memory->free_blocks = current; // Atualiza o início da lista de livres
    }
}

// Exibe alocações atuais
void mymemory_display(mymemory_t *memory) 
{
    allocation_t *current = memory->allocated_blocks; // Ponteiro para iterar sobre os blocos alocados
    printf("Alocacoes atuais:\n");

    while (current) // Percorre a lista de blocos alocados
    { 
        printf("Inicio: 0x%p, Tamanho: %lu\n", current->start, (unsigned long)current->size); // Imprime endereço e tamanho
        current = current->next; // Avança para o próximo bloco
    }
}

// Exibe estatísticas de memória
void mymemory_stats(mymemory_t *memory) 
{
    allocation_t *current = memory->allocated_blocks;  // Ponteiro para iterar sobre os blocos alocados
    unsigned long total_allocated = 0;  // Inicializa o total de memória alocada
    int alloc_count = 0;  // Inicializa o contador de alocações

    while (current) // Percorre a lista de blocos alocados
    {  
        total_allocated += current->size;  // Acumula o tamanho dos blocos alocados
        alloc_count++;  // Incrementa o contador de alocações
        current = current->next;  // Avança para o próximo bloco
    }

    unsigned long total_free = memory->total_size - total_allocated;  // Calcula a memória livre

    // Estatísticas adicionais
    current = memory->free_blocks;  // Ponteiro para iterar sobre os blocos livres
    unsigned long max_free_block = 0;  // Maior bloco contíguo de memória livre
    unsigned long current_contiguous_size = 0;
    void *previous_end = NULL;
    int free_fragments = 0;  // Número de fragmentos de memória livre

    while (current) // Percorre a lista de blocos livres
    {  
        if (previous_end && (char *)previous_end + 1 == current->start) 
        {
            current_contiguous_size += current->size;  // Bloco contíguo encontrado
        } else {
            if (current_contiguous_size > max_free_block) // Novo bloco começa, finalize o bloco anterior
            {
                max_free_block = current_contiguous_size;
            }
            current_contiguous_size = current->size;  // Inicia novo bloco contíguo
            free_fragments++;  // Conta o novo fragmento
        }
        previous_end = (char *)current->start + current->size - 1;
        current = current->next;  // Avança para o próximo bloco
    }

    if (current_contiguous_size > max_free_block) // Finalizar verificação do último bloco contíguo
    {
        max_free_block = current_contiguous_size;
    }

    // Exibe estatísticas de memória
    printf("Estatisticas de memoria:\n");
    printf("Total de alocacoes: %d\n", alloc_count);
    printf("Memoria total alocada: %lu bytes\n", total_allocated);
    printf("Memoria total livre: %lu bytes\n", total_free);
    printf("Maior bloco livre contiguo: %lu bytes\n", max_free_block);
    printf("Numero de fragmentos de memoria livre: %d\n", free_fragments);
}

// Limpa memória e libera recursos
void mymemory_cleanup(mymemory_t *memory) 
{
    allocation_t *current = memory->allocated_blocks; // Libera todos os blocos alocados
    while (current) 
    {
        allocation_t *temp = current; // Armazena o bloco atual
        current = current->next; // Avança para o próximo bloco
        free(temp); // Libera o bloco atual
    }
    current = memory->free_blocks; // Libera todos os blocos livres
    while (current) 
    {
        allocation_t *temp = current; // Armazena o bloco atual
        current = current->next; // Avança para o próximo bloco
        free(temp); // Libera o bloco atual
    }
    free(memory->pool); // Libera o pool de memória
    free(memory); // Libera a estrutura de controle principal
}

void display_menu() {
    printf("\n--- Menu de Gerenciamento de Memoria ---\n");
    printf("1. Inicializar memoria\n");
    printf("2. Alocar memoria\n");
    printf("3. Liberar memoria\n");
    printf("4. Exibir alocacoes atuais\n");
    printf("5. Exibir estatisticas de memoria\n");
    printf("6. Layout da memoria\n");
    printf("7. Limpar memoria e sair\n");
    printf("Escolha uma opcao: ");
}

int main() {
    mymemory_t *memory = NULL;
    AllocationStrategy strategy;
    size_t pool_size, alloc_size;
    int option;
    void *allocated_ptr;

    while (1) 
    {
        display_menu();
        scanf("%d", &option);

        switch (option) {
            case 1: // Inicializar memória
                if (memory != NULL) 
                {
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
                if (memory == NULL) 
                {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }
                printf("Digite o tamanho do bloco a ser alocado: ");
                scanf("%zu", &alloc_size);
                allocated_ptr = mymemory_alloc(memory, alloc_size);

                if (allocated_ptr != NULL) 
                {
                    printf("Memoria alocada no endereco 0x%p.\n", allocated_ptr);
                } 
                else 
                {
                    printf("Falha ao alocar memoria. Tente um tamanho menor ou verifique o pool disponivel.\n");
                }
                break;

            case 3: // Liberar memória
                if (memory == NULL) 
                {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }
                printf("Digite o endereco (em hexadecimal sem o '0x') do bloco a ser liberado: ");
                scanf("%p", &allocated_ptr);
                mymemory_free(memory, allocated_ptr);
                printf("Memoria no endereco 0x%p liberada.\n", allocated_ptr);
                break;

            case 4: // Exibir alocações atuais
                if (memory == NULL) 
                {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }
                mymemory_display(memory);
                break;

            case 5: // Exibir estatísticas de memória
                if (memory == NULL) 
                {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }
                mymemory_stats(memory);
                break;

            case 6: // Exibir layout da memória
                if (memory == NULL) 
                {
                    printf("Inicialize a memoria primeiro.\n");
                    break;
                }
                print_memory_layout(memory);  // Chamada para exibir o layout da memória
                break;

            case 7: // Limpar memória e sair
                if (memory != NULL) 
                {
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
