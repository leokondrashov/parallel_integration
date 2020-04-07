#!/bin/bash

for i in {1..8}
do
    echo "./integration $i"
    time ./integration $i > /dev/null
done

