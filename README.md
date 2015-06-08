*WARNING*

This is still work in progress.

## Overview

Inspired by lisp500 and tinyscheme silc is a small lisp implementation. It is made just for fun and mostly to demonstrate lambda calculus interpretation.
It features moveable garbage collector and is perfect for use in a constrained environment (all allocations are made in a limited region of heap).

## How to build

Execute ``make all`` in the root dir.

Then do:

```
$ rlwrap src/repl/target/silc
```

Sample session log:

```
;; SilcLisp by Alex Shabanov

? (+ 1)
1
? (+)
0
? (+ 1 2 (+ 4 5))
12
? nil
nil
? true
true
? (quit)

;; Exiting... Good bye!
```
