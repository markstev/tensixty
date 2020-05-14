#!/usr/bin/bash

set -e

cd cc
python3 /Users/mark/nanopb/generator/nanopb_generator.py motor_command.proto
protoc --proto_path=../cc --python_out=../python motor_command.proto --proto_path=/Users/mark/nanopb/generator/proto
