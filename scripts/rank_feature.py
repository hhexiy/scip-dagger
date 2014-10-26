import argparse
from collections import defaultdict

FEAT_NODESEL_SIZE = 18
FEAT_NODEPRU_SIZE = 16

feat_id_map = {
   'search': {
      0: 'SCIP_FEATNODESEL_LOWERBOUND',
      1: 'SCIP_FEATNODESEL_ESTIMATE',
      2: 'SCIP_FEATNODESEL_TYPE_SIBLING',
      3: 'SCIP_FEATNODESEL_TYPE_CHILD',
      4: 'SCIP_FEATNODESEL_TYPE_LEAF',
      5: 'SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF',
      6: 'SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF',
      7: 'SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP',
      8: 'SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN',
      9: 'SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST',
      10: 'SCIP_FEATNODESEL_BRANCHVAR_INF',
      11: 'SCIP_FEATNODESEL_RELATIVEBOUND',
      12: 'SCIP_FEATNODESEL_GLOBALUPPERBOUND',
      13: 'SCIP_FEATNODESEL_GAP',
      14: 'SCIP_FEATNODESEL_GAPINF',
      15: 'SCIP_FEATNODESEL_GLOBALUPPERBOUNDINF',
      16: 'SCIP_FEATNODESEL_PLUNGEDEPTH',
      17: 'SCIP_FEATNODESEL_RELATIVEDEPTH'
   },
   'prune': {
      0: 'SCIP_FEATNODEPRU_GLOBALLOWERBOUND',
      1: 'SCIP_FEATNODEPRU_GLOBALUPPERBOUND',
      2: 'SCIP_FEATNODEPRU_GAP',
      3: 'SCIP_FEATNODEPRU_NSOLUTION',
      4: 'SCIP_FEATNODEPRU_PLUNGEDEPTH',
      5: 'SCIP_FEATNODEPRU_RELATIVEDEPTH',
      6: 'SCIP_FEATNODEPRU_RELATIVEBOUND',
      7: 'SCIP_FEATNODEPRU_RELATIVEESTIMATE',
      8: 'SCIP_FEATNODEPRU_GAPINF',
      9: 'SCIP_FEATNODEPRU_GLOBALUPPERBOUNDINF',
      10: 'SCIP_FEATNODEPRU_BRANCHVAR_BOUNDLPDIFF',
      11: 'SCIP_FEATNODEPRU_BRANCHVAR_ROOTLPDIFF',
      12: 'SCIP_FEATNODEPRU_BRANCHVAR_PRIO_UP',
      13: 'SCIP_FEATNODEPRU_BRANCHVAR_PRIO_DOWN',
      14: 'SCIP_FEATNODEPRU_BRANCHVAR_PSEUDOCOST',
      15: 'SCIP_FEATNODEPRU_BRANCHVAR_INF'
   }
}

def read_model(filename):
   weights = []
   with open(filename, 'r') as fin:
      count = 0
      for line in fin:
         if count < 6:
            count += 1
            continue
         weights.append(float(line.strip()))
   return weights

def get_feat_id(id, feat_type):
   if feat_type == 'search':
      size = FEAT_NODESEL_SIZE
   elif feat_type == 'prune':
      size = FEAT_NODEPRU_SIZE
   depth = id / (size * 2)
   direction = (id % (size * 2)) / size
   feat_id = (id % (size * 2)) % size
   return depth, direction, feat_id


if __name__ == '__main__':
   parser = argparse.ArgumentParser()
   parser.add_argument('--model', dest='model', action='store', type=str, help='file name of the model')
   parser.add_argument('--type', dest='feat_type', action='store', type=str, help='search policy or pruning policy')
   parser.add_argument('--size', dest='size', action='store', type=int, help='feature size')
   parser.add_argument('--topk', dest='topk', action='store', type=int, help='show top k features', default=5)
   parser.add_argument('--directon', dest='direction', action='store_true', help='differentiate direction', default=False)
   args = parser.parse_args()

   weights = read_model(args.model)
   feat_type = args.feat_type
   if feat_type != 'prune' and feat_type != 'search':
      print 'invalid feature type'
      raise Exception
   feat_dict = defaultdict(lambda: defaultdict(lambda: defaultdict(float)))
   for i, w in enumerate(weights):
      depth, direction, feat_id = get_feat_id(i, feat_type)
      if args.direction:
         feat_dict[depth][direction][feat_id] = w
      else:
         feat_dict[depth][0][feat_id] += w

   # sort
   for depth, double_feats in feat_dict.items():
      for direction, feats in double_feats.items():
         sorted_feats = sorted(feats.items(), key=lambda x: abs(x[1]), reverse=True)
         if args.direction:
            print 'level:', depth, 'direction:', 'up' if direction == 0 else 'down'
         else:
            print 'level:', depth
         for i in range(args.topk):
            feat_id, weight = sorted_feats[i]
            print '%d %s %f %f' % (i, feat_id_map[feat_type][feat_id], abs(weight), weight)
