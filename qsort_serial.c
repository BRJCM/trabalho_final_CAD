#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h> // Para gettimeofday

// Função de comparação para qsort
// qsort exige uma função que compare dois elementos e retorne:
// < 0 se o primeiro for menor que o segundo
// = 0 se forem iguais
// > 0 se o primeiro for maior que o segundo
int compare_integers(const void* a, const void* b) {
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// Função para gerar um array com números aleatórios
void fill_random_array(int target_array[], int size_of_array, int max_random_value) {
    srand(time(NULL));
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
    if (argc != 2) {
        printf("Uso: %s <tamanho_array>\n", argv[0]);
        return 1;
    }

    int array_length = atoi(argv[1]);
    int *dynamic_array = (int *)malloc(array_length * sizeof(int));
    if (dynamic_array == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o array!\n");
        return 1;
    }

    struct timeval start_timer, end_timer;
    double total_time_elapsed = 0.0;
    int number_of_runs = 10; // Seu amigo usou 10 rodadas, vamos manter para consistência

    for (int r = 0; r < number_of_runs; r++) {
        fill_random_array(dynamic_array, array_length, 1000); // Preenche com valores aleatórios

        gettimeofday(&start_timer, NULL); // Início da medição
        qsort(dynamic_array, array_length, sizeof(int), compare_integers); // Chama a função qsort
        gettimeofday(&end_timer, NULL); // Fim da medição

        // Calcula o tempo de execução para esta rodada
        total_time_elapsed += (end_timer.tv_sec - start_timer.tv_sec) +
                              (end_timer.tv_usec - start_timer.tv_usec) / 1000000.0;
    }

    double average_time = total_time_elapsed / number_of_runs;

    printf("Tempo médio de execução qsort serial para array de tamanho %d: %.6f segundos\n", array_length, average_time);
    printf("Array está ordenado: %s\n", check_if_sorted(dynamic_array, array_length) ? "Sim" : "Não");

    free(dynamic_array);
    return 0;
}