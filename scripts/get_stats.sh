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

echo "getting stats of $experiment for $data (suffix=$suffix)"
# print header
printf "%-20s %-6s %-6s %-10s %-10s %-10s %-5s %-5s\n" \
"problem" "nnodes" "time" "DB" "PB" "opt" "ogap" "igap" > $output

fail=0
for prob in `ls $datDir`; do
  base=`sed "s/$suffix//g" <<< $prob`
  echo $base
  # read scip log 
  log=$resultDir/$data/$experiment/$base.log
  inf=$(grep "infeasible" $log | wc -l)
  if ! [[ "$inf" -eq "0" ]]; then
     fail=$((fail+1))
     continue
  fi
  sol=solution/$data/$base.sol
  if ! [ -e $log ]; then
    echo "missing $base.log"
    continue
  fi
  # second line is variables/constraints in the presolved problem
  #nvars=$(grep "Variables" $log | head -n 1 | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  #nconstrs=$(grep "Constraints" $log | head -n 1 | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  nnodes=$(grep "Solving Nodes" $log | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  time=$(grep "Solving Time" $log | sed "s/\s\+/ /g" | cut -d' ' -f5) 
  if [[ $nnodes -eq 0 ]]; then
   db=$(grep "  Dual Bound" $log | sed "s/\s\+/ /g" | cut -d' ' -f5) 
   pb=$(grep "  Primal Bound" $log | sed "s/\s\+/ /g" | cut -d' ' -f5) 
  else
   db=$(grep "^Dual Bound" $log | sed "s/\s\+/ /g" | cut -d' ' -f4) 
   pb=$(grep "^Primal Bound" $log | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  fi
  igap=$(grep "^Gap" $log | sed "s/\s\+/ /g" | cut -d' ' -f3) 
  if [ $igap == "infinite" ]; then
     fail=$((fail+1))
     continue
  fi
  opt=$(head -n 1 $sol | sed "s/\s\+/ /g" | cut -d' ' -f3)
  ogap=$(echo "$pb $opt" | awk 'function abs(x){return ((x < 0.0) ? -x : x)} {print abs($1-$2)}')
  printf "%-20s %-6d %-6.2f %-10.2f %-10.2f %-10.2f %-5.2f %-5.2f\n" \
  $base $nnodes $time $db $pb $opt $ogap $igap >> $output
done

# compute average
tail -n +2 $output | awk -v fail=$fail '{
nnodes += $2;
time += $3;
db += $4;
pb += $5;
opt += $6
ogap += $7;
igap += $8;
}
END {
printf "%-20s %-6d %-6.2f %-10.2f %-10.2f %-10.2f %-5.2f %-5.2f %-4d\n", "average", nnodes/NR, time/NR, db/NR, pb/NR, opt/NR, ogap/NR, igap/NR, fail;
}' >> $output

echo "saved in $output"
