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


def perf_record(pid, output_file="perf.data"):
    """Запускает perf record для указанного процесса"""
    return run(f"perf record -o {output_file} -p {pid} -g")


def stop_perf(perf_process):
    """Корректно останавливает perf record"""
    perf_process.send_signal(signal.SIGINT)
    if perf_process.poll() is None:
        perf_process.wait()


def generate_flamegraph(perf_data="perf.data", output_svg="graph.svg"):
    """Генерирует флеймграф из данных perf"""
    
    if not os.path.exists(perf_data):
        raise FileNotFoundError(f"File {perf_data} not found")
    
   
    perf_script = run(f"perf script -i {perf_data}", output=subprocess.PIPE)
    
    stackcollapse = run("./FlameGraph/stackcollapse-perf.pl", output=subprocess.PIPE)
    stackcollapse.stdin = perf_script.stdout
    
    with open(output_svg, "w") as svg_file:
        flamegraph = run("./FlameGraph/flamegraph.pl", output=svg_file)
        flamegraph.stdin = stackcollapse.stdout
        flamegraph.wait()



server_command = start_server()
server_process = run(server_command)

try:
    
    time.sleep(2)
    
    
    perf_process = perf_record(server_process.pid)
    
    
    time.sleep(1)
    
   
    make_shots()
    
   
    stop_perf(perf_process)
    
    
    generate_flamegraph()
    
finally:
    
    stop(server_process, wait=True)

time.sleep(1)
print('Job done')