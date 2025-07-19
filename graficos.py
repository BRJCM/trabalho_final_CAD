import matplotlib.pyplot as plt
import re
import os

# Definir globalmente para uso em gráficos e cálculos
NUM_THREADS_PROCESSES = [1, 2, 4, 8]
ARRAY_SIZES = [1000, 5000, 10000, 50000, 100000]

def parse_results(file_path):
    """
    Lê e processa os resultados do arquivo de texto gerado pelo script shell.
    Agora mais robusto na extração de cada bloco de dados.
    """
    results = []
    
    with open(file_path, 'r') as f:
        # Lê o arquivo inteiro e o divide em blocos baseados em "Tamanho do Array:"
        content = f.read()
        blocks = re.split(r'(Tamanho do Array: \d+, Algoritmo: .*)', content)
    
    # O primeiro elemento de blocks será o cabeçalho/vazio. Ignoramos.
    # Os pares subsequentes são (header_line, data_block)
    for i in range(1, len(blocks), 2):
        header_line = blocks[i].strip()
        data_block = blocks[i+1].strip()
        
        current_run = {}
        
        # 1. Extrair informações do cabeçalho (Tamanho, Algoritmo, Unidades)
        match_header = re.match(r"Tamanho do Array: (\d+), Algoritmo: (odd_even_serial|qsort_serial|odd_even_openmp|odd_even_mpi)(?:, (?:Threads|Processos): (\d+))?", header_line)
        if match_header:
            current_run['array_size'] = int(match_header.group(1))
            current_run['algorithm'] = match_header.group(2)
            current_run['num_units'] = int(match_header.group(3)) if match_header.group(3) else 1 # Se não tiver threads/procs, é 1
        else:
            continue # Pula este bloco se o cabeçalho não for reconhecido

        # 2. Extrair métricas do bloco de dados
        # Inicializa valores para campos que podem não estar presentes em todas as execuções (e.g., comm_time para serial)
        current_run['time'] = 0.0
        current_run['comm_time'] = 0.0
        current_run['overhead'] = 0.0

        for line in data_block.splitlines():
            line = line.strip()

            # Extrair Tempo de Execução principal
            # Captura o número que precede " segundos" ou " s"
            match_time = re.search(r"Tempo (?:médio de execução|Total \(max\))(?:\s+\(max\))?.*?: ([\d\.]+) (?:segundos|s)", line)
            if match_time:
                current_run['time'] = float(match_time.group(1))
            
            # Extrair Tempo Comunicação (soma)
            match_comm_time = re.search(r"Tempo Comunicação \(soma\): ([\d\.]+) s", line)
            if match_comm_time:
                current_run['comm_time'] = float(match_comm_time.group(1))

            # Extrair Overhead (%)
            match_overhead = re.search(r"Overhead \(aprox\): ([\d\.]+)%", line)
            if match_overhead:
                current_run['overhead'] = float(match_overhead.group(1))
            
        results.append(current_run)
    
    # Filtrar resultados incompletos se ainda houver algum problema de parsing
    results = [r for r in results if 'time' in r and r['time'] > 0 and 'array_size' in r and 'algorithm' in r and 'num_units' in r]
    
    return results

def calculate_metrics(results):
    """
    Calcula Speedup e Eficiência para cada experimento.
    """
    # Dicionário para armazenar tempos seriais de referência (odd_even_serial)
    serial_ref_times = {}
    for r in results:
        if r['algorithm'] == 'odd_even_serial' and r['num_units'] == 1:
            serial_ref_times[r['array_size']] = r['time']
    
    for r in results:
        if r['algorithm'] not in ['odd_even_serial', 'qsort_serial']: # Speedup e Eficiência só para paralelos
            t_serial = serial_ref_times.get(r['array_size'])
            if t_serial is not None and t_serial > 0 and r['time'] > 0: # Evitar divisão por zero e garantir tempo serial > 0
                r['speedup'] = t_serial / r['time']
                r['efficiency'] = r['speedup'] / r['num_units']
            else:
                r['speedup'] = 0.0 # Se a referência serial estiver faltando ou for zero
                r['efficiency'] = 0.0
        else: # Para serial, speedup e efficiency são 1
            r['speedup'] = 1.0
            r['efficiency'] = 1.0
    return results

def plot_execution_time_vs_size(results):
    """
    Gera o gráfico de Tempo de Execução vs. Tamanho do Problema para todas as versões.
    """
    plt.figure(figsize=(12, 7))
    for algo in ['odd_even_serial', 'qsort_serial', 'odd_even_openmp', 'odd_even_mpi']:
        units_to_plot = 4 # Ex: usar 4 threads/processos para representar os paralelos neste gráfico comparativo
        
        if algo == 'odd_even_openmp':
            subset = [r for r in results if r['algorithm'] == algo and r['num_units'] == units_to_plot]
            label = f'OpenMP ({units_to_plot} Threads)'
        elif algo == 'odd_even_mpi':
            subset = [r for r in results if r['algorithm'] == algo and r['num_units'] == units_to_plot]
            label = f'MPI ({units_to_plot} Processos)'
        else: # Serial e Qsort Serial sempre com 1 unidade
            subset = [r for r in results if r['algorithm'] == algo and r['num_units'] == 1]
            label = algo.replace('_', ' ').title()

        # Garante que os pontos estejam ordenados pelo tamanho do array
        subset = sorted(subset, key=lambda x: x['array_size'])
        sizes = [s['array_size'] for s in subset]
        times = [s['time'] for s in subset]
        
        if times: # Só plota se tiver dados
            plt.plot(sizes, times, marker='o', label=label)

    plt.xscale('log') 
    plt.yscale('log')
    plt.title('Tempo de Execução vs. Tamanho do Problema')
    plt.xlabel('Tamanho do Array (log scale)')
    plt.ylabel('Tempo de Execução (segundos, log scale)')
    plt.legend()
    plt.grid(True, which="both", ls="--", c='0.7')
    plt.tight_layout()
    plt.savefig('1_tempo_execucao_vs_tamanho.png')
    plt.show()

def plot_individual_scaling(results, algorithm_type):
    """
    Gera gráficos de Speedup e Eficiência vs. Threads/Processos para um algoritmo específico (OpenMP ou MPI).
    """
    algo_name_display = algorithm_type.replace('_', ' ').title()
    unit_label = 'Threads' if algorithm_type == 'odd_even_openmp' else 'Processos'
    
    plt.figure(figsize=(12, 6))
    plt.subplot(1, 2, 1) # Speedup
    for size in sorted(list(set([r['array_size'] for r in results if r['algorithm'] == algorithm_type]))):
        algo_data = sorted([r for r in results if r['algorithm'] == algorithm_type and r['array_size'] == size], key=lambda x: x['num_units'])
        
        if algo_data and len(algo_data) > 1: # Precisa de mais de 1 ponto para plotar linha
            plt.plot([d['num_units'] for d in algo_data], [d['speedup'] for d in algo_data], marker='o', label=f'Tamanho {size}')
    
    plt.title(f'Speedup {algo_name_display} vs. {unit_label}')
    plt.xlabel(f'Número de {unit_label} (p)')
    plt.ylabel('Speedup (S)')
    plt.xticks(NUM_THREADS_PROCESSES)
    plt.legend()
    plt.grid(True)

    plt.subplot(1, 2, 2) # Efficiency
    for size in sorted(list(set([r['array_size'] for r in results if r['algorithm'] == algorithm_type]))):
        algo_data = sorted([r for r in results if r['algorithm'] == algorithm_type and r['array_size'] == size], key=lambda x: x['num_units'])
        
        if algo_data and len(algo_data) > 1:
            plt.plot([d['num_units'] for d in algo_data], [d['efficiency'] for d in algo_data], marker='x', linestyle='--', label=f'Tamanho {size}')
    
    plt.title(f'Eficiência {algo_name_display} vs. {unit_label}')
    plt.xlabel(f'Número de {unit_label} (p)')
    plt.ylabel('Eficiência (E)')
    plt.xticks(NUM_THREADS_PROCESSES)
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig(f'2_{algorithm_type}_scaling.png')
    plt.show()

def plot_mpi_communication_overhead(results):
    """
    Gera gráficos de Tempo de Comunicação e Overhead (%) vs. Tamanho do Problema para MPI.
    """
    plt.figure(figsize=(12, 6))
    
    # Subplot 1: Tempo de Comunicação vs. Tamanho
    plt.subplot(1, 2, 1)
    for units in sorted(list(set([r['num_units'] for r in results if r['algorithm'] == 'odd_even_mpi']))):
        mpi_data = sorted([r for r in results if r['algorithm'] == 'odd_even_mpi' and r['num_units'] == units], key=lambda x: x['array_size'])
        
        sizes = [d['array_size'] for d in mpi_data]
        comm_times = [d['comm_time'] for d in mpi_data]

        if comm_times:
            # Substituir 0.0 por um valor muito pequeno para visualização em escala logarítmica
            comm_times_log = [max(t, 1e-9) for t in comm_times] # Use 1e-9 ou valor similar, se houver 0
            plt.plot(sizes, comm_times_log, marker='o', label=f'{units} Processos')
    
    plt.xscale('log')
    plt.yscale('log')
    plt.title('Tempo de Comunicação MPI vs. Tamanho')
    plt.xlabel('Tamanho do Array (log scale)')
    plt.ylabel('Tempo de Comunicação (s, log scale)')
    plt.legend()
    plt.grid(True, which="both", ls="--", c='0.7')

    # Subplot 2: Overhead (%) vs. Tamanho
    plt.subplot(1, 2, 2)
    for units in sorted(list(set([r['num_units'] for r in results if r['algorithm'] == 'odd_even_mpi']))):
        mpi_data = sorted([r for r in results if r['algorithm'] == 'odd_even_mpi' and r['num_units'] == units], key=lambda x: x['array_size'])
        
        sizes = [d['array_size'] for d in mpi_data]
        overheads = [d['overhead'] for d in mpi_data]

        if overheads:
            plt.plot(sizes, overheads, marker='x', linestyle='--', label=f'{units} Processos')
    
    plt.xscale('log')
    plt.title('Overhead MPI (%) vs. Tamanho')
    plt.xlabel('Tamanho do Array (log scale)')
    plt.ylabel('Overhead (%)')
    plt.legend()
    plt.grid(True, which="both", ls="--", c='0.7')
    
    plt.tight_layout()
    plt.savefig('3_mpi_communication_overhead.png')
    plt.show()

def plot_cross_paradigm_comparison_for_size(results, array_size_to_compare):
    """
    Compara Speedup e Eficiência de OpenMP e MPI para um tamanho de array específico.
    Esta é a nova função que será chamada em loop para diferentes tamanhos.
    """
    plt.figure(figsize=(12, 6))
    
    openmp_data = sorted([r for r in results if r['algorithm'] == 'odd_even_openmp' and r['array_size'] == array_size_to_compare], key=lambda x: x['num_units'])
    mpi_data = sorted([r for r in results if r['algorithm'] == 'odd_even_mpi' and r['array_size'] == array_size_to_compare], key=lambda x: x['num_units'])

    # Subplot 1: Speedup
    plt.subplot(1, 2, 1)
    if openmp_data and len(openmp_data) > 1:
        plt.plot([d['num_units'] for d in openmp_data], [d['speedup'] for d in openmp_data], marker='o', label='OpenMP')
    if mpi_data and len(mpi_data) > 1:
        plt.plot([d['num_units'] for d in mpi_data], [d['speedup'] for d in mpi_data], marker='x', linestyle='--', label='MPI')
    
    plt.title(f'Speedup (Tamanho: {array_size_to_compare})')
    plt.xlabel('Número de Unidades (p)')
    plt.ylabel('Speedup (S)')
    plt.xticks(NUM_THREADS_PROCESSES)
    plt.legend()
    plt.grid(True)

    # Subplot 2: Efficiency
    plt.subplot(1, 2, 2)
    if openmp_data and len(openmp_data) > 1:
        plt.plot([d['num_units'] for d in openmp_data], [d['efficiency'] for d in openmp_data], marker='o', label='OpenMP')
    if mpi_data and len(mpi_data) > 1:
        plt.plot([d['num_units'] for d in mpi_data], [d['efficiency'] for d in mpi_data], marker='x', linestyle='--', label='MPI')
    
    plt.title(f'Eficiência (Tamanho: {array_size_to_compare})')
    plt.xlabel('Número de Unidades (p)')
    plt.ylabel('Eficiência (E)')
    plt.xticks(NUM_THREADS_PROCESSES)
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig(f'4_comparacao_speedup_eficiencia_tamanho_{array_size_to_compare}.png')
    plt.show()


# --- Execução do Script ---
if __name__ == "__main__":
    results_file = "resultados_experimentos.txt"
    if not os.path.exists(results_file):
        print(f"Erro: Arquivo '{results_file}' não encontrado. Certifique-se de que o script 'run_experiments.sh' foi executado e o arquivo foi gerado.")
    else:
        print("Analisando resultados...")
        parsed_data = parse_results(results_file)
        
        if not parsed_data:
            print("Nenhum dado válido encontrado após o parsing. Verifique o formato do 'resultados_experimentos.txt'.")
        else:
            calculated_data = calculate_metrics(parsed_data)
            
            # --- Gerar os Gráficos ---
            print("Gerando gráfico 1: Tempo de Execução vs. Tamanho do Problema...")
            plot_execution_time_vs_size(calculated_data)

            print("Gerando gráficos 2: Escalabilidade OpenMP (Speedup e Eficiência)...")
            plot_individual_scaling(calculated_data, 'odd_even_openmp')

            print("Gerando gráficos 3: Escalabilidade MPI (Speedup e Eficiência)...")
            plot_individual_scaling(calculated_data, 'odd_even_mpi')

            print("Gerando gráficos 4: Overhead de Comunicação MPI...")
            plot_mpi_communication_overhead(calculated_data)

            # Gerar gráficos de comparação para CADA tamanho de array
            print("Gerando gráficos 5 (comparação OpenMP vs MPI para cada tamanho de array)...")
            for size in sorted(list(set([r['array_size'] for r in calculated_data if r['algorithm'] in ['odd_even_openmp', 'odd_even_mpi']]))):
                plot_cross_paradigm_comparison_for_size(calculated_data, size)

            print("Todos os gráficos foram gerados e salvos como .png na pasta atual.")