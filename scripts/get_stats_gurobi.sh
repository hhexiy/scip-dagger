#!/bin/bash
set -e;
usage() {
  echo "Usage: $0 -d <data_path_under_dat> -e <experiment> -x <suffix> -i <inverse>"
}
inverse=false
while getopts ":hd:e:x:c:w:n:i" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      ;;
    e)
      experiment=${OPTARG}
      ;;
    x)
      suffix=${OPTARG}
      ;;
    i)
      inverse=true
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
resultDir=/fs/clip-scratch/hhe/scip-dagger/result
output=$resultDir/$data/$experiment/stats

echo "getting stats of $experiment for $data (suffix=$suffix)"
# print header
printf "%-20s %-6s %-6s %-10s %-10s %-10s %-5s %-5s\n" \
"problem" "nnodes" "time" "DB" "PB" "opt" "ogap" "igap" > $output

fail=0
for prob in `ls $datDir`; do
  base=`sed "s/$suffix//g" <<< $prob`
  # read scip log 
  log=$resultDir/$data/$experiment/$base.log
  sol=solution/$data/$base.sol
  if ! [ -e $log ]; then
    echo "missing $base.log"
    continue
  fi
  # second line is variables/constraints in the presolved problem
  #nvars=$(grep "Variables" $log | head -n 1 | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  #nconstrs=$(grep "Constraints" $log | head -n 1 | sed "s/\s\+/ /g" | cut -d' ' -f4) 
  nnodes=$(grep "Explored" $log | cut -d' ' -f2) 
  #time=$(grep "Solving time" $log | cut -d' ' -f3) 
  time=$(grep "Explored" $log | cut -d' ' -f8) 
  db=$(grep "^Best objective" $log | cut -d' ' -f6) 
  db=${db%,}
  pb=$(grep "^Best objective" $log | cut -d' ' -f3) 
  pb=${pb%,}
  opt=$(head -n 1 $sol | sed "s/\s\+/ /g" | cut -d' ' -f5)
  igap=$(grep "^Best objective" $log | cut -d' ' -f8) 
  igap=${igap%\%}
  if [ $pb == "-" ]; then
   fail=$((fail+1))
  else
   # because scip convert to min, negate gurobi's result
   if [ $inverse == true ]; then
      db=$(echo $db | awk '{if ($1 < 0) print -1*$1; else print $1}')
      pb=$(echo $pb | awk '{if ($1 < 0) print -1*$1; else print $1}')
   fi
   ogap=$(echo "$pb $opt" | awk 'function abs(x){return ((x < 0.0) ? -x : x)} {print abs($1-$2)}')
   #echo $base $nnodes $time $db $pb $opt $ogap $igap
   printf "%-20s %-6d %-6.2f %-10.2f %-10.2f %-10.2f %-5.2f %-5.2f\n" \
   $base $nnodes $time $db $pb $opt $ogap $igap >> $output
  fi
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
