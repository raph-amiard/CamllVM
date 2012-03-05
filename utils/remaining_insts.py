import re
import os


PATH = "/".join(os.path.realpath(__file__).split("/")[:-1]) + "/"

impl_instrs = set(
    re.findall("case (.+)?:",
               open(PATH + "../src/GenBlock.cpp").read().split("switch (")[-1])
)
all_instrs = set(
    re.findall("code\((.+)?, [0-9]\)",
               open(PATH + "../include/Instructions.hpp").read())
)

print "============================================================================="
print "#               Instructions that still have to be implemented              #"
print "============================================================================="

insts = sorted(all_instrs - impl_instrs - set(["GRAB", "RESTART"]))
for inst in insts:
    print inst

print "{0} instructions remaining".format(len(insts))
