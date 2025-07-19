#!/bin/bash

# Nome do arquivo de saída onde os resultados serão salvos
OUTPUT_FILE="resultados_experimentos.txt"

# Limpa o arquivo de saída anterior (ou cria um novo)
> $OUTPUT_FILE

echo "Iniciando experimentos..." | tee -a $OUTPUT_FILE
echo "Data: $(date)" | tee -a $OUTPUT_FILE
echo "---------------------------------------------------" | tee -a $OUTPUT_FILE

# Definições dos tamanhos de array e números de threads/processos
ARRAY_SIZES=(1000 5000 10000 50000 100000)
NUM_THREADS_PROCESSES=(1 2 4 8) # Usado para OpenMP e MPI

# --- Testes para odd_even_serial.c ---
echo "" | tee -a $OUTPUT_FILE
echo "--- Rodando odd_even_serial ---" | tee -a $OUTPUT_FILE
for size in "${ARRAY_SIZES[@]}"; do
    echo "Tamanho do Array: $size, Algoritmo: odd_even_serial" | tee -a $OUTPUT_FILE
    ./odd_even_serial $size | tee -a $OUTPUT_FILE
    echo "" | tee -a $OUTPUT_FILE
done

# --- Testes para qsort_serial.c (Opcional, se você o incluiu) ---
echo "" | tee -a $OUTPUT_FILE
echo "--- Rodando qsort_serial ---" | tee -a $OUTPUT_FILE
for size in "${ARRAY_SIZES[@]}"; do
    echo "Tamanho do Array: $size, Algoritmo: qsort_serial" | tee -a $OUTPUT_FILE
    ./qsort_serial $size | tee -a $OUTPUT_FILE
    echo "" | tee -a $OUTPUT_FILE
done

# --- Testes para odd_even_openmp.c ---
echo "" | tee -a $OUTPUT_FILE
echo "--- Rodando odd_even_openmp ---" | tee -a $OUTPUT_FILE
for size in "${ARRAY_SIZES[@]}"; do
    for threads in "${NUM_THREADS_PROCESSES[@]}"; do
        echo "Tamanho do Array: $size, Algoritmo: odd_even_openmp, Threads: $threads" | tee -a $OUTPUT_FILE
        ./odd_even_openmp $size $threads | tee -a $OUTPUT_FILE
        echo "" | tee -a $OUTPUT_FILE
    done
done

# --- Testes para odd_even_mpi.c ---
echo "" | tee -a $OUTPUT_FILE
echo "--- Rodando odd_even_mpi ---" | tee -a $OUTPUT_FILE
for size in "${ARRAY_SIZES[@]}"; do
    for processes in "${NUM_THREADS_PROCESSES[@]}"; do
        echo "Tamanho do Array: $size, Algoritmo: odd_even_mpi, Processos: $processes" | tee -a $OUTPUT_FILE
        # O mpirun pode se comportar estranhamente com 1 processo dependendo da implementação/ambiente
        # No geral, o tempo com 1 processo MPI ou OpenMP deve ser sua referência "paralela uniprocessador"
        # mas o serial puro é a referência ideal.
        mpirun -np $processes ./odd_even_mpi $size | tee -a $OUTPUT_FILE
        echo "" | tee -a $OUTPUT_FILE
    done
done

echo "---------------------------------------------------" | tee -a $OUTPUT_FILE
echo "Experimentos concluídos. Resultados em: $OUTPUT_FILE" | tee -a $OUTPUT_FILE