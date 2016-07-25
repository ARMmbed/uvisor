#!/bin/bash

SSH_USER=mbed
SSH_PASS=`dd if=/dev/urandom bs=15 count=1 status=none | base64`
# show password
echo -e "\nssh password for user $SSH_USER:'$SSH_PASS'"

# create run-directory
mkdir -p /var/run/sshd

# generate password for default user
echo -e "$SSH_PASS\n$SSH_PASS" | passwd --stdin $SSH_USER

# generate RSA host key
ssh-keygen -b 4096 -t rsa -f /etc/ssh/ssh_host_rsa_key -N ''

exec "$@"
