FROM fedora
MAINTAINER Milosch Meriac <milosch@meriac.com>

USER root

# update packages
# RUN dnf -y update && dnf clean all

# remove competing vim package
RUN dnf -y remove vim-minimal

# install packages
RUN dnf -y install \
	\
	mc \
	wget \
	bzip2 \
	glibc.i686 \
	vim \
	python-pip \
	git \
	mercurial \
	ccache \
	arm-none-eabi-newlib \
	tar \
	make \
	findutils \
	\
	&& dnf clean all

# install mbed-cli and dependencies
RUN \
	pip install --upgrade pip && \
	pip install mbed-cli pyelftools && \
	pip install -r https://raw.githubusercontent.com/mbedmicro/mbed/master/requirements.txt

#
# Only small changes beyond this step
#

# Install the GNU ARM Embedded Toolchain.
# Done in 2 steps so the download can be decoupled from the installation during
# Docker deployment.
RUN \
	wget -nv https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2?product=GNU%20ARM%20Embedded%20Toolchain,32-bit,,Linux,5-2016-q3-update -O gcc-arm-none-eabi.tar.bz2
RUN \
	mkdir gcc-arm-none-eabi && \
	tar xjf gcc-arm-none-eabi.tar.bz2 -C gcc-arm-none-eabi --strip-components 1 && \
	rm -rf gcc-arm-none-eabi.tar.bz2
ENV PATH="$PATH:/gcc-arm-none-eabi/bin"

# extend ccache to C++
ENV CCACHE_DIR=/usr/lib64/ccache
ENV CCACHE_BIN=../../bin/ccache
RUN \
	ln -s `which arm-none-eabi-objcopy` ${CCACHE_DIR}/arm-none-eabi-objcopy &&\
	ln -s ${CCACHE_BIN} ${CCACHE_DIR}/arm-none-eabi-g++ && \
	ln -s ${CCACHE_BIN} ${CCACHE_DIR}/arm-none-eabi-c++

# Add default user and enable sudo access
RUN useradd -c "mbed Developer" -m mbed

# configure git
COPY mbed-gitconfig /home/mbed/.gitconfig

USER mbed
WORKDIR /home/mbed
CMD ["bash"]
