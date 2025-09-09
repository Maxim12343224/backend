# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Создаем рабочую директорию
WORKDIR /app

# Копируем conanfile.txt и устанавливаем зависимости
COPY conanfile.txt .
RUN mkdir build && \
    cd build && \
    conan install .. \
        --build=missing \
        -s compiler.libcxx=libstdc++11 \
        -s build_type=Release

# Копируем исходный код, тесты и данные
COPY ./src ./src
COPY ./test ./test 2>/dev/null || echo "Test directory not found, skipping"
COPY ./data ./data
COPY ./static ./static
COPY CMakeLists.txt .

# Собираем проект
RUN cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . && \
    # Запускаем тесты, если они есть
    if [ -f "game_server_tests" ]; then \
        echo "Running tests..."; \
        ./game_server_tests --success; \
    else \
        echo "No tests to run"; \
    fi

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Обновляем и устанавливаем минимальные зависимости
RUN apt update && \
    apt install -y --no-install-recommends \
        libstdc++6 \
        libgcc-s1 \
    && \
    rm -rf /var/lib/apt/lists/*

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Создаем рабочую директорию
WORKDIR /app

# Скопируем приложение со сборочного контейнера
COPY --from=build /app/build/bin/game_server .
COPY --from=build /app/data ./data
COPY --from=build /app/static ./static

# Запускаем игровой сервер с поддержкой генерации лута
ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static"]