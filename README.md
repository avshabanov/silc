*WARNING*

This is still work in progress.

## Overview

Inspired by lisp500 and tinyscheme silc is a small lisp implementation. It is made just for fun and mostly to demonstrate lambda calculus interpretation.
It features moveable garbage collector and is perfect for use in a constrained environment (all allocations are made in a limited region of heap).

## How to build

Execute ``./configure && make repl`` in the root dir.

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
? (define foo (lambda (a b) (+ a b 1000)))
#<CLOSURE>
? (foo 1 2)
1003
? (quit)

;; Exiting... Good bye!
```

## Garbage collection demo

Create some garbage in repl (notice closure objects):

```
rlwrap -n src/repl/target/silc
;; SilcLisp by Alex Shabanov

? (lambda ())
#<CLOSURE-A2>
? (lambda ())
#<CLOSURE-AE>
? (lambda ())
#<CLOSURE-BA>
? (lambda ())
#<CLOSURE-C6>
? (+ 1 2 (+ 3 4 (+ 5 6 7)) 8 9 10)
55
? (lambda ())
#<CLOSURE-10E>
```

Then trigger garbage collection:

```
? (gc)
;; starting garbage collection...
;; garbage collected.
nil
```

Then notice, that newly created objects (lambdas) are allocated in place of the previously allocated
and now garbage collected objects:

```
? (lambda ())
#<CLOSURE-A2>
?
```
