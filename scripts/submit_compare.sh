#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -e <experiment> -c <svm_c> -w <svm_w> -n <policy_id> -x <suffix> -m <mem> -q <queue>"
}

suffix=".lp.gz"
mem=4
queue=batch

while getopts ":hd:e:x:c:w:n:m:q:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      ;;
    c)
      c=${OPTARG}
      ;;
    w)
      w=${OPTARG}
      ;;
    n)
      n=${OPTARG}
      ;;
    e)
      experiment=${OPTARG}
      ;;
    x)
      suffix=${OPTARG}
      ;;
    m)
      mem=${OPTARG}
      ;;
    q)
      queue=${OPTARG}
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

dir=/fs/clip-ml/he/scip-dagger
scratch=/fs/clip-scratch/hhe/scip-dagger/scratch
output=$scratch/$data/$experiment/c${c}w${w}/policy$n
if ! [ -d $output ]; then mkdir -p $output; fi
echo "cd $dir; \
module add cplex; \
if ! [ -d $scratch/$data/$experiment ]; then mkdir -p $scratch/$data/$experiment; fi; \
  ./scripts/compare.sh -d $data -e $experiment -x $suffix -c $c -w $w -n $n;" | 
qsub -N compare-$data-$experiment -q $queue -l walltime=24:00:00,pmem=${mem}g -o $output/compare.output -e $output/compare.error
