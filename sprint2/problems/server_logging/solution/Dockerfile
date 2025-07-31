FROM gcc:11.3 as build

# Установка зависимостей
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Создаем рабочую директорию
WORKDIR /app

# Копируем conanfile.txt в рабочую директорию
COPY conanfile.txt .

# Устанавливаем зависимости через Conan
RUN mkdir build && cd build && \
    conan install .. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Debug

# Копируем исходный код и CMakeLists.txt
COPY ./src ./src
COPY CMakeLists.txt .

# Собираем проект
RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target game_server

# Второй контейнер для запуска
FROM ubuntu:22.04 as run

# Установка runtime зависимостей (если нужны)
RUN apt update && apt install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Создаем пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Создаем рабочую директорию
WORKDIR /app

# Копируем бинарник и данные из build-контейнера
COPY --from=build /app/build/bin/game_server .
COPY --from=build /app/data ./data

# Запускаем сервер
ENTRYPOINT ["/app/game_server", "/app/data/config.json"]