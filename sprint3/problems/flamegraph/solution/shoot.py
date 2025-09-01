import argparse
import subprocess
import time
import random
import shlex
import os
import sys

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
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.PIPE)
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


server_command = start_server()

print(f"Starting server: {server_command}")
server_process = run(server_command)

time.sleep(2)

# Проверка что сервер запустился
if server_process.poll() is not None:
    _, server_stderr = server_process.communicate()
    print(f'Error: Server failed to start. Stderr: {server_stderr.decode()}')
    sys.exit(1)

print(f"Server PID: {server_process.pid}")

print("Starting perf record...")
perf_record = run(f'perf record -o perf.data -p {server_process.pid} -g')

# Даем perf время начать запись
time.sleep(1)

make_shots()

# Останавливаем perf и проверяем ошибки
stop(perf_record, wait=True)
_, perf_stderr = perf_record.communicate()
if perf_stderr:
    print(f"perf record errors: {perf_stderr.decode()}")

stop(server_process, wait=True)

time.sleep(1)

# Проверка наличия данных в perf.data
if not os.path.exists('perf.data'):
    print('Error: perf.data does not exist')
    sys.exit(1)
    
if os.path.getsize('perf.data') == 0:
    print('Error: perf.data is empty')
    sys.exit(1)

print(f"perf.data size: {os.path.getsize('perf.data')} bytes")

print("Generating flamegraph...")

# perf script -> stackcollapse -> flamegraph -> graph.svg
perf_script = subprocess.Popen(
    ['perf', 'script', '-i', 'perf.data'],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE
)

stackcollapse = subprocess.Popen(
    ['./FlameGraph/stackcollapse-perf.pl'],
    stdin=perf_script.stdout,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE
)
perf_script.stdout.close()

with open('graph.svg', 'w') as f:
    flamegraph = subprocess.Popen(
        ['./FlameGraph/flamegraph.pl'],
        stdin=stackcollapse.stdout,
        stdout=f,
        stderr=subprocess.PIPE
    )
stackcollapse.stdout.close()

_, perf_err = perf_script.communicate()
_, collapse_err = stackcollapse.communicate()
_, flame_err = flamegraph.communicate()


if perf_err:
    print(f"perf script error: {perf_err.decode()}")
if collapse_err:
    print(f"stackcollapse error: {collapse_err.decode()}")
if flame_err:
    print(f"flamegraph error: {flame_err.decode()}")


if os.path.exists('graph.svg') and os.path.getsize('graph.svg') > 0:
    print('Job done')
    print('Flamegraph saved as graph.svg')
else:
    print('Error: graph.svg was not created or is empty')
    sys.exit(1)