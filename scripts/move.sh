for file in $(ls dat/hybrid100/train | head -n 3); do 
   base=$(sed 's/.lp//g' <<< $file)
   mv dat/hybrid100/train/$base.lp dat/hybrid100/dev/$base.lp
   mv solution/hybrid100/train/$base.sol solution/hybrid100/dev/$base.sol 
   mv /fs/clip-scratch/hhe/scip-dagger/result/hybrid100/train/scip/full/$base.log /fs/clip-scratch/hhe/scip-dagger/result/hybrid100/dev/scip/full/$base.log
done
