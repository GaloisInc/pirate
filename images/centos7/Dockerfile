FROM centos:7
WORKDIR /root

# Install packages
RUN yum install -y centos-release-scl epel-release \
 && yum install -y \
           wget \
           cmake3 devtoolset-8-make \
           devtoolset-8-gcc devtoolset-8-libstdc++-devel \
           libedit \
           libyaml-devel-0.1.4-11.el7_0.x86_64 \
           valgrind \
           openssl11 openssl11-devel \
           libjpeg-devel libX11-devel \
 && yum clean all \
 && rm -rf /var/cache/yum

# Append devtoolset-8 to end of path so it does not take precedence over /usr/local/bin
ENV PATH="$PATH:/opt/rh/devtoolset-8/root/bin"
RUN ln -s /usr/bin/cmake3 /usr/local/bin/cmake 
RUN ln -s /usr/bin/openssl11 /usr/local/bin/openssl

# Download pirate artifacts
RUN wget -qO /root/pirate.tgz https://pirate.azureedge.net/dist/pirate/master/pirate.centos7.tgz \
 && tar -C /usr/local -xz --strip-components=1 -f /root/pirate.tgz \
 && rm -rf /root/pirate.tgz

# Update library path to include libpirate
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/libc.conf
RUN ldconfig