#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h> // Biblioteca para OpenMP

// Função para trocar dois inteiros
void swap_elements(int *elem1, int *elem2) {
    int temporary_value = *elem1;
    *elem1 = *elem2;
    *elem2 = temporary_value;
}

// Algoritmo de ordenação Odd-Even Transposition Sort (OpenMP)
void odd_even_sort_openmp(int array_to_sort[], int array_size, int num_threads) {
    int current_phase;

    // A região paralela engloba o loop das fases.
    // 'shared' especifica variáveis que todas as threads compartilham.
    // 'private' especifica variáveis que cada thread terá sua própria cópia.
    #pragma omp parallel num_threads(num_threads) shared(array_to_sort, array_size) private(current_phase)
    {
        for (current_phase = 0; current_phase < array_size; current_phase++) {
            // Fase Par: Comparações entre (i-1, i) para i ímpar
            if (current_phase % 2 == 0) {
                // Paraleliza o loop interno das comparações.
                // schedule(static) é um dos modos de divisão de trabalho.
                // Outras opções são schedule(dynamic) e schedule(guided).
                #pragma omp for schedule(static)
                for (int i = 1; i < array_size; i += 2) {
                    if (array_to_sort[i-1] > array_to_sort[i]) {
                        swap_elements(&array_to_sort[i-1], &array_to_sort[i]);
                    }
                }
            }
            // Fase Ímpar: Comparações entre (i, i+1) para i ímpar
            else {
                // Paraleliza o loop interno das comparações.
                #pragma omp for schedule(static)
                for (int i = 1; i < array_size - 1; i += 2) {
                    if (array_to_sort[i] > array_to_sort[i+1]) {
                        swap_elements(&array_to_sort[i], &array_to_sort[i+1]);
                    }
                }
            }
            // Uma barreira implícita é adicionada ao final de cada #pragma omp for,
            // garantindo que todas as threads completem a fase atual antes de iniciar a próxima.
        }
    }
}

// Função para gerar um array com números aleatórios
void fill_random_array(int target_array[], int size_of_array, int max_random_value) {
    srand(time(NULL)); // Inicializa o gerador de números aleatórios
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

// Função principal
int main(int argc, char *argv[]) {
    // Verifica se o número correto de argumentos foi passado: tamanho do array e número de threads
    if (argc != 3) {
        printf("Uso: %s <tamanho_array> <num_threads>\n", argv[0]);
        return 1;
    }

    int array_length = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int *dynamic_array = (int *)malloc(array_length * sizeof(int));
    if (dynamic_array == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o array!\n");
        return 1;
    }

    double start_time, end_time;
    double total_time_sum = 0.0;
    int number_of_runs = 3; // Rodadas para calcular a média de tempo, conforme o PDF

    for (int r = 0; r < number_of_runs; r++) {
        fill_random_array(dynamic_array, array_length, 1000);
        start_time = omp_get_wtime(); // Início da medição de tempo com OpenMP
        odd_even_sort_openmp(dynamic_array, array_length, num_threads);
        end_time = omp_get_wtime(); // Fim da medição de tempo com OpenMP

        total_time_sum += (end_time - start_time);
    }

    printf("Tempo médio de execução OpenMP para array de tamanho %d com %d threads: %.6f segundos\n", array_length, num_threads, total_time_sum / number_of_runs);
    printf("Array está ordenado: %s\n", check_if_sorted(dynamic_array, array_length) ? "Sim" : "Não");

    free(dynamic_array);
    return 0;
}