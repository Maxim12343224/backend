#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint4/problems/leave_game/solution
GET_IP=${SCRIPT_FOLDER}/../get_ip.py

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

docker container stop postgres && docker container rm postgres

POSTGRES_ID=$(docker run --name postgres -e POSTGRES_HOST_AUTH_METHOD=trust -d --rm postgres)

sleep 5s # wait while postgres is setting up

export POSTGRES_USER=postgres
export POSTGRES_PASSWORD=Mys3Cr3t
export POSTGRES_HOST=$(python3 ${GET_IP} $POSTGRES_ID)
export POSTGRES_PORT=5432

export IMAGE_NAME=leave_game
export CONFIG_PATH=${SOLUTION_FOLDER}/data/config.json

pytest --workers auto --junitxml=${BASE_DIR}/leave_game.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s04_leave_game.py

docker container stop postgres