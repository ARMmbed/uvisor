FROM fedora
MAINTAINER Milosch Meriac <milosch@meriac.com>

# update packages
# RUN dnf -y update && dnf clean all

# remove competing vim package
RUN dnf -y remove vim-minimal

# install packages
RUN dnf -y install \
	\
	mc \
	vim \
	python-pip \
	git \
	mercurial \
	ccache \
	arm-none-eabi-gcc* \
	arm-none-eabi-newlib \
	arm-none-eabi-binutils-cs \
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

CMD ["/usr/bin/su","-l","mbed"]
