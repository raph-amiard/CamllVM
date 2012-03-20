(***********************************************************************)
(*                                                                     *)
(*                           Objective Caml                            *)
(*                                                                     *)
(*               Pierre Weis, projet Cristal, INRIA Rocquencourt       *)
(*                                                                     *)
(*  Copyright 2001 Institut National de Recherche en Informatique et   *)
(*  en Automatique.  All rights reserved.  This file is distributed    *)
(*  only by permission.                                                *)
(*                                                                     *)
(***********************************************************************)

(*
- Variables exist in Caml.
    A variable x is defined using the ``ref'' variable constructor
    applied to its initial value (initial value is mandatory).
    let x = ref 0
  The variable can be modified using the assignment operator :=
    x := 3
*)
let chars = ref 0
and words = ref 0
and lines = ref 0
;;

(*
- New type definitions are introduced by the keyword type. To define an
  enumerated type, just list the set of alternatives.
  Here type state introduced two cases, Inside_word and Outside_word,
  that will serve3 to denote if we are scanning a word or not.
*)
type state = Inside_word | Outside_word;;

(*
- Case analysis is introduced by match. It is a list of clauses
  | pat -> e
  meaning that if pat is the case at hand, e should be returned.
  For instance, to return integer 1 if character c is 'a' and 2 if c
   is 'b', use
   match c with
   | 'a' -> 1
   | 'b' -> 2
  A catch all case is introduced by special pattern ``_''. Hence,
  match c with
  | 'a' -> true
  | _ -> false
  tests is character c is 'a'.

- Character can be read in input channel using primitive input_char.

- Primitive incr, increments a variable.
*)
let count_channel in_channel =
  let rec count status =
    let c = input_char in_channel in
    incr chars;
    match c with
    | '\n' ->
        incr lines; count Outside_word
    | ' ' | '\t' ->
        count Outside_word
    | _ ->
        if status = Outside_word then incr words;
        count Inside_word in
  try
    count Outside_word
  with End_of_file -> ()
;;

(*
- Primitive open_in opens an input channel.
*)
let count_file name =
  let ic = open_in name in
  count_channel ic;
  close_in ic
;;

(*
- The current value of variable x is denoted by !x.
  Hence incr x is equivalent to x := !x + 1
*)
let print_result () =
  print_int !chars; print_string " characters, ";
  print_int !words; print_string " words, ";
  print_int !lines; print_string " lines";
  print_newline ()
;;

let count name =
  count_file name;
  print_result ()
;;  

try
  count_file "27_wc_clean.ml";
  print_result ();
with Sys_error s ->
  print_string "I/O error: ";
  print_string s;
  print_newline ()
;;
