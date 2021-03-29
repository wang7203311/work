#!/bin/bash

trap 'kill %1; kill %2 ; kill %3 ; kill %4 ; kill %5 ; kill %6 ; kill %7 ; kill %8 ; kill %9; kill %10' SIGINT
./node.go 4000 3000 & ./node.go 4001 3000 & ./node.go 4002 3000 & ./node.go 4003 3000 & ./node.go 4004 3000 & ./node.go 4005 3000 & ./node.go 4006 3000 & ./node.go 4007 3000 & ./node.go 4008 3000 & ./node.go 4009 3000 & ./node.go 4010 3000 & ./node.go 4011 3000
