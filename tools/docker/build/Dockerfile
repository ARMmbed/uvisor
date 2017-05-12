FROM meriac/mbed-base
MAINTAINER Milosch Meriac <milosch@meriac.com>

USER root

# install sudo for mbed user
RUN dnf -y install sudo && dnf clean all
RUN usermod -aG wheel mbed
# enable sudo for mbed user - no password asked!
COPY sudoers /etc

# update mbed-cli to latest version
RUN pip install -U mbed-cli

USER mbed
WORKDIR /home/mbed
CMD ["bash"]
