data=$1
e1=$2
e2=$3
scratch=/fs/clip-scratch/hhe/scip-dagger/result/$data
stats1=$scratch/$e1/stats
stats2=$scratch/$e2/stats
cat $stats1 | tail -n +2 | head -n -1 | sed 's/\s\+/ /g' | cut -d' ' -f7 > ogap1
cat $stats1 | tail -n +2 | head -n -1 | sed 's/\s\+/ /g' | cut -d' ' -f8 > igap1
cat $stats2 | tail -n +2 | head -n -1 | sed 's/\s\+/ /g' | cut -d' ' -f7 > ogap2
cat $stats2 | tail -n +2 | head -n -1 | sed 's/\s\+/ /g' | cut -d' ' -f8 > igap2
paste -d' ' ogap1 ogap2 > ogap
paste -d' ' igap1 igap2 > igap
echo "ogap"
python scripts/t_test.py ogap
echo "igap"
python scripts/t_test.py igap
