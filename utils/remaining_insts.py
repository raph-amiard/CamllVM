import re
import os

print "============================================================================="
print "#               Instructions that still have to be implemented              #"
print "============================================================================="

PATH = "/".join(os.path.realpath(__file__).split("/")[:-1]) + "/"

impl_instrs = set(
    re.findall("case (.+)?:",
               open(PATH + "../src/GenBlock.cpp").read().split("switch (")[-1])
)
all_instrs = set(
    re.findall("code\((.+)?, [0-9]\)",
               open(PATH + "../include/Instructions.hpp").read())
)

for inst in sorted(all_instrs - impl_instrs):
    print inst
