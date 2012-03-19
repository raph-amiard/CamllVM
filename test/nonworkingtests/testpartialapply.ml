let fn a b = a + b;;

let c = fn 1;;
let d = fn 2;;

print_newline (print_int (d 3));;
print_newline (print_int (c 3));;
