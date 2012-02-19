#! /bin/bash
echo "======== Original ml file ========"
echo ""
cat $1
fname=${1%.ml}
ocamlc $1
echo ""
echo "============ Z3 output ============"
echo ""
bin/Z3 a.out
rm $fname.cmi $fname.cmo a.out
