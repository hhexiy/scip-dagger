dat=$1
suffix=$2
result=/fs/clip-scratch/hhe/scip-dagger/result
for fold in test dev train; do
   stats=$result/$dat/$fold/scip/full/stats
   if ! [ -e $stats ]; then
      ./scripts/get_stats.sh -d $dat/$fold -e scip/full -x $suffix
   fi
   count=0
   for prob in $(sed 's/\s\+/ /g' $stats | awk '{ if ($2 < 10 && $1 != "problem") {print $1} }'); do
      rm dat/$dat/$fold/$prob$suffix
      rm $result/$dat/$fold/scip/full/$prob.log
      rm solution/$dat/$fold/$prob.sol
      count=$((count+1))
   done
   echo "filtered $count problem in $dat/$fold"
done
