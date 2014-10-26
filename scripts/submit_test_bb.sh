#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -c <svm_c> -w <svm_w> -n <policy_id> -e <experiment> -x <suffix> -r <restriced_level> -m <mem> -q <queue> -p <policy_dir>"
}

suffix=".lp.gz"
freq=1
mem=4
queue=batch
policy=""

while getopts ":hd:e:x:m:r:q:c:w:n:p:" arg; do
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
    r)
      freq=${OPTARG}
      ;;
    p)
      policy=${OPTARG}
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

# show info
experiment=$experiment/c${c}w${w}/policy$n
dir=/fs/clip-ml/he/scip-dagger
if [ -z $policy ]; then
   policyDir=policy/${data%/*}/train/${experiment%/*}
else
   policyDir=policy/$policy/c${c}w${w}
fi
scratch=/fs/clip-scratch/hhe/scip-dagger/scratch
resultDir=/fs/clip-scratch/hhe/scip-dagger/result
if ! [ -d $scratch/$data/$experiment ]; then mkdir -p $scratch/$data/$experiment; fi
if ! [ -d $resultDir/$data/$experiment ]; then mkdir -p $resultDir/$data/$experiment; fi

echo "submitting test $experiment for $data (restricted=$freq, suffix=$suffix) using policy from $policyDir"
echo "memory=${mem}g, queue=$queue"

test_jobid=$(echo "cd $dir; \
module add cplex; \
  ./scripts/test_bb.sh -d $data -e $experiment -x $suffix -s $policyDir/searchPolicy.$n -k $policyDir/killPolicy.$n &> $resultDir/$data/$experiment/log;" |
qsub -N test-$data-$experiment -q $queue -l walltime=24:00:00,pmem=${mem}g -o $scratch/$data/$experiment/test.output -e $scratch/$data/$experiment/test.error)

echo "submitting stats $experiment for $data (c=$c, w=$w, n=$n) after ${test_jobid}"
echo "cd $dir; \
./scripts/get_stats.sh -d $data -e ${experiment} -x $suffix" | 
qsub -N stats-$data-$experiment -q $queue -l walltime=24:00:00,pmem=${mem}g -o $scratch/$data/$experiment/stats.output -e $scratch/$data/$experiment/stats.error -W depend=afterok:${test_jobid}
