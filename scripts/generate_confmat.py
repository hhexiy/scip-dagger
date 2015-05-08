import sys, csv
import numpy as np
import matplotlib.pylab as plt
import matplotlib
from sklearn import preprocessing
min_max_scaler = preprocessing.MinMaxScaler()

# avoid type 3 fonts
matplotlib.rcParams['ps.useafm'] = True
matplotlib.rcParams['pdf.use14corefonts'] = True
matplotlib.rcParams['text.usetex'] = True

plt.rcParams['ytick.major.pad']='8'

def read_obj(filename, scip_filename):
  time = 0
  gap = 0
  with open(filename, 'r') as fin:
    for line in fin:
      line = line.strip().split()
      time = float(line[2])
      # no fail
      if len(line) < 9 or float(line[-1]) == 0:
         gap = float(line[6])
      else:
         gap = None
  with open(scip_filename, 'r') as fin:
     for line in fin:
        pass
     scip_time = float(line.strip().split()[2])
  if time >= scip_time:
     time = None
  return time, gap

if __name__ == '__main__':
   time = np.zeros((4,4))  # time
   gap = np.zeros((4,4))  # gap
   idx = {'mik/bounded':0, 'corlat':1, 'regions200':2, 'hybrid100':3}
   for data in ['mik/bounded', 'corlat', 'regions200', 'hybrid100']:
     for policy in ['mik/bounded', 'corlat', 'regions200', 'hybrid100']:
       time[idx[policy]][idx[data]], gap[idx[policy]][idx[data]] = read_obj('results/%s/test/cross/%s/result' % (data, policy), '/fs/clip-scratch/hhe/scip-dagger/result/%s/test/scip/full/stats' % (data))
   # fill failed cases
   max_gap = np.nanmax(gap, axis=0)
   max_time = np.nanmax(time, axis=0)
   fail = set()
   for i in range(4):
      for j in range(4):
         if np.isnan(gap[i][j]):
            gap[i][j] = max_gap[j]
            fail.add((i, j))
         if np.isnan(time[i][j]):
            time[i][j] = max_time[j]
            fail.add((i, j))
   time = min_max_scaler.fit_transform(time)
   gap = min_max_scaler.fit_transform(gap)
   time_gap = time + gap
   print 'time'
   print time
   print 'gap'
   print gap
   print 'time+gap'
   print time_gap

   for policy in range(4):
     for data in range(4):
       if (policy, data) in fail:
         time_gap[policy][data] = 0
       elif policy != data:
         time_gap[policy][data] = 1.0 / (time_gap[policy][data] / time_gap[data][data])
   for i in range(4):
      time_gap[i][i] = 1.0
   max_value = np.max(np.amax(time_gap, axis=0))
   min_value = np.min(np.amin(time_gap, axis=0))
   # add scip (default)
   print time_gap

   fig = plt.figure()
   ax = fig.add_subplot(1, 1, 1)

   transpose = True
   if not transpose:
      img = ax.imshow(np.array(time_gap), vmin=min_value, vmax=max_value, cmap=plt.cm.jet, interpolation='nearest')
      data = ['MIK', 'CORLAT', 'Regions', 'Hybrid']
      ax.xaxis.tick_top()
      ax.set_xticks(range(4))
      ax.set_xticklabels(data, rotation=10, size=20)
      ax.set_yticks(range(4))
      ax.set_yticklabels(data, size=20)
      ax.set_xlabel("Test Dataset", size=20, labelpad=8)
      ax.set_ylabel("Policy Dataset", size=20, labelpad=8)

      cbar = plt.colorbar(img)
      cbar.ax.set_ylabel("1 / (time + opt. gap)", size=20, rotation=270, labelpad=8)
      fig.tight_layout(w_pad=5)
      plt.savefig("results/conf_mat.pdf")
   else:
      img = ax.imshow(np.transpose(np.array(time_gap)), vmin=min_value, vmax=max_value, cmap=plt.cm.jet, interpolation='nearest')
      data = ['MIK', 'CORLAT', 'Regions', 'Hybrid']
      ax.yaxis.tick_top()
      ax.set_yticks(range(4))
      ax.set_yticklabels(data, rotation=10, size=20)
      ax.set_xticks(range(5))
      ax.set_xticklabels(data+['SCIP'], size=20)
      ax.set_ylabel("Test Dataset", size=20, labelpad=8)
      ax.set_xlabel("Policy Dataset", size=20, labelpad=8)

      cbar = plt.colorbar(img)
      cbar.ax.set_xlabel("1 / (time + opt. gap)", size=20, rotation=270, labelpad=8)
      fig.tight_layout(w_pad=5)
      plt.savefig("results/conf_mat.pdf")

