import sys, csv
import numpy as np
import matplotlib.pylab as plt
from sklearn import preprocessing

min_max_scaler = preprocessing.MinMaxScaler()

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
   print time_gap

   fig = plt.figure()
   ax = fig.add_subplot(1, 1, 1)
   img = ax.imshow(np.array(time_gap), vmin=min_value, vmax=max_value, cmap=plt.cm.jet, interpolation='nearest')
   data = ['MIK', 'CORLAT', 'Regions', 'Hybrid']
   ax.xaxis.tick_top()
   ax.set_xticks(range(4))
   ax.set_xticklabels(data, rotation=20, size=20)
   ax.set_yticks(range(4))
   ax.set_yticklabels(data, size=20)
   ax.set_xlabel("Test Dataset", size=20, labelpad=8)
   ax.set_ylabel("Policy Dataset", size=20, labelpad=8)

   #fig.subplots_adjust(right=0.8)
   #cbar_ax = fig.add_axes([0.85, 0.1, 0.04, 0.8])
   cbar = plt.colorbar(img)
   cbar.ax.set_ylabel("1 / (time + opt. gap)", size=20, rotation=270, labelpad=8)
   fig.tight_layout(w_pad=5)
   plt.savefig("results/conf_mat.pdf")

  # fig, (ax1, ax2) = plt.subplots(1, 2)
  #
  # img1 = ax1.imshow(np.array(arr1), vmin=min_value, vmax=max_value, cmap=plt.cm.jet, interpolation='nearest')
  # img2 = ax2.imshow(np.array(arr2), vmin=min_value, vmax=max_value, cmap=plt.cm.jet, interpolation='nearest')
  # fig.subplots_adjust(wspace=0.5)
  #
  # data = ['MIK', 'Regions1', 'Regions2', 'Hybrid']
  # for ax in [ax1, ax2]:
  #   ax.xaxis.tick_top()
  #   ax.set_xticks(range(4))
  #   ax.set_xticklabels(data, rotation=40)
  #   ax.set_yticks(range(4))
  #   ax.set_yticklabels(data)
  # ax1.set_xlabel("Test Dataset")
  # ax2.set_xlabel("Test Dataset")
  # ax1.set_ylabel("Policy Dataset")
  # fig.tight_layout(w_pad=5)
  #
  # fig.subplots_adjust(right=0.8)
  # cbar_ax = fig.add_axes([0.85, 0.32, 0.02, 0.36])
  # fig.colorbar(img2, cax=cbar_ax)
  #
  # plt.savefig("conf_mat.pdf")
  #
