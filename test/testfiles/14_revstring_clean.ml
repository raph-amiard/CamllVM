let rev_string str =
  let len = String.length str in
  let res = String.create len in
  let last = len - 1 in
  for i = 0 to last do
    let j = last - i in
    res.[i] <- str.[j];
  done;
  (res);;

print_string (rev_string ("Hello my dear how are we today"));;
print_newline ();;
