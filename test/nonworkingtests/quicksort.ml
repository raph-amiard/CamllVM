let rec qsort = function
  | [] -> []
  | pivot :: rest ->
      let is_less x = x < pivot in
      let left, right = List.partition is_less rest in
      qsort left @ [pivot] @ qsort right;;


let rec _print_int_list = function
    | [] -> ()
    | fst :: [] ->
        print_int fst; ();
    | fst :: rest ->
        print_int fst;
        print_string ",";
        _print_int_list rest;
        ()
;;

let print_int_list l = 
    print_string "[";
    _print_int_list l;
    print_string "]";
    print_newline ();
    ()
;;

print_int_list (qsort
[1;56;12;8;23;123;9423;9;39;4;5;12;53;43;94;239;12;30]);;
