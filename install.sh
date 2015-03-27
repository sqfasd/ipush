#!/bin/bash
set -x
readonly PROGNAME=$(basename $0)
readonly PROGDIR=$(readlink -m $(dirname $0))
readonly ARGS=$@
readonly ARGC=$#

if [ $ARGC != 1 ]; then
  echo "usage: $PROGNAME <install_dir>"
  exit 1
fi

install_dir=$1
mkdir -p $install_dir/{conf,scripts,bin,logs} || exit 1
mkdir -p $install_dir/ssdb/var || exit 1

cp build/release/bin/* $install_dir/bin/
cp conf/* $install_dir/conf/
cp scripts/* $install_dir/scripts/ && chmod +x $install_dir/scripts/*
cp deps/ssdb/ssdb-server $install_dir/ssdb/
cp deps/ssdb/ssdb.conf $install_dir/ssdb/
cp deps/ssdb/ssdb_slave.conf $install_dir/ssdb/

version_file=$install_dir/version
rm -f $version_file
echo -e "[Date]" >> $version_file
date >> $version_file
echo -e "\n[Git]" >> $version_file
git branch | grep '^*' >> $version_file
git log | head -n 1 >> $version_file
echo -e "\n[MD5]" >> $version_file
md5sum build/release/bin/* >> $version_file

echo "ok"
