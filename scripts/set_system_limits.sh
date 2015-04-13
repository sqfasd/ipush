#!/bin/bash

SYSCTL_CONF=/etc/sysctl.conf
LIMITS_CONF=/etc/security/limits.conf

cp $SYSCTL_CONF ${SYSCTL_CONF}.bak
cat >> /etc/sysctl.conf <<EOF
net.core.somaxconn = 60000
net.core.rmem_default = 4096
net.core.wmem_default = 4096
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.core.netdev_max_backlog = 60000
net.ipv4.tcp_rmem = 4096 4096 16777216
net.ipv4.tcp_wmem = 4096 4096 16777216
net.ipv4.tcp_mem = 786432 2097152 3145728
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_max_syn_backlog = 60000
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_tw_recycle = 1
net.ipv4.tcp_max_orphans = 131072
net.ipv4.tcp_keepalive_time = 1200
net.ipv4.tcp_max_tw_buckets = 60000
net.netfilter.nf_conntrack_max = 1000000
net.netfilter.nf_conntrack_tcp_timeout_established = 1200
fs.file-max = 2000000
fs.nr_open = 1900000
EOF

cp $LIMITS_CONF ${LIMITS_CONF}.bak
cat >> /etc/security/limits.conf <<EOF
root    soft    nofile  1700000
root    hard    nofile  1800000
EOF
