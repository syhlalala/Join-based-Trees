import os
f = open("run.sh", "w")

program_name = "intervalTree"
#for n in [100000, 1000000, 10000000, 20000000,50000000,100000000]:
for n in [100000, 1000000, 10000000, 100000000]:
    thread = 1
    while thread<=128:
        cmdExp = "export CILK_NWORKERS=" + str(thread) + "\n"
        f.write(cmdExp)
        cmd = "./" + program_name + " " + str(n)
        if (thread > 1):
            cmd = "numactl -iall "+cmd + " 10\n"
        else:
            cmd = cmd+" 5\n"
        f.write(cmd)
        thread = thread*2
    thread = 144
    cmdExp = "export CILK_NWORKERS=" + str(thread) + "\n"
    f.write(cmdExp)
    cmd = "numactl -iall ./" + program_name + " " + str(n) + " 10\n"
    f.write(cmd)

f.close()


