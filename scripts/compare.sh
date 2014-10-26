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

resultDir=/fs/clip-ml/he/scip-dagger/results/$data/$experiment/c${c}w${w}/policy$n
scratch=/fs/clip-scratch/hhe/scip-dagger/result
if ! [ -d $resultDir ]; then mkdir -p $resultDir; fi
result=$resultDir/test.result
# print header
printf "%-20s %-6s %-6s %-10s %-10s %-10s %-5s %-5s\n" \
"problem" "nnodes" "time" "DB" "PB" "opt" "ogap" "igap" > $result

# run policy
echo "running policy $experiment c=$c w=$w n=$n for $data"
echo "policy" >> $result
policyDir=policy/${data%/*}/train/${experiment}/c${c}w${w}
./scripts/test_bb.sh -d $data -e $experiment/c${c}w${w}/policy$n -x $suffix -s $policyDir/searchPolicy.$n -k $policyDir/killPolicy.$n > /dev/null
echo "getting stats for $data with $experiment c=$c w=$w n=$n"
stats=$(./scripts/get_stats.sh -d $data -e $experiment -c $c -w $w -n $n -x $suffix | tail -n 1 | cut -d' ' -f3)
tail -n 1 $stats >> $result
time=$(tail -n 1 $stats | sed 's/\s\+/ /g' | cut -d' ' -f3)
nnodes=$(tail -n 1 $stats | sed 's/\s\+/ /g' | cut -d' ' -f2)

# full scip
#echo "running scip with full feature"
#echo "scip full" >> $result
#./scripts/test_scip.sh -d $data -x $suffix -e scip/full > /dev/null
#echo "getting stats for $data with scip/full"
#stats=$(./scripts/get_stats.sh -d $data -e scip/full -x $suffix | tail -n 1 | cut -d' ' -f3)
tail -n 1 $scratch/$data/scip/full/stats >> $result 

# test scip
echo "test scip with time limit $time"
echo "scip time limit" >> $result
./scripts/test_scip.sh -d $data -x $suffix -e scip/time-limit/c${c}w${w}/policy$n -t $time > /dev/null
echo "getting stats for $data with scip/time-limit"
stats=$(./scripts/get_stats.sh -d $data -e scip/time-limit/c${c}w${w}/policy$n -x $suffix | tail -n 1 | cut -d' ' -f3)
tail -n 1 $stats >> $result

# test gurobi
echo "test gurobi with node limit $nnodes"
echo "gurobi node limit" >> $result
./scripts/test_gurobi.sh -d $data -n $nnodes -e gurobi/node-limit/c${c}w${w}/policy$n -x $suffix > /dev/null
echo "getting stats for $data with gurobi/node-limit"
stats=$(./scripts/get_stats_gurobi.sh -d $data -e gurobi/node-limit/c${c}w${w}/policy$n -x $suffix | tail -n 1 | cut -d' ' -f3)
tail -n 1 $stats >> $result
