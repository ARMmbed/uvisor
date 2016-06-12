FROM fedora
MAINTAINER Milosch Meriac <milosch@meriac.com>

# update packages
# RUN dnf -y update && dnf clean all

# remove competing vim package
RUN dnf -y remove vim-minimal

# install packages
RUN dnf -y install \
	\
	sudo \
	mc \
	vim \
	python-pip \
	git \
	mercurial \
	ccache \
	arm-none-eabi-gcc* \
	arm-none-eabi-newlib \
	arm-none-eabi-binutils-cs \
	openssh-server \
	passwd \
	tar \
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
	ln -s ${CCACHE_BIN} ${CCACHE_DIR}/arm-none-eabi-g++ && \
	ln -s ${CCACHE_BIN} ${CCACHE_DIR}/arm-none-eabi-c++

# Add default user and enable sudo access
RUN useradd -c "mbed Developer" -m mbed
RUN usermod -aG wheel mbed
# enabled sudo for mbed user - no password asked!
COPY sudoers /etc

# configure git
COPY .gitconfig /home/mbed/

CMD ["/usr/bin/su","-l","mbed"]
