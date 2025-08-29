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


# === ВАШ КОД НАЧИНАЕТСЯ ЗДЕСЬ ===

# 1. Запускаем сервер
server_command = start_server()
server_process = run(server_command)
time.sleep(2)  # ждем запуска

# 2. Запускаем perf record для профилирования
perf_record = run(f'perf record -o perf.data -p {server_process.pid} -g')

# 3. Выполняем обстрел запросами
make_shots()

# 4. Останавливаем запись perf
stop(perf_record, wait=True)
time.sleep(1)

# 5. Останавливаем сервер
stop(server_process, wait=True)

# 6. Строим флеймграф через пайп
print("Generating flamegraph...")

# perf script -> stackcollapse -> flamegraph
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

# Ждем завершения
perf_script.wait()
stackcollapse.wait()
flamegraph.wait()

print('Job done')
print('Flamegraph saved as graph.svg')