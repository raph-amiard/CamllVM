(*************************************************************************)
(*                                                                       *)
(*                            Objective Caml                             *)
(*                                                                       *)
(*            Pierre Weis, projet Estime, INRIA Rocquencourt             *)
(*                                                                       *)
(*   Copyright 2009 Institut National de Recherche en Informatique et    *)
(*   en Automatique.  All rights reserved.  This file is distributed     *)
(*   under the terms of the Q Public License version 1.0.                *)
(*                                                                       *)
(*************************************************************************)

(* $Id: tformat.ml 10713 2010-10-08 11:53:19Z doligez $

A testbed file for the module Format.

*)

open Testing;;

open Format;;

(* BR#4769 *)
let test0 () =
  let b = Buffer.create 10 in
  let msg = "Hello world!" in
  Format.bprintf b "%s" msg;
  let s = Buffer.contents b in
  s = msg
;;

test (test0 ())
;;
