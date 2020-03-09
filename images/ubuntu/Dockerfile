FROM pirateteam/llvm-ubuntu:latest
MAINTAINER Joe Hendrix (jhendrix@galois.com)
RUN apt-get update \
 && apt-get install --no-install-recommends -y libjpeg-dev libssl-dev libx11-dev \
 && rm -rf /var/lib/apt/lists/*

COPY libpirate /usr/local
COPY doc   /root/pirate/doc
COPY demos /root/pirate/demos
WORKDIR /root