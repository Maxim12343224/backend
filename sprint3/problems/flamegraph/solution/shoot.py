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
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


# Получаем команду для запуска сервера
server_command = start_server()

# Запускаем сервер в фоновом режиме
server_process = run(server_command)

# Даем серверу время на запуск
time.sleep(2)

# Запускаем perf record для записи трассировки серверного процесса
perf_record = run(f'perf record -o perf.data -p {server_process.pid} -g')

# Выполняем обстрел сервера запросами
make_shots()

# Останавливаем запись perf
stop(perf_record, wait=True)

# Даем perf время на завершение записи
time.sleep(1)

# Останавливаем сервер
stop(server_process, wait=True)

# Строим флеймграф с помощью perf script и скриптов FlameGraph
print("Generating flamegraph...")

# perf script -> stackcollapse -> flamegraph -> graph.svg
perf_script = subprocess.Popen(
    ['perf', 'script', '-i', 'perf.data'],
    stdout=subprocess.PIPE,
    stderr=subprocess.DEVNULL
)

stackcollapse = subprocess.Popen(
    ['./FlameGraph/stackcollapse-perf.pl'],
    stdin=perf_script.stdout,
    stdout=subprocess.PIPE,
    stderr=subprocess.DEVNULL
)
perf_script.stdout.close()

with open('graph.svg', 'w') as f:
    flamegraph = subprocess.Popen(
        ['./FlameGraph/flamegraph.pl'],
        stdin=stackcollapse.stdout,
        stdout=f,
        stderr=subprocess.DEVNULL
    )
stackcollapse.stdout.close()

# Ждем завершения всех процессов
perf_script.wait()
stackcollapse.wait()
flamegraph.wait()

print('Job done')
print('Flamegraph saved as graph.svg')