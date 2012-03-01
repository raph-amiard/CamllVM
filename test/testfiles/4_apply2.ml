let func a b =
    let fn c = a + b + c in
        fn;;

let adder = func 12 15;;

adder 10;;
