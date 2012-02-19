#! /bin/bash

fname=${1%.ml}
ocamlc $1
ocamlclean a.out
bin/Z3 a.out
rm $fname.cmi $fname.cmo a.out
