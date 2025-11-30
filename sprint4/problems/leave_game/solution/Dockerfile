FROM gcc:11.3 as build

RUN apt update && \
    apt install -y python3-pip cmake && \
    pip3 install conan==1.*

WORKDIR /app
COPY conanfile.txt ./
RUN mkdir -p build && cd build && \
    conan install .. --build=missing -g cmake -s compiler.libcxx=libstdc++11 -s build_type=Release

COPY . .
RUN cd build && \
    cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target game_server && \
    ls -la /app/build/bin/game_server  # проверка

FROM ubuntu:22.04 as run
RUN apt update && \
    apt install -y libstdc++6 libpq5 && \
    rm -rf /var/lib/apt/lists/*

RUN groupadd -r app && useradd -r -g app app
RUN mkdir -p /app/state

# Копируем ВСЕ необходимое
COPY --from=build /app/build/bin/game_server /app/
COPY --from=build /app/data /app/data
COPY --from=build /app/static /app/static

# Даем права и переключаем пользователя
RUN chown -R app:app /app && chmod +x /app/game_server
USER app
WORKDIR /app

CMD ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static"]