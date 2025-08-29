import argparse
import subprocess
import time
import random
import shlex
import os
import signal

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'http://localhost:8080/api/v1/maps/map1',
    'http://localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None, input=None):
    """Запускает команду и возвращает процесс"""
    return subprocess.Popen(shlex.split(command), 
                          stdout=output, 
                          stderr=subprocess.DEVNULL,
                          stdin=input,
                          text=True)


def stop(process, wait=False):
    """Останавливает процесс"""
    if process.poll() is None:
        if wait:
            process.wait()
        else:
            process.terminate()


def shoot(ammo):
    """Выполняет один запрос к серверу"""
    hit = run(f"curl -s {ammo}", output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    """Выполняет все запросы"""
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def perf_record(pid, output_file="perf.data"):
    """Запускает perf record для указанного процесса"""
    return run(f"perf record -o {output_file} -p {pid} -g")


def stop_perf(perf_process):
    """Корректно останавливает perf record"""
    perf_process.send_signal(signal.SIGINT)
    perf_process.wait()


def generate_flamegraph(perf_data="perf.data", output_svg="graph.svg"):
    """Генерирует флеймграф из данных perf"""
    
    # Проверяем что файл perf.data существует
    if not os.path.exists(perf_data):
        raise FileNotFoundError(f"File {perf_data} not found")
    
    # Проверяем что скрипты FlameGraph существуют
    if not os.path.exists("./FlameGraph/stackcollapse-perf.pl"):
        raise FileNotFoundError("FlameGraph scripts not found")
    
    # Правильный пайплайн: perf script | stackcollapse | flamegraph
    try:
        # Первый процесс: perf script
        perf_script = subprocess.Popen(
            ["perf", "script", "-i", perf_data],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        )
        
        # Второй процесс: stackcollapse
        stackcollapse = subprocess.Popen(
            ["./FlameGraph/stackcollapse-perf.pl"],
            stdin=perf_script.stdout,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        )
        
        # Третий процесс: flamegraph
        with open(output_svg, "w") as svg_file:
            flamegraph = subprocess.Popen(
                ["./FlameGraph/flamegraph.pl"],
                stdin=stackcollapse.stdout,
                stdout=svg_file,
                stderr=subprocess.DEVNULL,
                text=True
            )
            
            # Ждем завершения всех процессов
            perf_script.wait()
            stackcollapse.wait()
            flamegraph.wait()
            
            # Проверяем коды возврата
            if perf_script.returncode != 0:
                raise RuntimeError("perf script failed")
            if stackcollapse.returncode != 0:
                raise RuntimeError("stackcollapse failed")
            if flamegraph.returncode != 0:
                raise RuntimeError("flamegraph failed")
                
    except Exception as e:
        raise RuntimeError(f"Flamegraph generation failed: {e}")


def main():
    """Основная функция"""
    server_command = start_server()
    server_process = run(server_command)

    try:
        # Даем серверу время на запуск
        time.sleep(3)
        
        # Запускаем perf record
        perf_process = perf_record(server_process.pid)
        
        # Даем perf время на запуск
        time.sleep(1)
        
        # Выполняем обстрел
        make_shots()
        
        # Останавливаем perf
        stop_perf(perf_process)
        
        # Генерируем флеймграф
        generate_flamegraph()
        
        print('Flamegraph generated successfully')
        
    except Exception as e:
        print(f'Error: {e}')
        raise
    finally:
        # Гарантированно останавливаем сервер
        stop(server_process, wait=True)

    print('Job done')


if __name__ == "__main__":
    main()