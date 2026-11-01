FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends cmake g++ make && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAVENZA_BUILD_TESTS=OFF && cmake --build build -j2
FROM debian:bookworm-slim
RUN useradd --system --uid 10001 avenza && mkdir /data && chown avenza /data
COPY --from=build /src/build/avenza-server /usr/local/bin/avenza-server
COPY --from=build /src/build/avenza-cli /usr/local/bin/avenza-cli
USER avenza
EXPOSE 7001
VOLUME ["/data"]
ENTRYPOINT ["avenza-server"]
CMD ["--port","7001","--capacity","10000","--data-dir","/data"]
