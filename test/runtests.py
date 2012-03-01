import subprocess
import os
from termcolor import colored

Z3_PATH = "../../bin/Z3"

test_num = 1

def test_fail(test_name, message):
    print colored("{0}: Test {1} failed ! {2}".format(test_num, test_name, message), "red")

def compile_and_run(file_path):
    print "{1}: Running test {0}".format(file_path, test_num)

    try:
        compile_output = subprocess.check_output(["ocamlc", file_path + ".ml"], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError, e:
        test_fail(file_path, "Compilation error")
        print "Compilation output : "
        print e.output
        return

    os.remove(file_path + ".cmo")
    os.remove(file_path + ".cmi")

    options = open(file_path + ".opts").read().split(" ")
    z3_call_vect = [Z3_PATH, "a.out"] + options

    try:
        out = subprocess.check_output(z3_call_vect, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
        test_fail(file_path, "Non zero return code")
        return

    res = out.split("\n")[-2].strip()

    try:
        outres = open(file_path + ".out").read().strip()
        if res != outres:
            test_fail(file_path, "Non expected test value")
            print "{0}: Expected value: {1}".format(test_num, outres)
            print "{0}: Test out value: {1}".format(test_num, res)
            print out
            return
    except IOError:
        pass

    print colored("{1}: Test {0} succeeded !".format(file_path, test_num), "green")


os.chdir("test/testfiles")

def cmp_filenames(a, b):
    na = int(a.split("_")[0])
    nb = int(b.split("_")[0])
    return na - nb

for f in sorted(os.listdir("."), cmp_filenames):
    if f.endswith(".ml"):
        compile_and_run(f.split(".")[0])
        test_num += 1

os.remove("a.out")


