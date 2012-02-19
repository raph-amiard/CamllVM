let fn1 a = 
    (let fn2 b =
        (let fn3 c = a + b + c in
            fn3));;

fn1 4 5 6 ;;
