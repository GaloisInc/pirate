FROM pirateteam/llvm-ubuntu-x86_64:latest
MAINTAINER Joe Hendrix (jhendrix@galois.com)
RUN apt-get update \
 && apt-get install --no-install-recommends -y libjpeg-dev libssl-dev libx11-dev libyaml-dev openssl \
 && rm -rf /var/lib/apt/lists/*
COPY libpirate /usr/local
COPY pal /usr/local
COPY libpirategetopt /usr/local
# Update library path to include libpirate
RUN ldconfig
COPY doc   /root/pirate/doc
COPY demos /root/pirate/demos
WORKDIR /root
