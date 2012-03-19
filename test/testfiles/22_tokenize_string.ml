let rec _print_string_list = function
    | [] -> ()
    | fst :: [] ->
        print_string fst; ();
    | fst :: rest ->
        print_string fst;
        print_string ",";
        _print_string_list rest;
        ()
;;

let print_string_list l = 
    print_string "[";
    _print_string_list l;
    print_string "]";
    print_newline ();
    ()
;;
let rec split_char sep str =
  try
    let i = String.index str sep in
    String.sub str 0 i ::
      split_char sep (String.sub str (i+1) (String.length str - i - 1))
  with Not_found ->
      [str];;

let l = split_char ' ' "Hello dude how are you today";;
print_string_list l;;

