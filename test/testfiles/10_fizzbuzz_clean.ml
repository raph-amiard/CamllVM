let fizzbuzz i = match ((i mod 3), (i mod 5)) with
    | (0,0) -> "fizzbuzz"
    | (_,0) -> "buzz"
    | (0,_) -> "fizz"
    | (_,_) -> string_of_int i ;;

let do_fizzbuzz n = for i = 1 to n do
    print_string (fizzbuzz i);
    print_newline ();
    done ;;

do_fizzbuzz 10;;
