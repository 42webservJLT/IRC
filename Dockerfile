FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    build-essential \
    netcat-openbsd \
    nmap