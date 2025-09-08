# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Запуск conan как раньше
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Debug

# Копируем исходный код, тесты и данные
COPY ./src /app/src
COPY ./test /app/test 2>/dev/null || echo "Test directory not found, skipping"
COPY ./data /app/data
COPY ./static /app/static
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . && \
    # Запускаем тесты, если они есть
    if [ -f "/app/build/game_server_tests" ]; then \
        echo "Running tests..."; \
        /app/build/game_server_tests --success; \
    else \
        echo "No tests to run"; \
    fi

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера
COPY --from=build /app/build/bin/game_server /app/
COPY --from=build /app/data /app/data
COPY --from=build /app/static /app/static

# Запускаем игровой сервер с поддержкой генерации лута
ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static"]