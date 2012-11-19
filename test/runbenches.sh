#!/bin/bash

export GDFONTPATH=/usr/share/fonts
export GNUPLOT_DEFAULT_GDFONT=verdana

dir=./benches
cd `dirname "$0"`
for file in `ls $dir/*.ml`; do
    if [ "$file" = "$dir/mandelbrot.ml" ]; then
        ocamlopt graphics.cmxa "$file" -o "$file.opt" 2>/dev/null
        ocamlc graphics.cma -dllpath /usr/lib/ocaml/stublibs "$file" 2>/dev/null
    else
        ocamlopt "$file" -o "$file.opt" 2>/dev/null
        ocamlc "$file" 2>/dev/null
    fi

    opt=`/usr/bin/time -f '%E' "./$file.opt" 2>&1`
    run=`/usr/bin/time -f '%E' ocamlrun a.out 2>&1`
    ocamllvm=`../bin/ocamllvm -t a.out 2>/dev/null` 


    echo "$file"
    echo -en "opt:\t"
    echo "$opt" | tail -n 1
    echo -en "run:\t"
    echo "$run" | tail -n 1
    echo -en "ocamllvm:\t"
    echo "$ocamllvm" | tail -n 1
    rm "$file.opt"
done

rm  a.out "$dir"/*.cm* "$dir"/*.o
