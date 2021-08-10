# ettus/buildbot-worker:ubuntu-16.04-amd64

# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

# Provides a base Ubuntu (20.04) image with latest buildbot worker installed
# the worker image is not optimized for size, but rather uses ubuntu for wider package availability

FROM        ubuntu:20.04
MAINTAINER  Ettus Research

#COPY buildbot.tac /buildbot/buildbot.tac

# Last build date - this can be updated whenever there are security updates so
# that everything is rebuilt
ENV         security_updates_as_of 2018-07-25

# This will make apt-get install without question
ARG         DEBIAN_FRONTEND=noninteractive

# Install security updates and required packages
RUN         apt-get update && \
            # apt-get -y upgrade && \ We want to stay at release configuration
            apt-get -y install -q \
                build-essential \
                ccache \
                git \
                subversion \
                libffi-dev \
                libssl-dev \
                curl \
# Install UHD dependencies
                libboost-all-dev \
                libusb-1.0-0-dev \
                libudev-dev \
                libgps-dev \
                doxygen \
                cmake \
                libgps-dev \
                python3-dev \
                python3-pip \
                python3-mako \
                python3-docutils \
                python3-requests \
                python3-numpy \
                abi-dumper \
                dpdk \
                libdpdk-dev \
                ncompress \
# Mingw build deps
                mingw-w64 \
                autoconf \
                && \
            rm -rf /var/lib/apt/lists/* && \
# Test runs produce a great quantity of dead grandchild processes.  In a
# non-docker environment, these are automatically reaped by init (process 1),
# so we need to simulate that here.  See https://github.com/Yelp/dumb-init
            curl -Lo /usr/local/bin/dumb-init https://github.com/Yelp/dumb-init/releases/download/v1.1.3/dumb-init_1.1.3_amd64 && \
            chmod +x /usr/local/bin/dumb-init

COPY build_boost.sh ./build_boost.sh
# build boost for mingw
RUN           curl -L https://downloads.sourceforge.net/project/boost/boost/1.58.0/boost_1_58_0.tar.gz -o boost.tar.gz && \
              mkdir boost_src && \
              tar xf boost.tar.gz -C boost_src --strip=1 && \
              cd boost_src && echo "using gcc : : x86_64-w64-mingw32-g++ ;" > user_config.jam \
              ../build_boost.sh x86_64 && \
              cd ../ && rm -rf boost_src && mkdir boost_src && \
              tar xf boost.tar.gz -C boost_src --strip=1 && \
              cd boost_src && echo "using gcc : : i686-w64-mingw32-g++ ;" > user_config.jam \
              ../build_boost.sh i686
# build libusb for mingw
RUN apt-get update && apt-get -y install -q gawk
COPY build_libusb.sh ./build_libusb.sh
RUN           curl -L https://github.com/libusb/libusb/archive/v1.0.21.tar.gz -o libusb.tar.gz && \
              mkdir libusb_src && tar xf libusb.tar.gz -C libusb_src --strip=1 && \
              cd libusb_src && \
              ../build_libusb.sh x86_64 && \
              cd ../ && rm -rf libusb_src && mkdir libusb_src && \
              tar xzf libusb.tar.gz -C libusb_src --strip=1 && \
              cd libusb_src && \
              ../build_libusb.sh i686

# download coverity tools and install it to /usr/local/share/cov-build
RUN           curl -L https://scan.coverity.com/download/linux64 \
              --data "token=f-NGkL9tyT-qSsdVxAB3Vg&project=UHD" -o coverity_tool.tgz && \
              mkdir -p /usr/local/share/cov-build && \
              tar xzf coverity_tool.tgz -C /usr/local/share/cov-build --strip-components=2



# Update pip
RUN pip3 install -U pip
## Install required python packages, and twisted
#RUN            pip3 --no-cache-dir install \
#                'buildbot-worker' \
#                'twisted[tls]' && \
#               useradd -u 2017 -ms /bin/bash buildbot && chown -R buildbot /buildbot
#RUN            pip install --upgrade idna
#VOLUME /data /cache
#
#USER buildbot
#
#WORKDIR /buildbot
#
#CMD ["/usr/local/bin/dumb-init", "twistd", "-ny", "buildbot.tac"]