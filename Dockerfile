FROM mcr.microsoft.com/devcontainers/base:ubuntu

WORKDIR /app
COPY . .

RUN apt-get update && apt-get install -y \
    xz-utils \
    gcc \
    clang \
    git-all \
    git-core \
    sudo \
    build-essential \
    cmake \
    valgrind \
    libcppunit-dev \
    zsh \
    curl \
    libreadline-dev

RUN make re
