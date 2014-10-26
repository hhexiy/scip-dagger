python scripts/rank_feature.py --model policy/mik/bounded/train/all/c1w8/searchPolicy.2 --type search > results/feat/mik/search.feat 
python scripts/rank_feature.py --model policy/mik/bounded/train/all/c1w8/killPolicy.2 --type prune > results/feat/mik/prune.feat 

python scripts/rank_feature.py --model policy/corlat/train/all/c4w4/searchPolicy.0 --type search > results/feat/corlat/search.feat 
python scripts/rank_feature.py --model policy/corlat/train/all/c4w4/killPolicy.0 --type prune > results/feat/corlat/prune.feat 

python scripts/rank_feature.py --model policy/regions200/train/all/c4w8/searchPolicy.9 --type search > results/feat/regions200/search.feat 
python scripts/rank_feature.py --model policy/regions200/train/all/c4w8/killPolicy.9 --type prune > results/feat/regions200/prune.feat 

python scripts/rank_feature.py --model policy/hybrid100/train/all/c2w8/searchPolicy.3 --type search > results/feat/hybrid100/search.feat 
python scripts/rank_feature.py --model policy/hybrid100/train/all/c2w8/killPolicy.3 --type prune > results/feat/hybrid100/prune.feat 
