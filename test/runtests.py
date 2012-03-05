import subprocess
import os

try:
    from termcolor import colored
except ImportError:
    def colored(txt, col):
        return txt

PATH = "/".join(os.path.realpath(__file__).split("/")[:-1]) + "/"
Z3_PATH = "../../bin/Z3"

test_num = 1

def test_print(msg):
    print "{0} {1}".format(colored("[{0}]".format(test_num), "blue"), msg)


def test_fail(test_name, message):
    test_print(colored("Test {1} failed ! {2}".format(test_num, test_name, message), "red"))


def compile_and_run(file_path):
    test_print("Running test {0}".format(file_path))

    clean = False
    if file_path.find("clean") != -1:
        clean = True

    try:
        compile_output = subprocess.check_output(["ocamlc", file_path + ".ml"], stderr=subprocess.STDOUT)
        if clean:
            subprocess.check_output(["ocamlclean", "a.out"])
    except subprocess.CalledProcessError, e:
        test_fail(file_path, "Compilation error")
        test_print("Compilation output : ")
        print e.output
        raise e

    os.remove(file_path + ".cmo")
    os.remove(file_path + ".cmi")

    try:
        options = open(file_path + ".opts").read().split(" ")
    except IOError:
        options = []
    z3_call_vect = [Z3_PATH, "a.out"] + options

    try:
        out = subprocess.check_output(z3_call_vect, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError, e:
        test_fail(file_path, "Non zero return code")
        test_print(colored("Output dumped to file test_fail.txt", "red"))
        f = open("../../test_fail.txt", "w")
        f.write(e.output)
        raise e

    if clean:
        res = out.strip()
    else:
        res = out.split("\n")[-2].strip()

    try:
        outres = open(file_path + ".out").read().strip()
        if res != outres:
            test_fail(file_path, "Non expected test value")
            print outres
            print res
            test_print("Expected value: {1}".format(outres))
            test_print("Test out value: {1}".format(res))
            print out
            raise Exception()
    except IOError:
        pass

    test_print(colored("Test {0} succeeded !".format(file_path), "green"))


os.chdir(PATH + "testfiles")

def cmp_filenames(a, b):
    na = int(a.split("_")[0])
    nb = int(b.split("_")[0])
    return na - nb

for f in sorted(os.listdir("."), cmp_filenames):
    try:
        if f.endswith(".ml"):
            compile_and_run(f.split(".")[0])
            test_num += 1
    except Exception, e:
        break

os.remove("a.out")


