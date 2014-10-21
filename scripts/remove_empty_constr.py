import sys, re
from subprocess import call

if __name__ == '__main__':
   fname = sys.argv[1]
   prev = None
   #with open(fname, 'r') as fin, open(fname+'.tmp', 'w') as fout:
   #   for line in fin:
   #      line = line.strip()
   #      if re.match(r'^\s*[<>=]', line) and prev == '':
   #         print 'delete line:', line
   #      else:
   #         fout.write('%s\n' % line)
   #         prev = line
   call(('mv %s.tmp %s' % (fname, fname)).split(' '))
