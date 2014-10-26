#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -s <search_policy> -k <kill_policy> -e <experiment> -x <suffix> -r <restriced_level>"
}

suffix=".lp.gz"
freq=1
time=-1

while getopts ":hd:s:k:e:x:r:t:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      echo "test data: $data"
      ;;
    s)
      searchPolicy=${OPTARG}
      echo "search policy: $searchPolicy"
      ;;
    k)
      killPolicy=${OPTARG}
      echo "kill policy: $killPolicy"
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
  if [ -z $searchPolicy ]; then
   bin/scipdagger -r $freq -s scip.set -t $time -f $dir/$file --nodepru policy $killPolicy &> $resultDir/$data/$experiment/$base.log
  elif [ -z $killPolicy ]; then
   bin/scipdagger -r $freq -s scip.set -t $time -f $dir/$file --nodesel policy $searchPolicy &> $resultDir/$data/$experiment/$base.log
  else
   bin/scipdagger -r $freq -s scip.set -t $time -f $dir/$file --nodesel policy $searchPolicy --nodepru policy $killPolicy &> $resultDir/$data/$experiment/$base.log
  fi
done
