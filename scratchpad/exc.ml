exception Exc1;;
exception Exc2 of int;;

let c () =
  (raise (Exc2(42));
  ())
;;

let b () =
  try
    c ()
  with Exc1 ->
      ()
;;

let a () =
  try
    b ()
  with Exc2(a) -> ()
;;

a ();;
