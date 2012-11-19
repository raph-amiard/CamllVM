CamllVM : An LLVM backed runtime for OCaml
=========================================

CamllVM allows you to run OCaml bytecode with the regular OCaml runtime, and an LLVM backend.

It is quite faster than the regular OCaml interpreter, but a lot slower than ocamlopt, OCaml's native code compiler.

The idea is to reuse ocaml's runtime and builtins, but to plug in an LLVM backed native code generator.

A word of caution
-----------------

*THIS IS ALPHA SOFTWARE*
A lot of things don't yet work. If you would be so kind, send me test cases with complete instructions on how to reproduce the case.

Building the project
--------------------

### What you need

- A recent version of clang and LLVM (post 3.0 for LLVM). 
The one on debian unstable is working fine, but you can build it too if your distro doesn't provide recent enough packages.
- gcc (for building the OCaml runtime). OCaml will build with clang if you feel brave enough to edit its makefiles.
- libboost-program-options

### How does the build works

CamllVM depends on a *very slightly modified* version of the OCaml runtime, so the first thing that will be done is to get OCaml's sources, patch them, and build this modified version of the runtime.
Then CamllVM will be built.
CamllVM is coded in C++, because it is LLVM's language, and more convenient in my opinion regarding use of data structures and such than pure C. It could have been done in OCaml, but it wasn't.

How to use it
-------------

CamllVM works in the same way ocamlrun does : It takes a bytecode file in, and compiles *and* runs the program.
There is no option *yet* for AOT compilation, but it shouldn't be too hard to bake in.

So the workflow is something like :

~~~sh
ocamlc myprogram.ml
camllvm a.out
~~~

Testing
-------

Run "python tests/runtests.py" to run all regression tests

### About it

This project is a proof of concept, and a way to explore some ideas regarding compilation of OCaml bytecode.
The version on the main trunk is one of such approaches, and the one that works the best, but also the simplest and slowest.

Here is the performance you can expect from it :

![camllvm benchmarks](http://i.imgur.com/pS7fv.png)

CamllVM is named "Z3" here, because it was the original name for this project.

### If you like that

Mail me if you are interrested in the project : raph.amiard at gmail dot com

Follow me on twitter for updates : [@notfonk](http://twitter.com/notfonk)

