FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libcurl4-openssl-dev \
    libboost-all-dev \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN g++ -std=c++17 -O2 -I/app -o server main.cpp \
    -lcurl -lpthread -lboost_system -lssl -lcrypto

EXPOSE 8080
CMD ["./server"]
