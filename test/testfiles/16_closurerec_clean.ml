let rec
odd n = if n = 0 then 0 else even (n-1)
                               and
even n = if n = 0 then 1 else odd (n-1)
;;

print_int (even 9);
print_newline ();;
print_int (even 10);
print_newline ();;
