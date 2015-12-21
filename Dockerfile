FROM ubuntu
MAINTAINER IronicBadger <ironicbadger@linuxserver.io>

# Install MHDDFS compile pre-reqs for 14.04
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y \
      build-essential \
      libattr1-dev \
      libglib2.0-dev \
      dpkg-dev \
      git \
      curl \ 
      debhelper && \
    curl -LO http://ge.archive.ubuntu.com/ubuntu/pool/main/libs/libsepol/libsepol1_2.3-2_amd64.deb && \
    curl -LO http://ge.archive.ubuntu.com/ubuntu/pool/main/libs/libselinux/libselinux1_2.3-2build1_amd64.deb && \
    curl -LO http://ge.archive.ubuntu.com/ubuntu/pool/main/libs/libsepol/libsepol1-dev_2.3-2_amd64.deb && \
    curl -LO http://ge.archive.ubuntu.com/ubuntu/pool/main/libs/libselinux/libselinux1-dev_2.3-2build1_amd64.deb && \
    curl -LO http://ge.archive.ubuntu.com/ubuntu/pool/main/f/fuse/libfuse2_2.9.4-1ubuntu1_amd64.deb && \
    curl -LO http://ge.archive.ubuntu.com/ubuntu/pool/main/f/fuse/libfuse-dev_2.9.4-1ubuntu1_amd64.deb && \
    dpkg -i *.deb && \
    rm *.deb && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Add source
RUN mkdir -p /tmp/mhddfs-nosegfault /artifact
ADD . /tmp/mhddfs-nosegfault/

RUN cd /tmp/mhddfs-nosegfault && \
    dpkg-buildpackage && \
    cp /tmp/*.deb /artifact/ && \
    rm -rf /tmp/mhddfs-nosegfault
