# Используем многостадийную сборку
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
      postgresql-server-dev-all \
    && \
    pip3 install conan==1.*

# Копируем conanfile.txt
COPY conanfile.txt /app/
RUN mkdir -p /app/build && cd /app/build && \
    conan install .. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Release

# Копируем все исходные файлы
COPY . /app/

# Собираем bookypedia
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target bookypedia

# Собираем тесты (опционально)
RUN cd /app/build && \
    cmake --build . --target tests

# Финальный образ для запуска
FROM ubuntu:22.04 as run

# Установка минимальных зависимостей для PostgreSQL клиента
RUN apt update && \
    apt install -y libstdc++6 libpq5 && \
    rm -rf /var/lib/apt/lists/*

# Создаем пользователя и рабочие директории
RUN groupadd -r app && useradd -r -g app app
USER app
WORKDIR /app

# Копируем бинарник из стадии сборки
COPY --from=build /app/build/bookypedia /app/

# Запускаем программу
CMD ["/app/bookypedia"]