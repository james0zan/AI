import os, sys, random, re

def genInput():
  numThreads = random.randint(2, 5)
  fileSize = random.randint(10, 30)
  fileName = os.path.join(os.path.realpath("."), "random.tmp")
  f = open(fileName, "wb")
  for _ in xrange(fileSize):
    f.write(os.urandom(1024 * 1024))
  f.close()
  return (numThreads, fileName, fileSize)

def run(pbzip2, numThreads, tmpFileName):
  print "Runing: %s -k -v -f -p%d %s" % (pbzip2, numThreads, tmpFileName)
  (_, output) = os.popen4('%s -k -v -f -p%d %s' % (pbzip2, numThreads, tmpFileName))
  t = re.findall("Wall Clock: (\d+.\d+) seconds", output.read())[0]
  print "Totle Used Time:", t
  return float(t)

if __name__ == '__main__':
  if len(sys.argv) != 4:
    print "[Usage:] run.py [train|test] [times] [app]"
    exit(1)

  TIMES = int(sys.argv[2])

  if sys.argv[1] == "train":
    for i in xrange(TIMES):
      (numThreads, tmpFileName, sz) = genInput()
      run(sys.argv[3] + ".trace", numThreads, tmpFileName)
      os.system("rm -f " + tmpFileName)
      os.system("rm -f " + tmpFileName + ".bz2")
      os.system("mv trace.bin trace." + str(i))

  if sys.argv[1] == "relax":
    for i in xrange(TIMES):
      (numThreads, tmpFileName, sz) = genInput()
      for i in xrange(10):
        run(sys.argv[3] + ".relax", numThreads, tmpFileName)
        os.system("$AI_HOME/build/tools/relax-bset sanitized-bset.txt violations.bin")

  if sys.argv[1] == "test":
    Overheads = []
    for i in xrange(TIMES):
      print "============== Iter %d ==============" % (i+1)
      (numThreads, tmpFileName, sz) = genInput()
      print ">> Generated Input:"
      print "Number of Threads:", numThreads
      print "Size of File: %d MB" % sz
      print ">> Native Application:"
      nativeTime = run(sys.argv[3] + ".native", numThreads, tmpFileName)
      print ">> Instrumented Application:"
      instrumentedTime = run(sys.argv[3] + ".instrumented", numThreads, tmpFileName)
      os.system("rm -f " + tmpFileName)
      os.system("rm -f " + tmpFileName + ".bz2")

      o = max(0, (instrumentedTime/nativeTime) - 1)
      Overheads.append(o)
      print ">> Result:"
      print "Current Overhead: %lf\t Average Overhead: %lf\n" % (o, sum(Overheads)/len(Overheads))
      print ""
