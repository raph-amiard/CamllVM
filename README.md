Z3 README
=========

This is the readme for the Z3 Project, a Zam to LLVM compiler and (hopefully) runtime.

A word of disclaimer
--------------------

This project is nothing but ideas at the moment, and most of them are not fixed yet. This github space is meant as a discussion space for the time being, and this disclaimer will be removed when runnable and useful code is put in this repo. *For the moment, all code you'll find here is scraps and implementation ideas*

Building the project
--------------------

The default makefile will use your default c++ compiler. It depends on boost for integer types, so make sure you have boost dev installed.

What does it do for the moment
------------------------------

Parses Ocaml executable file format, and reads instructions.
