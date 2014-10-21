#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -e <experiment> -n <num_policy> -x <suffix>"
}

while getopts ":hd:e:x:n:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      ;;
    n)
      npolicy=${OPTARG}
      ;;
    e)
      experiment=${OPTARG}
      ;;
    x)
      suffix=${OPTARG}
      ;;
    :)
      echo "ERROR: -${OPTARG} requires an argument"
      usage
      exit 1
      ;;
    ?)
      echo "ERROR: unknown option -${OPTARG}"
      usage
      exit 1
      ;;
  esac
done

if ! [ -d results/$data/$experiment ]; then mkdir -p results/$data/$experiment; fi
for c in 0.25 0.5 1 2 4 8; do
   for w in 1 2 4 8; do
      for n in `seq 0 $((policy-1))`; do
         ./scripts/get_stats.sh -d $data -e $experiment -c $c -w $w -n $n -x $suffix 
