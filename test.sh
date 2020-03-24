#!/bin/bash

for i in {1..8}
do
    perf stat ./integration $i
done

