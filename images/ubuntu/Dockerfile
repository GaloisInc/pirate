FROM pirateteam/llvm-ubuntu-x86_64:latest
RUN apt-get update \
 && apt-get install --no-install-recommends -y libpython3.8 libjpeg-dev libssl-dev libx11-dev libyaml-dev openssl \
 && rm -rf /var/lib/apt/lists/*
COPY libpirate /usr/local
COPY pal /usr/local
COPY libpirategetopt /usr/local
# Update library path to include libpirate
RUN ldconfig
WORKDIR /root
