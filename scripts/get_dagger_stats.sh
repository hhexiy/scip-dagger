#!/bin/bash

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -e <experiment> -c <svm_c> -w <svm_w> -n <policy_id> -x <suffix>"
}

while getopts ":hd:e:x:c:w:n:" arg; do
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


datDir=dat/$data
if ! [ -z $c ]; then
   experiment=$experiment/c${c}w${w}/policy$n
fi
resultDir=/fs/clip-scratch/hhe/scip-dagger/result
output=$resultDir/$data/$experiment/stats

echo "getting dagger stats of $experiment for $data (suffix=$suffix)"

for prob in `ls $datDir`; do
  base=`sed "s/$suffix//g" <<< $prob`
  log=$resultDir/$data/$experiment/$base.log
  ncomperr=$(grep "comp error rate" $log | sed "s/[^0-9\/]//g" | cut -d'/' -f1) 
  ncomp=$(grep "comp error rate" $log | sed "s/[^0-9\/]//g" | cut -d'/' -f2) 
  seltime=$(grep "selection time" $log | sed "s/ //g" | cut -d':' -f2) 
  nprune=$(grep "nodes pruned" $log | sed "s/[^0-9\/]//g" | cut -d'/' -f1) 
  nnodes=$(grep "nodes pruned" $log | sed "s/[^0-9\/]//g" | cut -d'/' -f2) 
  fpprune=$(grep "FP pruned" $log | sed "s/[^0-9\/]//g" | cut -d'/' -f1) 
  fnprune=$(grep "FN pruned" $log | sed "s/[^0-9\/]//g" | cut -d'/' -f1) 
  prutime=$(grep "pruning time" $log | sed "s/ //g" | cut -d':' -f2) 
  echo $seltime $prutime $ncomperr $ncomp $nprune $fpprune $fnprune $nnodes 
done | awk '{
   seltime += $1
   prutime += $2
   ncomperr += $3
   ncomp += $4
   nprune += $5
   fpprune += $6
   fnprune += $7
   nnodes += $8
}
END {
   printf "select time: %f\n", seltime/NR;
   printf "pruning time: %f\n", prutime/NR;
   printf "comp err: %f\n", ncomperr / ncomp;
   printf "pruning rate: %f\n", nprune / nnodes;
   printf "FP pruning: %f\n", fpprune / nnodes;
   printf "FN pruning: %f\n", fnprune / nnodes;
}'

