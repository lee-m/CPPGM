#!/bin/bash

(./$1 < $2 | tee $3) &> $3.stderr
 
