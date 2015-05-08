#!/bin/bash
set -e

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -s <search_policy> -k <kill_policy> -e <experiment> -x <suffix> -m <problem> -r <restriced_level> -g <dagger>"
}

suffix=".lp.gz"
freq=1
dagger=0

while getopts ":hd:s:k:e:x:m:r:g:" arg; do
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
    r)
      freq=${OPTARG}
      echo "restriced level: $freq"
      ;;
    g)
      dagger=${OPTARG}
      echo "run dagger: $dagger"
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
  if [[ $dagger -eq 0 ]]; then
    bin/scipdagger -r $freq -s scip.set -f $dir/$file --nodesel policy $searchPolicy --nodepru policy $killPolicy &> $resultDir/$data/$experiment/$base.log
  else
    sol=solution/$data/$base.sol
    bin/scipdagger -r $freq -s scip.set -f $dir/$file -o $sol --nodesel dagger $searchPolicy --nodepru dagger $killPolicy &> $resultDir/$data/$experiment/$base.log
  fi
done

