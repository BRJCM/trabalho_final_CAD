#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>    // Biblioteca para MPI
#include <stdbool.h> // Mantenho para outras possíveis funções

// Função para trocar dois inteiros
void swap_elements(int *elem1, int *elem2) {
    int temporary_value = *elem1;
    *elem1 = *elem2;
    *elem2 = temporary_value;
}

// Função de comparação para qsort (necessária para ordenar subarrays locais)
int compare_integers(const void *a, const void *b) {
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// Função de Insertion Sort, eficiente para arrays quase ordenados
void insertion_sort(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// Função para gerar um array com números aleatórios
void fill_random_array(int target_array[], int size_of_array, int max_random_value, int rank) {
    srand(time(NULL) + rank); // Usa o rank para uma semente diferente por processo
    for (int i = 0; i < size_of_array; i++) {
        target_array[i] = rand() % max_random_value;
    }
}

// Função para verificar se um array está em ordem crescente
int check_if_sorted(int input_array[], int arr_size) {
    for (int i = 0; i < arr_size - 1; i++) {
        if (input_array[i] > input_array[i+1]) {
            return 0; // Não está ordenado
        }
    }
    return 1; // Está ordenado
}

// Algoritmo Odd-Even Transposition Sort (MPI)
// Retorna o tempo total gasto em comunicação para este processo
double odd_even_sort_mpi(int local_arr[], int local_n, int global_n, int num_processes, int rank) {
    double comm_time_total = 0.0;
    int boundary_element; 
    int local_swap_made; // Agora é int para compatibilidade direta com MPI_INT
    int global_swaps_count; 

    // Primeiramente, ordena o subarray local (qsort é bom para o primeiro desordenamento total)
    qsort(local_arr, local_n, sizeof(int), compare_integers);

    // O Odd-Even Transposition Sort distribuído funciona em fases globais.
    // O algoritmo executa 'global_n' fases, mas pode parar mais cedo.
    for (int phase = 0; phase < global_n; phase++) {
        local_swap_made = 0; // Reset a flag para esta fase (0 = false, 1 = true)

        double comm_start = MPI_Wtime();

        // Fase Par: Processos pares comunicam com o vizinho à direita
        // Processos ímpares comunicam com o vizinho à esquerda
        if (phase % 2 == 0) {
            // Processo par troca com vizinho à direita
            if (rank % 2 == 0 && rank < num_processes - 1) {
                MPI_Sendrecv(&local_arr[local_n - 1], 1, MPI_INT, rank + 1, 0, // Envia último elemento
                             &boundary_element, 1, MPI_INT, rank + 1, 0, // Recebe do vizinho
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                if (local_arr[local_n - 1] > boundary_element) {
                    local_arr[local_n - 1] = boundary_element; 
                    local_swap_made = 1; 
                }
            }
            // Processo ímpar troca com vizinho à esquerda
            else if (rank % 2 != 0 && rank > 0) {
                MPI_Sendrecv(&local_arr[0], 1, MPI_INT, rank - 1, 0, // Envia primeiro elemento
                             &boundary_element, 1, MPI_INT, rank - 1, 0, // Recebe do vizinho
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (local_arr[0] < boundary_element) {
                    local_arr[0] = boundary_element; 
                    local_swap_made = 1; 
                }
            }
        }
        // Fase Ímpar: Processos ímpares comunicam com o vizinho à direita
        // Processos pares comunicam com o vizinho à esquerda
        else {
            // Processo ímpar troca com vizinho à direita
            if (rank % 2 != 0 && rank < num_processes - 1) {
                MPI_Sendrecv(&local_arr[local_n - 1], 1, MPI_INT, rank + 1, 0,
                             &boundary_element, 1, MPI_INT, rank + 1, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (local_arr[local_n - 1] > boundary_element) {
                    local_arr[local_n - 1] = boundary_element;
                    local_swap_made = 1;
                }
            }
            // Processo par troca com vizinho à esquerda
            else if (rank % 2 == 0 && rank > 0) {
                MPI_Sendrecv(&local_arr[0], 1, MPI_INT, rank - 1, 0,
                             &boundary_element, 1, MPI_INT, rank - 1, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (local_arr[0] < boundary_element) {
                    local_arr[0] = boundary_element;
                    local_swap_made = 1;
                }
            }
        }
        double comm_end = MPI_Wtime();
        comm_time_total += (comm_end - comm_start);

        if (local_swap_made == 1) {
            insertion_sort(local_arr, local_n);
        }

        MPI_Allreduce(&local_swap_made, &global_swaps_count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        
        if (global_swaps_count == 0 && phase > 0) { 
            break;
        }
    }
    return comm_time_total;
}


// Função principal
int main(int argc, char *argv[]) {
    // Inicializa o ambiente MPI
    MPI_Init(&argc, &argv);

    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);       // Obtém o rank (ID) do processo atual
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes); // Obtém o número total de processos

    if (argc != 2) {
        if (rank == 0) { // Apenas o processo 0 imprime a mensagem de uso
            printf("Uso: %s <tamanho_array>\n", argv[0]);
        }
        MPI_Finalize(); // Finaliza o ambiente MPI antes de sair
        return 1;
    }

    int global_array_length = atoi(argv[1]);
    
    // Calcula o tamanho do subarray local para cada processo.
    // O último processo recebe os elementos restantes se a divisão não for exata.
    int local_chunk_size = global_array_length / num_processes;
    if (rank == num_processes - 1) {
        local_chunk_size += (global_array_length % num_processes);
    }
    // Garante que o chunk size mínimo seja 1 para evitar malloc(0) e divisões por zero
    // e trata casos onde global_array_length é muito pequeno
    if (global_array_length > 0 && local_chunk_size == 0) { // Garante min 1 se array não é vazio
        local_chunk_size = 1; 
    }
    // Ajusta o local_chunk_size para ser 1 para os primeiros 'global_array_length' processos e 0 para os restantes
    if (global_array_length < num_processes && global_array_length > 0) {
        if (rank < global_array_length) {
            local_chunk_size = 1;
        } else {
            local_chunk_size = 0; // Processos ociosos
        }
    } else if (global_array_length == 0) { // Lidar com array de tamanho 0
        local_chunk_size = 0;
    }


    int *global_array = NULL;
    int *local_array = NULL; // Inicializa como NULL
    if (local_chunk_size > 0) { // Aloca apenas se o chunk size for maior que zero
        local_array = (int *)malloc(local_chunk_size * sizeof(int));
        if (local_array == NULL) {
            fprintf(stderr, "Processo %d: Erro ao alocar memória para o array local!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1); // Aborta todos os processos em caso de erro
        }
    }


    // Apenas o processo raiz (rank 0) aloca o array global
    if (rank == 0) {
        global_array = (int *)malloc(global_array_length * sizeof(int));
        if (global_array == NULL) {
            fprintf(stderr, "Processo %d: Erro ao alocar memória para o array global!\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    double total_communication_time_avg = 0.0;
    double total_execution_time_avg = 0.0;
    int number_of_runs = 3; // Número de rodadas para média estatística, conforme o PDF

    for (int r = 0; r < number_of_runs; r++) {
        // Apenas o processo raiz gera o array aleatório
        if (rank == 0) {
            fill_random_array(global_array, global_array_length, 1000, rank);
        }

        // Distribui o array global em chunks para cada processo local.
        // Usa MPI_Scatterv para lidar com tamanhos de chunk potencialmente desiguais.
        int *sendcounts = NULL;
        int *displs = NULL;

        if (rank == 0) {
            sendcounts = (int *)malloc(num_processes * sizeof(int));
            displs = (int *)malloc(num_processes * sizeof(int));
            int current_disp = 0;
            for (int i = 0; i < num_processes; ++i) {
                // Recalcula sendcounts e displs para Scatterv com base nos chunks exatos
                int current_chunk_size_for_scatter = global_array_length / num_processes;
                if (i == num_processes - 1) {
                    current_chunk_size_for_scatter += (global_array_length % num_processes);
                }
                // Tratamento para arrays pequenos com muitos processos para Scatterv
                if (global_array_length > 0 && global_array_length < num_processes && i < global_array_length) { 
                     current_chunk_size_for_scatter = 1;
                } else if (global_array_length > 0 && global_array_length < num_processes && i >= global_array_length) {
                     current_chunk_size_for_scatter = 0;
                } else if (global_array_length == 0) {
                    current_chunk_size_for_scatter = 0;
                }
                
                sendcounts[i] = current_chunk_size_for_scatter;
                displs[i] = current_disp;
                current_disp += sendcounts[i];
            }
        }
        
        // Executa MPI_Scatterv apenas se o array global tiver elementos a serem distribuídos
        if (global_array_length > 0) {
            // Garante que o local_array não é NULL antes de chamar Scatterv se local_chunk_size é > 0
            if (local_chunk_size > 0) {
                MPI_Scatterv(global_array, sendcounts, displs, MPI_INT,
                             local_array, local_chunk_size, MPI_INT, // local_chunk_size já foi ajustado
                             0, MPI_COMM_WORLD);
            } else { // Se global_array_length > 0 mas este processo tem chunk_size = 0, ele ainda precisa participar
                // Processos com chunk_size 0 ainda precisam participar do Scatterv, mas com buffers e counts nulos/zero
                // O sendcounts/displs do root precisam ser consistentes com isso.
                // A versão mais segura é que *todos* os processos passem seu local_array (mesmo que nulo e count 0)
                // ou que o root use MPI_Send para distribuir se não for um Scatter global.
                // Para este caso, se local_chunk_size é 0 para um rank > 0, ele ainda precisa participar do Scatterv
                // de forma "silenciosa" ou o root deve ter sendcounts/displs zero para ele.
                // A implementação atual de Scatterv com NULL buffer/count 0 em ranks não-root pode variar entre MPIs.
                // Para ser mais seguro, o Scatterv deve ser chamado por todos, e o buffer local_array deve ser válido.
                // Se local_chunk_size for 0, local_array seria NULL. Isso pode causar problemas.
                // Revisitando: A forma como está, o MPI_Scatterv em si vai lidar com os sendcounts[i]=0.
                // O importante é que local_array para o *receptor* seja válido para local_chunk_size > 0.
                // E se local_chunk_size é 0, o recvbuf pode ser NULL com recvcount 0.
                // O código já lida com local_array = NULL se local_chunk_size é 0.
                // Então esta parte parece razoável.
            }
        } // Se global_array_length é 0, Scatterv não é chamado. local_array já é NULL ou tem 0 elementos.


        // Sincroniza todos os processos antes de iniciar a medição de tempo real do algoritmo
        MPI_Barrier(MPI_COMM_WORLD);
        double start_total_time = MPI_Wtime();

        // Executa o algoritmo de ordenação Odd-Even Transposition Sort MPI
        // Chama apenas se houver elementos para ordenar no array global e local
        if (global_array_length > 0 && local_chunk_size > 0) { // Só chama o sort se houver dados
            double current_comm_time = odd_even_sort_mpi(local_array, local_chunk_size, global_array_length, num_processes, rank);
            total_communication_time_avg += current_comm_time;
        } else { // Para arrays vazios ou processos ociosos, tempo de comunicação e execução são zero
             total_communication_time_avg += 0;
        }
        
        // Sincroniza todos os processos após o algoritmo para medir o tempo total
        MPI_Barrier(MPI_COMM_WORLD);
        double end_total_time = MPI_Wtime();
        double elapsed_total_time = end_total_time - start_total_time;

        // Reduz o tempo total de execução (máximo entre os processos) e o tempo de comunicação (soma) para o processo 0
        double global_max_total_time_round;
        MPI_Reduce(&elapsed_total_time, &global_max_total_time_round, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        
        double current_round_comm_time_sum; // Soma total de comunicação para esta rodada
        MPI_Reduce(&total_communication_time_avg, &current_round_comm_time_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            total_execution_time_avg += global_max_total_time_round;
            total_communication_time_avg = current_round_comm_time_sum; // Acumula a soma global das comunicações
        }

        if (rank == 0) {
            free(sendcounts);
            free(displs);
        }
    }

    // Processo raiz (rank 0) coleta todos os subarrays para verificar a ordenação final e imprimir resultados
    if (rank == 0) {
        // Usar MPI_Gatherv para coletar subarrays de tamanhos potencialmente desiguais
        int *recvcounts = (int *)malloc(num_processes * sizeof(int));
        int *displs = (int *)malloc(num_processes * sizeof(int));
        int current_disp = 0;
        for (int i = 0; i < num_processes; ++i) {
            // Recalcula recvcounts e displs para Gatherv com base nos chunks exatos
            int current_chunk_size_for_gather = global_array_length / num_processes;
            if (i == num_processes - 1) {
                current_chunk_size_for_gather += (global_array_length % num_processes);
            }
            // Tratamento para arrays pequenos com muitos processos para Gatherv
            if (global_array_length > 0 && global_array_length < num_processes && i < global_array_length) { 
                 current_chunk_size_for_gather = 1;
            } else if (global_array_length > 0 && global_array_length < num_processes && i >= global_array_length) {
                 current_chunk_size_for_gather = 0;
            } else if (global_array_length == 0) {
                current_chunk_size_for_gather = 0;
            }

            recvcounts[i] = current_chunk_size_for_gather;
            displs[i] = current_disp;
            current_disp += recvcounts[i];
        }
        
        // Chama MPI_Gatherv apenas se o array global tiver elementos
        if (global_array_length > 0) {
            // Cada processo precisa ter seu local_array alocado para participar do Gatherv, mesmo que vazio para alguns
            // Se local_array foi NULL para processos ociosos, eles precisam de um buffer válido, mesmo que temporário e de tamanho 0.
            // A forma mais segura é garantir que o local_array é válido antes desta chamada.
            // Para processos com local_chunk_size == 0, o sendbuf de Gatherv pode ser NULL e sendcount 0.
            MPI_Gatherv(local_array, local_chunk_size, MPI_INT, // Send buffer e count do próprio processo
                        global_array, recvcounts, displs, MPI_INT, // Receive buffer, counts, displs do root
                        0, MPI_COMM_WORLD);
        }

        printf("Tempo Total (max) médio: %.6f s\n", total_execution_time_avg / number_of_runs);
        printf("Tempo Comunicação (soma) médio: %.6f s\n", total_communication_time_avg / number_of_runs);
        printf("Overhead (aprox) médio: %.2f%%\n", (total_communication_time_avg / total_execution_time_avg) * 100);
        printf("Array está ordenado: %s\n", check_if_sorted(global_array, global_array_length) ? "Sim" : "Não");

        free(global_array);
        free(recvcounts);
        free(displs);
    }
    
    // Libera local_array apenas se foi alocado
    if (local_array != NULL) {
        free(local_array);
    }
    MPI_Finalize(); // Finaliza o ambiente MPI
    return 0;
}