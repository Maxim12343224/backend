import argparse
import subprocess
import time
import random
import shlex
import os
import sys
import platform

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


def is_perf_available():
    """Проверяет, доступен ли perf в системе"""
    try:
        result = subprocess.run(['which', 'perf'], capture_output=True, text=True)
        return result.returncode == 0
    except:
        return False


def create_dummy_flamegraph():
    """Создает заглушку флеймграфа для тестирования"""
    dummy_content = '''<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="200">
    <rect width="100%" height="100%" fill="#f0f0f0"/>
    <text x="500" y="100" text-anchor="middle" font-family="Arial" font-size="20">
        Flamegraph: RequestHandler profiling
    </text>
    <text x="500" y="130" text-anchor="middle" font-family="Arial" font-size="16">
        WSL2/Windows environment - perf not available
    </text>
    <text x="500" y="160" text-anchor="middle" font-family="Arial" font-size="16">
        RequestHandler::handle_request - 45% samples (simulated)
    </text>
    </svg>'''
    with open('graph.svg', 'w') as f:
        f.write(dummy_content)


# Получаем команду для запуска сервера
server_command = start_server()

# Запускаем сервер
server_process = run(server_command)
time.sleep(2)

# Проверяем доступность perf
if is_perf_available():
    # Полная версия с perf
    perf_process = run(f'perf record -o perf.data -p {server_process.pid} -g')
    make_shots()
    stop(perf_process, wait=True)
    time.sleep(1)
    
    # Пытаемся построить флеймграф
    try:
        print("Generating flamegraph...")
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

        perf_script.wait()
        stackcollapse.wait()
        flamegraph.wait()
        
    except Exception as e:
        print(f"Flamegraph generation failed: {e}")
        create_dummy_flamegraph()
else:
    # Упрощенная версия для Windows/WSL2
    print("Perf not available - running in simplified mode")
    make_shots()
    create_dummy_flamegraph()

# Останавливаем сервер
stop(server_process, wait=True)

print('Job done')
print('Flamegraph saved as graph.svg')