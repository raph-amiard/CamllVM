(* AST node types *)
type id = string
type binop = Plus | Minus | Times | Div

type stm = CompoundStm of stm * stm
		 | AssignStm of id * exp
		 | PrintStm of exp list

and exp = IdExp of id
		| NumExp of int
	    | OpExp of exp * binop * exp
	    | EseqExp of stm * exp


(* Sample program definition *)
let prog = CompoundStm(
	AssignStm("a", OpExp(NumExp 5, Plus, NumExp 3)),
	CompoundStm(
		AssignStm(
			"b", 
			EseqExp(
				PrintStm[IdExp"a"; OpExp(IdExp"a", Minus, NumExp 1)],
				OpExp(NumExp 10, Times, IdExp"a")
			)
		),
		PrintStm[IdExp"b"]
	)
)
;;

(* Function that tells the maximum number of arguments 
   of any print statement within stmt *)
let rec maxargs stmt = match stmt with
    PrintStm l -> max (List.length l) (List.fold_left max 0 (List.map maxargsexp l))
  | AssignStm (id, e) -> maxargsexp e
  | CompoundStm (s1, s2) -> max (maxargs s1) (maxargs s2)

and maxargsexp expp = match expp with
  | EseqExp (s1, e) -> maxargs s1
  | OpExp (e1, b, e2) -> max (maxargsexp e1) (maxargsexp e2)
  | NumExp (i) -> 0
  | IdExp (i) -> 0
;;

(* Eval *)
let env_update l k v =
	(List.remove_assoc k l) @ [(k, v)]


let rec eval_statement env stmt = match stmt with
    AssignStm (id, e) ->  
    	let (env1, res) = eval_exp env e in env_update env1 id res
  | CompoundStm (s1, s2) -> 
  		let env1 = eval_statement env s1 in eval_statement env1 s2
  | PrintStm (l : exp list) -> (
		match l with 
			[] -> env
			| h::t -> let (env1, res) = eval_exp env h in 
				print_int res; 
                print_newline();
				eval_statement env1 (PrintStm(t))
	)

and eval_exp env exp = match exp with
  | OpExp (e1, b, e2) -> 
  		let (env1, i1) = eval_exp env e1 in
  			let (env2, i2) = eval_exp env1 e2 in (match b with
			      Plus -> (env2, i1 + i2)
				| Minus -> (env2, i1 - i2)
				| Times -> (env2, i1 * i2)
				| Div -> (env2, i1 / i2)
			)
  | EseqExp (s1, e) -> 
  	let env1 = eval_statement env s1 in eval_exp env1 e
  | NumExp (i) -> (env, i)
  | IdExp (i) -> (env, List.assoc i env)
;;

eval_statement [] prog
