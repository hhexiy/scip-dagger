#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -n <node_limit> -t <time_limit> -r <restriced_level> -x <suffix> -e <experiment>"
}

suffix=".lp.gz"
freq=1
nnode=-1
time=-1

while getopts ":hd:n:t:x:r:e:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      echo "test data: $data"
      ;;
    e)
      experiment=${OPTARG}
      echo "experiment: $experiment"
      ;;
    x)
      suffix=${OPTARG}
      echo "data suffix: $suffix"
      ;;
    r)
      freq=${OPTARG}
      echo "restriced level: $freq"
      ;;
    n)
      nnode=${OPTARG}
      echo "node limit: $nnode"
      ;;
    t)
      time=${OPTARG}
      echo "time limit: $time"
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

resultDir=/fs/clip-scratch/hhe/scip-dagger/result
dir=dat/$data
if ! [ -d $resultDir/$data/$experiment ]; then
  mkdir -p $resultDir/$data/$experiment
fi
for file in `ls $dir`; do
  base=`sed "s/$suffix//g" <<< $file`
  echo $base
  bin/scipdagger -r $freq -s scip.set -f $dir/$file -n $nnode -t $time &> $resultDir/$data/$experiment/$base.log
done
