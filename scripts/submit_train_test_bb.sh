#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -x <suffix> -p <num_passes> -n <num_per_iter> -N <num_policy> -c <svm_c> -w <svm_w> -e <experiment> -m <mem> -r <restriced_level>"
}

# default values
suffix=".lp.gz"
freq=1
mem=4
queue=batch

while getopts ":hd:p:n:c:e:w:x:m:r:q:N:" arg; do
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
    N)
      npolicy=${OPTARG}
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

# training
train_jobid=$(echo "cd $dir; \
module add cplex; \
if ! [ -d $scratch/$data/$experiment ]; then mkdir -p $scratch/$data/$experiment; fi; \
./scripts/train_bb.sh -d $data -r $freq -p $numPasses -n $numPerIter -c $c -w $w -e $experiment -x $suffix &> $scratch/$data/$experiment/training.log;" |
qsub -N bb-$data-$experiment -q $queue -l walltime=24:00:00,pmem=${mem}g -o $scratch/$data/$experiment/training.output -e $scratch/$data/$experiment/training.error)

# test
resultDir=/fs/clip-scratch/hhe/scip-dagger/result
experiment=$experiment/policy
data=${data%/*}/dev
for n in $(seq 0 $((npolicy-1))); do
   experiment=${experiment%/*}/policy$n
   policyDir=policy/${data%/*}/train/${experiment%/*}

   echo "submitting test $experiment for $data (restricted=$freq, suffix=$suffix) after ${train_jobid}"
   test_jobid=$(echo "cd $dir; \
   module add cplex; \
   if ! [ -d $scratch/$data/$experiment ]; then mkdir -p $scratch/$data/$experiment; fi; \
   if ! [ -d $resultDir/$data/$experiment ]; then mkdir -p $resultDir/$data/$experiment; fi; \
     ./scripts/test_bb.sh -d $data -e $experiment -x $suffix -s $policyDir/searchPolicy.$n -k $policyDir/killPolicy.$n &> $resultDir/$data/$experiment/log;" |
   qsub -N bb-$data-$experiment -q $queue -l walltime=24:00:00,pmem=${mem}g -o $scratch/$data/$experiment/test.output -e $scratch/$data/$experiment/test.error -W depend=afterok:${train_jobid})

   echo "submitting stats $experiment for $data (c=$c, w=$w, n=$n) after ${test_jobid}"
   echo "cd $dir; \
   ./scripts/get_stats.sh -d $data -e ${experiment%%/*} -x $suffix -c $c -w $w -n $n" | 
   qsub -N bb-$data-$experiment-stats -q $queue -l walltime=24:00:00,pmem=${mem}g -o $scratch/$data/$experiment/stats.output -e $scratch/$data/$experiment/stats.error -W depend=afterok:${test_jobid}
done
   
