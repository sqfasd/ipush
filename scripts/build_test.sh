#!/bin/bash

path=tools
project=push_client
dest_dir=build/release/bin/
go build $path/$project/push_client.go
mv push_client $dest_dir
project=push_test
go build $path/$project/push.go
mv push $dest_dir/push_test
