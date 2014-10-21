#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -x <suffix> -p <num_passes> -n <num_per_iter> -c <svm_c> -w <svm_w> -e <experiment> -m <mem> -r <restriced_level>"
}

# default values
suffix=".lp.gz"
freq=1
mem=4
queue=batch

while getopts ":hd:p:n:c:e:w:x:m:r:q:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      ;;
    p)
      numPasses=${OPTARG}
      ;;
    n)
      numPerIter=${OPTARG}
      ;;
    c)
      c=${OPTARG}
      ;;
    w)
      w=${OPTARG}
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
    r)
      freq=${OPTARG}
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

# show info
experiment=$experiment/c${c}w${w}
echo "submitting experiment $experiment for $data (restricted=$freq, numPasses=$numPasses, numPerIter=$numPerIter, c=$c, w=$w, suffix=$suffix)"
echo "memory=${mem}g, queue=$queue"

dir=/fs/clip-ml/he/scip-dagger
scratch=/fs/clip-scratch/hhe/scip-dagger/scratch

echo "cd $dir; \
module add cplex; \
if ! [ -d $scratch/$data/$experiment ]; then mkdir -p $scratch/$data/$experiment; fi; \
./scripts/train_bb.sh -d $data -r $freq -p $numPasses -n $numPerIter -c $c -w $w -e $experiment -x $suffix &> $scratch/$data/$experiment/training.log;" |
qsub -N bb-$data-$experiment -q $queue -l walltime=24:00:00,pmem=${mem}g -o $scratch/$data/$experiment/training.output -e $scratch/$data/$experiment/training.error
