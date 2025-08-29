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


def run(command, output=None, input_pipe=None):
    process = subprocess.Popen(shlex.split(command), 
                              stdout=output, 
                              stderr=subprocess.DEVNULL,
                              stdin=input_pipe)
    return process


def stop(process, wait=False):
    if process.poll() is None:
        if wait:
            process.wait()
        else:
            process.terminate()


def shoot(ammo):
    hit = run(f"curl -s {ammo}", output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
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
    
    if not os.path.exists(perf_data):
        raise FileNotFoundError(f"File {perf_data} not found")
    
    # Правильный пайплайн: perf script | stackcollapse | flamegraph
    perf_script = run(f"perf script -i {perf_data}", output=subprocess.PIPE)
    
    # Ждем завершения perf script и передаем вывод в stackcollapse
    perf_output, _ = perf_script.communicate()
    
    stackcollapse = run("./FlameGraph/stackcollapse-perf.pl", 
                       input_pipe=subprocess.PIPE,
                       output=subprocess.PIPE)
    
    # Ждем завершения stackcollapse
    stackcollapse_output, _ = stackcollapse.communicate(input=perf_output)
    
    # Записываем результат в файл
    with open(output_svg, "w") as svg_file:
        flamegraph = run("./FlameGraph/flamegraph.pl", 
                        input_pipe=subprocess.PIPE,
                        output=svg_file)
        flamegraph.communicate(input=stackcollapse_output)


# Основной код
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
    
finally:
    # Гарантированно останавливаем сервер
    stop(server_process, wait=True)

time.sleep(1)
print('Job done')