let rec
odd n = if n = 0 then 0 else even (n-1)
                               and
even n = if n = 0 then 1 else odd (n-1)
;;

even 9;;
