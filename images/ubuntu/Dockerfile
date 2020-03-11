FROM pirateteam/llvm-ubuntu:latest
MAINTAINER Joe Hendrix (jhendrix@galois.com)
RUN apt-get update \
 && apt-get install --no-install-recommends -y libjpeg-dev libssl-dev libx11-dev \
 && rm -rf /var/lib/apt/lists/*
COPY dist/libpirate /usr/local
COPY doc   /root/pirate/doc
COPY libpirate /root/pirate/libpirate
COPY demos /root/pirate/demos
COPY CMakeLists.txt /root/pirate/CMakeLists.txt
WORKDIR /root
