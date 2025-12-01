
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*


COPY conanfile.txt /app/
RUN mkdir -p /app/build && cd /app/build && \
    conan install .. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Release

# Копируем все исходные файлы
COPY . /app/

# основной сервер
RUN cd /app/build && \
    cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target game_server

# Финальный образ для запуска
FROM ubuntu:22.04 as run

# PostgreSQL
RUN apt update && \
    apt install -y \
      libstdc++6 \
      libpq5 \
      libpqxx-7.7 \
      libpqxx-dev && \
    rm -rf /var/lib/apt/lists/*

# Создаем пользователя и рабочие директории
RUN groupadd -r app && useradd -r -g app app
USER app
WORKDIR /app

# Создаем директорию для файлов состояния
RUN mkdir -p /app/state

# Копируем бинарник из стадии сборки
COPY --from=build /app/build/bin/game_server /app/
COPY --from=build /app/data /app/data
COPY --from=build /app/static /app/static


CMD ["/app/game_server", \
    "--config-file", "/app/data/config.json", \
    "--www-root", "/app/static", \