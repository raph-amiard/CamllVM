let rec fibo a = if a < 2 then 1 else fibo (a - 2) + fibo (a - 1);;

print_int (fibo 35);;
print_newline ();;
