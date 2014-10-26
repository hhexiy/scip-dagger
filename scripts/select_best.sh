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
result=results/$data/$experiment/results.dev
scratch=/fs/clip-scratch/hhe/scip-dagger/result/$data/$experiment
if [ -e $result ]; then rm $result; fi
#for c in 0.0625 0.125 0.25 0.5 1 2 4 8; do
for c in 0.25 0.5 1 2 4 8; do
   for w in 1 2 4 8; do
      #if [[ $c == "2" && $w == "8" ]]; then continue; fi
      for n in `seq 0 $((npolicy-1))`; do
         echo $c $w $n
         #stats=$(./scripts/get_stats.sh -d $data -e $experiment -c $c -w $w -n $n -x $suffix | tail -n 1 | cut -d' ' -f3) 
         stats=$scratch/c${c}w${w}/policy$n/stats
         avgstats=$(cat $stats | tail -n 1 | sed 's/\s\+/ /g')
         nnodes=$(cut -d' ' -f2 <<< $avgstats)
         opt=$(cut -d' ' -f6 <<< $avgstats)
         ogap=$(cut -d' ' -f7 <<< $avgstats)
         igap=$(cut -d' ' -f8 <<< $avgstats)
         fail=$(cut -d' ' -f9 <<< $avgstats)
         echo $c $w $n $nnodes $opt $ogap $igap $fail >> $result
      done
   done
done

# sort
sort -k8 -k4 -n $result -o $result
cat $result | head -n 10

