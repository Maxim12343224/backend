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
RUN mkdir -p /app/build && cd /app/build && \
    # Изменил --install-folder на . вместо build
    # Добавил --generator-folder для совместимости
    conan install .. --install-folder=. --generator-folder=. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Debug

# Проверка наличия conanbuildinfo.cmake
RUN cd /app/build && \
    [ -f "conanbuildinfo.cmake" ] || (echo "Conan files not found!" && exit 1)

# Папка data больше не нужна
COPY ./src /app/src
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    # Добавил вывод содержимого для отладки
    ls -la && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Путь к бинарнику может отличаться в зависимости от CMake
COPY --from=build /app/build/bin/game_server /app/ || \
    COPY --from=build /app/build/game_server /app/

COPY ./data /app/data

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "/app/data/config.json"]