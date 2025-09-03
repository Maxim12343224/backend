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
    parser = argparse.ArgumentParser(description='Load testing with flamegraph generation')
    parser.add_argument('server', type=str, help='Server command to execute')
    return parser.parse_args().server


def run(command, output=None):
    """Run a command in subprocess"""
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    """Stop a process gracefully"""
    if process.poll() is None:  # Process is still running
        if wait:
            process.wait()
        process.terminate()


def shoot(ammo):
    """Execute a single HTTP request"""
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    """Execute multiple HTTP requests"""
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def generate_flamegraph():
    """Generate flamegraph from perf data"""
    print("Generating flamegraph...")

    try:
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

        # Ждем завершения всех процессов
        _, perf_err = perf_script.communicate()
        _, collapse_err = stackcollapse.communicate()
        _, flame_err = flamegraph.communicate()

        if perf_err:
            print(f"perf script warning: {perf_err.decode().strip()}")
        if collapse_err:
            print(f"stackcollapse warning: {collapse_err.decode().strip()}")
        if flame_err:
            print(f"flamegraph warning: {flame_err.decode().strip()}")

    except Exception as e:
        print(f"Error generating flamegraph: {e}")
        return False
    
    return True


def main():
    """Main function"""
    try:
        # Запускаем сервер
        server_command = start_server()
        server_process = run(server_command)
        
        # Даем серверу время на запуск
        time.sleep(2)
        
        # Запускаем запись perf
        perf_record = run(f'perf record -o perf.data -p {server_process.pid} -g')
        
        # Выполняем нагрузочное тестирование
        make_shots()
        
        # Останавливаем запись perf и сервер (неблокирующе)
        stop(perf_record, wait=False)
        stop(server_process, wait=False)
        
        # Даем процессам время на корректное завершение
        time.sleep(1)
        
        # Генерируем flamegraph
        success = generate_flamegraph()
        
        # Проверяем результат
        if success and os.path.exists('graph.svg') and os.path.getsize('graph.svg') > 0:
            print('Job done')
            print('Flamegraph saved as graph.svg')
            return 0
        else:
            print('Error: graph.svg was not created or is empty')
            return 1
            
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        stop(perf_record, wait=False)
        stop(server_process, wait=False)
        return 1
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())