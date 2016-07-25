FROM meriac/mbed-base
MAINTAINER Milosch Meriac <milosch@meriac.com>

# update mbed-cli to latest version
RUN pip install -U mbed-cli

# install ssh-server
RUN dnf -y install \
	\
	openssh-server \
	passwd \
	\
	&& dnf clean all

# set up entry point
COPY boot.sh /etc/boot.sh

# set up SSH key on first boot
ENTRYPOINT ["/etc/boot.sh"]

# open ssh port
EXPOSE 22

# start SSH daemon by default
CMD ["/usr/sbin/sshd", "-D"]

