#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -s <search_policy> -k <kill_policy> -e <experiment> -x <suffix> -m <problem>"
}

suffix=".lp.gz"

while getopts ":hd:s:k:e:x:m:" arg; do
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
    m)
      problem=${OPTARG}
      echo "problem: $problem"
      ;;
    x)
      suffix=${OPTARG}
      echo "data suffix: $suffix"
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
if ! [ -d $resultDir/$data/$experiment/solution ]; then
  mkdir -p $resultDir/$data/$experiment/solution
fi
for file in `ls $dir`; do
  base=`sed "s/$suffix//g" <<< $file`
  echo $base
  sol=solution/$data/$base.sol
  bin/scipdagger -f $dir/$file -o $sol --nodesel policy $searchPolicy --nodepru policy $killPolicy &> $resultDir/$data/$experiment/$base.log
done
