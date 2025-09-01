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
SERVER_STARTUP_TIME = 2
PERF_FINISH_TIME = 1

def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server

def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process

def stop(process, wait=False):
    if process.poll() is None:
        if wait:
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

def generate_flamegraph():
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

    # Проверяем что graph.svg создан и не пустой
    if os.path.exists('graph.svg') and os.path.getsize('graph.svg') > 0:
        print('Job done')
        print('Flamegraph saved as graph.svg')
        return True
    else:
        print('Error: graph.svg was not created or is empty')
        return False

def main():
    server_command = start_server()
    server_process = run(server_command)

    # Даем серверу время на запуск
    time.sleep(SERVER_STARTUP_TIME)

    perf_record = run(f'perf record -o perf.data -p {server_process.pid} -g')
    make_shots()
    
    stop(perf_record, wait=True)
    stop(server_process, wait=True)
    
    # Даем perf и серверу время на завершение
    time.sleep(PERF_FINISH_TIME)

    if not generate_flamegraph():
        sys.exit(1)

if __name__ == "__main__":
    main()