import os, sys, random, re

def genInput():
  numThreads = random.choice([2,4])
  num = random.choice([4, 8, 16])
  return (num, numThreads)

def run(fmm, num, numThreads):
  print "Runing: %s -m%d -p%d -n6553600 -l4" % (fmm, num, numThreads)
  (_, output) = os.popen4('%s -m%d -p%d -n6553600 -l4' % (fmm, num, numThreads))
  t = re.findall("Total time without initialization\s+:\s+(\d+)", output.read())[0]
  print "Totle Used Time:", t
  return float(t)

if __name__ == '__main__':
  if len(sys.argv) != 4:
    print "[Usage:] run.py [train|test] [times] [app]"
    exit(1)

  TIMES = int(sys.argv[2])

  if sys.argv[1] == "train":
    for i in xrange(TIMES):
      (num, numThreads) = genInput()
      run(sys.argv[3] + ".trace", num, numThreads)
      os.system("mv trace.bin trace." + str(i))

  if sys.argv[1] == "train2":
    for i in xrange(TIMES):
      (num, numThreads) = genInput()
      run(sys.argv[3] + ".trace2", num, numThreads)
      os.system("mv trace.bin trace2." + str(i))

  if sys.argv[1] == "relax":
    for i in xrange(TIMES):
      (num, numThreads) = genInput()
      for i in xrange(50):
        run(sys.argv[3] + ".relax", num, numThreads)
        os.system("$AI_HOME/build/tools/relax-bset sanitized-bset.txt violations.bin")

  if sys.argv[1] == "test":
    Overheads = []
    for i in xrange(TIMES):
      print "============== Iter %d ==============" % (i+1)
      (num, numThreads) = genInput()
      print ">> Native Application:"
      nativeTime = run(sys.argv[3] + ".native", num, numThreads)
      print ">> Instrumented Application:"
      instrumentedTime = run(sys.argv[3] + ".instrumented", num, numThreads)

      o = max(0, (instrumentedTime/nativeTime) - 1)
      Overheads.append(o)
      print ">> Result:"
      print "Current Overhead: %lf\t Average Overhead: %lf\n" % (o, sum(Overheads)/len(Overheads))
      print ""

  if sys.argv[1] == "bias":
    Overheads = []
    for i in xrange(TIMES):
      print "============== Iter %d ==============" % (i+1)
      (num, numThreads) = genInput()
      print ">> Native Application:"
      nativeTime = run(sys.argv[3] + ".native", num, numThreads)
      print ">> Bias Instrumented Application:"
      instrumentedTime = run(sys.argv[3] + ".bias", num, numThreads)

      o = max(0, (instrumentedTime/nativeTime) - 1)
      Overheads.append(o)
      print ">> Result:"
      print "Current Overhead: %lf\t Average Overhead: %lf\n" % (o, sum(Overheads)/len(Overheads))
      print ""

