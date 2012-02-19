(* $Id: length.mli 10713 2010-10-08 11:53:19Z doligez $

A testbed file for private type abbreviation definitions.

We define a Length module to implement positive integers.

*)

type t = private int;;

val make : int -> t;;

external from : t -> int = "%identity";;
