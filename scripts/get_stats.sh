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
experiment=$experiment/c${c}w${w}/policy$n
resultDir=/fs/clip-scratch/hhe/scip-dagger/result
output=$resultDir/$data/$experiment/stats

echo "getting stats of $experiment for $data (suffix=$suffix)"
# print header
printf "%-20s %-5s %-5s %-6s %-6s %-10s %-10s %-5s\n" \
"problem" "nvars" "ncons" "nnodes" "time" "LB" "UB" "gap" > $output

for prob in `ls $datDir`; do
  base=`sed "s/$suffix//g" <<< $prob`
  #echo $base
  # read scip log 
  log=$resultDir/$data/$experiment/$base.log
  if ! [ -e $log ]; then
    echo "missing $base.log"
    continue
  fi
  # second line is variables/constraints in the presolved problem
  nvars=$(grep "Variables" $log | head -n 1 | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  nconstrs=$(grep "Constraints" $log | head -n 1 | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  nnodes=$(grep "Solving Nodes" $log | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  time=$(grep "Solving Time" $log | sed "s/\s\+/ /g" | cut -d' ' -f5) 
  lb=$(grep "^Dual Bound" $log | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  ub=$(grep "^Primal Bound" $log | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  gap=$(grep "^Gap" $log | sed "s/\s\+/ /g" | cut -d' ' -f3) 
  printf "%-20s %-5d %-5d %-6d %-6.2f %-10.2f %-10.2f %-5.2f\n" \
  $base $nvars $nconstrs $nnodes $time $lb $ub $gap >> $output
done

# compute average
tail -n +2 $output | awk '{
nvars += $2; 
nconstrs += $3;
nnodes += $4;
time += $5;
lb += $6;
ub += $7;
gap += $8;
}
END {
printf "%-20s %-5d %-5d %-6d %-6.2f %-10.2f %-10.2f %-5.2f\n", "average", nvars/NR, nconstrs/NR, nnodes/NR, time/NR, lb/NR, ub/NR, gap/NR;
}' >> $output

echo "saved in $output"
