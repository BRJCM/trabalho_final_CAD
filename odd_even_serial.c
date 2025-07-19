#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h> // Para gettimeofday

// Função para trocar dois inteiros de posição
void swap_elements(int *elem1, int *elem2) {
    int temporary_value = *elem1;
    *elem1 = *elem2;
    *elem2 = temporary_value;
}

// Algoritmo de ordenação Odd-Even Transposition Sort (Serial)
void odd_even_sort_sequential(int array_to_sort[], int array_size) {
    int current_phase, index_iter;
    for (current_phase = 0; current_phase < array_size; current_phase++) {
        // Fase de comparações pares
        if (current_phase % 2 == 0) {
            for (index_iter = 1; index_iter < array_size; index_iter += 2) {
                if (array_to_sort[index_iter-1] > array_to_sort[index_iter]) {
                    swap_elements(&array_to_sort[index_iter-1], &array_to_sort[index_iter]);
                }
            }
        }
        // Fase de comparações ímpares
        else {
            for (index_iter = 1; index_iter < array_size - 1; index_iter += 2) {
                if (array_to_sort[index_iter] > array_to_sort[index_iter+1]) {
                    swap_elements(&array_to_sort[index_iter], &array_to_sort[index_iter+1]);
                }
            }
        }
    }
}

// Função para gerar um array com números aleatórios
void fill_random_array(int target_array[], int size_of_array, int max_random_value) {
    srand(time(NULL)); // Inicializa o gerador de números aleatórios com o tempo atual
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
    // Verifica se o número correto de argumentos foi passado
    if (argc != 2) {
        printf("Uso: %s <tamanho_array>\n", argv[0]);
        return 1;
    }

    int array_length = atoi(argv[1]); // Converte o argumento para inteiro
    int *dynamic_array = (int *)malloc(array_length * sizeof(int));
    if (dynamic_array == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o array!\n");
        return 1;
    }

    struct timeval start_timer, end_timer;
    double total_time_elapsed = 0.0;
    int number_of_runs = 3; // Rodadas para calcular a média de tempo, conforme o PDF

    for (int r = 0; r < number_of_runs; r++) {
        fill_random_array(dynamic_array, array_length, 1000); // Preenche com valores aleatórios até 999
        
        gettimeofday(&start_timer, NULL); // Início da medição de tempo
        odd_even_sort_sequential(dynamic_array, array_length);
        gettimeofday(&end_timer, NULL); // Fim da medição de tempo

        // Calcula o tempo de execução para esta rodada de forma mais robusta
        double current_run_time = (end_timer.tv_sec - start_timer.tv_sec) +
                                  (end_timer.tv_usec - start_timer.tv_usec) / 1000000.0;
        
        total_time_elapsed += current_run_time;
    }

    // Calcula a média dos tempos de execução
    printf("Tempo médio de execução serial para array de tamanho %d: %.6f segundos\n", array_length, total_time_elapsed / number_of_runs);
    printf("Array está ordenado: %s\n", check_if_sorted(dynamic_array, array_length) ? "Sim" : "Não");

    free(dynamic_array); // Libera a memória alocada
    return 0;
}