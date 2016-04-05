# ========================================
# run scip to get solution for training
# ========================================

data=$1
data=${data%/}
suffix=$2
dir=dat/$data
echo "Running scip for $data"

resultDir=/fs/clip-scratch/hhe/scip-dagger/result

for fold in train dev test; do
   if ! [ -d $resultDir/$data/$fold/scip ]; 
      then mkdir -p $resultDir/$data/$fold/scip
   fi
   if ! [ -d solution/$data/$fold ]; then 
     mkdir -p solution/$data/$fold
   fi
done
echo "Writing logs in $resultDir/$data"
echo "Writing solutions in solution/$data"

for fold in train dev test; do
  echo "Solving problems in $dir/$fold ..."
  for file in `ls $dir/$fold`; do
    base=`sed "s/$suffix//g" <<< $file`
    if ! [ -e solution/$data/$fold/$base.sol ]; then
      echo $base
      bin/scipdagger -f $dir/$fold/$file --sol solution/$data/$fold/$base.sol -s scip.set -t 30 > $resultDir/$data/$fold/scip/$base.log
    fi
  done
done
