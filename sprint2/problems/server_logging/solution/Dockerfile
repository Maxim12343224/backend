FROM gcc:11.3

# Установка зависимостей
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

WORKDIR /app

# Копируем все файлы
COPY . .

# Установка зависимостей и сборка
RUN mkdir build && cd build && \
    conan install .. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Debug && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target game_server

# Создаем пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Запуск сервера
ENTRYPOINT ["/app/build/bin/game_server", "/app/data/config.json"]