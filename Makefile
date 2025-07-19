CC = gcc
MPICC = mpicc
CFLAGS = -O2 -Wall -I/usr/lib/x86_64-linux-gnu/mpich/include

all: odd_even_serial odd_even_openmp odd_even_mpi qsort_serial

odd_even_serial: odd_even_serial.c
	$(CC) $(CFLAGS) -o odd_even_serial odd_even_serial.c

odd_even_openmp: odd_even_openmp.c
	$(CC) $(CFLAGS) -fopenmp -o odd_even_openmp odd_even_openmp.c

odd_even_mpi: odd_even_mpi.c
	$(CC) $(CFLAGS) -o odd_even_mpi odd_even_mpi.c -L/usr/lib/x86_64-linux-gnu/mpich/lib -lmpich

qsort_serial: qsort_serial.c
	$(CC) $(CFLAGS) -o qsort_serial qsort_serial.c

clean:
	rm -f odd_even_serial odd_even_openmp odd_even_mpi qsort_serial

test: all
	@echo "--- Testando odd_even_serial com 1000 elementos ---"
	./odd_even_serial 1000
	@echo "\n--- Testando odd_even_openmp com 1000 elementos e 2 threads ---"
	./odd_even_openmp 1000 2
	@echo "\n--- Testando odd_even_mpi com 1000 elementos e 2 processos ---"
	mpirun -np 2 ./odd_even_mpi 1000
	@echo "\n--- Testando qsort_serial com 1000 elementos ---"
	./qsort_serial 1000