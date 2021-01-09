FROM pirateteam/llvm-centos7-x86_64
RUN yum install -y epel-release \
 && yum install -y cmake3 \
           devtoolset-7-gcc devtoolset-7-libstdc++-devel \
           libyaml-devel-0.1.4-11.el7_0.x86_64 \
           valgrind \
           openssl11 openssl11-devel \
           libjpeg-devel libX11-devel \
 && yum clean all \
 && rm -rf /var/cache/yum


RUN ln -s /usr/bin/cmake3 /usr/local/bin/cmake
COPY libpirate /usr/local
COPY pal /usr/local
COPY libpirategetopt /usr/local
# Update library path to include libpirate
RUN ldconfig
WORKDIR /root