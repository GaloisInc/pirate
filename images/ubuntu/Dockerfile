FROM ubuntu:focal
WORKDIR /root

# Install packages
RUN apt-get update \
 && TZ=UTC DEBIAN_FRONTEND=noninteractive \
    apt-get install --no-install-recommends -y \
       wget ca-certificates \
       git cmake make libstdc++-9-dev \
       libedit2 libpython3.8 \
       libjpeg-dev libssl-dev libx11-dev libyaml-dev openssl \
 && rm -rf /var/lib/apt/lists/*

# Download pirate artifacts
RUN wget -qO /root/pirate.tgz https://pirate.azureedge.net/dist/pirate/master/pirate.ubuntu.tgz \
 && tar -C /usr/local -xz --strip-components=1 -f /root/pirate.tgz \
 && rm -rf /root/pirate.tgz

# Update library path to include libpirate
RUN ldconfig