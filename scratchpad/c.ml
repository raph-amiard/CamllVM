let fn a b = a + b;;

let fn2 a b c = a * b + c;;

let fn3 a b c = (a * b + c) in 
    let fnnn d e f = (d + e - (fn3 d e f)) in
        fnnn 5 6 7;;

let fn4 a b c = a * b + c;;

let fn5 a b c d e f = a + b + c + d + e + f
