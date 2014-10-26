import sys
import numpy as np
from scipy import stats
file = sys.argv[1]

a = []
with open(file, 'r') as fin:
  for line in fin:
    a.append([float(x) for x in line.strip().split(' ')])
a = np.array(a)
print a.shape
t, prob = stats.ttest_rel(a[:,0], a[:,1])
print np.mean(a[:,0]), np.mean(a[:,1])
print t, prob
