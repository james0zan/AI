import os, sys, random, re, time, subprocess

def genInput():
  rate = random.randint(100, 200)
  numConn = random.randint(1000, 2000)
  return (rate, numConn)

def run(httpd, server, port, rate, numConn):
  print "Start Server: %s -X &" % httpd
  P = subprocess.Popen([httpd, '-X'])
  time.sleep(5)
  print "Runing: httperf --server %s --port %s --uri / --rate %d  --num-conn %d  --num-call 1  --timeout 5" % (server, port, rate, numConn)
  (_, output) = os.popen4("httperf --server %s --port %s --uri / --rate %d  --num-conn %d  --num-call 1  --timeout 5" % (server, port, rate, numConn))
  t = re.findall("Net I/O: (\d+(.\d+)?) KB/s", output.read())[0][0]
  print "Speed:", t, "KB/s"
  P.send_signal(2)
  P.wait()
  time.sleep(5)
  return float(t)

if __name__ == '__main__':
  if len(sys.argv) != 6:
    print "[Usage:] run.py [train|test] [times] [app] [server] [port]"
    exit(1)

  TIMES = int(sys.argv[2])
  httpd = sys.argv[3]
  server = sys.argv[4]
  port = sys.argv[5]
  if sys.argv[1] == "train":
    for i in xrange(TIMES):
      (rate, numConn) = genInput()
      run('trace/'+httpd, server, port, rate, numConn)
      os.system("mv trace.bin trace." + str(i))

  if sys.argv[1] == "relax":
    err = False
    for i in xrange(TIMES):
      (rate, numConn) = genInput()
      for i in xrange(10):
        if run('relax/'+httpd, server, port, rate, numConn) < 0.01:
          err = True
          break
        os.system("$AI_HOME/build/tools/relax-bset sanitized-bset.txt violations.bin")
      if err:
        break
    if err:
      print "Something wrong with the httpd!\nPlease check wether the native version `native/bin/httpd -X` can start normally.\nIf not, you may need to restart your computer and redo the relaxing!"

  if sys.argv[1] == "relax2":
    err = False
    for i in xrange(TIMES):
      (rate, numConn) = genInput()
      for i in xrange(10):
        if run('instrumented/'+httpd, server, port, rate, numConn) < 0.01:
          err = True
          break
        os.system("$AI_HOME/build/tools/relax-bset sanitized-bset.txt violations.bin")
      if err:
        break
    if err:
      print "Something wrong with the httpd!\nPlease check wether the native version `native/bin/httpd -X` can start normally.\nIf not, you may need to restart your computer and redo the relaxing!"

  if sys.argv[1] == "test":
    Overheads = []
    for i in xrange(TIMES):
      print "============== Iter %d ==============" % (i+1)
      (rate, numConn) = genInput()
      print ">> Native Application:"
      nativeSpeed = run('native/'+httpd, server, port, rate, numConn)
      print ">> Instrumented Application:"
      instrumentedSpeed = run('instrumented/'+httpd, server, port, rate, numConn)

      o = max(0, (nativeSpeed/instrumentedSpeed) - 1)
      Overheads.append(o)
      print ">> Result:"
      print "Current Overhead: %lf\t Average Overhead: %lf\n" % (o, sum(Overheads)/len(Overheads))
      print ""


