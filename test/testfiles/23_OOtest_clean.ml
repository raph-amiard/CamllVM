class point =
   object 
     val mutable x = 0
     method get_x = x
     method move d = x <- x + d
   end;;

let p = new point;;
print_int (p#get_x);
print_newline ();
p#move 3;
print_int (p#get_x);
print_newline ();
