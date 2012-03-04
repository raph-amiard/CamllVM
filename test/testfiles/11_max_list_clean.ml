let my_max = function
    [] -> invalid_arg "empty list"
  | x::xs -> List.fold_left max x xs;;

print_int(my_max [12;56;69;4;22;43]);;
print_newline ();;
