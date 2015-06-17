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

## Lambda calculus demo

This is an illustration how Church numerals can work in silc:

```
? (define zero (lambda (s) (lambda (z) z)))
#<CLOSURE-12A>
? (define succ (lambda (n) (lambda (s) (lambda (z) (s ((n s) z))))))
#<CLOSURE-19A>
? (define n0 zero)
#<CLOSURE-12A>
? (define n1 (succ n0))
#<CLOSURE-1E6>
? (define n2 (succ n1))
#<CLOSURE-21A>
? ((n1 inc) 0)
1
? ((n0 inc) 0)
0
? ((n0 inc) 0)
0
? ((n1 inc) 0)
1
? (define n3 (succ n2))
#<CLOSURE-502>
? (define pow (lambda (b) (lambda (e) (e b))))
#<CLOSURE-55E>
? ((((pow n2) n3) inc) 0)
8
? ((((pow n3) n2) inc) 0)
9
?
```

Alternatively run the following to introduce the required definitions in one turn:

```
(begin

  (define zero (lambda (s) (lambda (z) z)))
  (define succ (lambda (n) (lambda (s) (lambda (z) (s ((n s) z))))))
  (define n0 zero)
  (define n1 (succ n0))
  (define n2 (succ n1))
  (define n3 (succ n2))
  (define n4 (succ n3))
  (define n5 (succ n4))
  (define n6 (succ n5))
  (define n7 (succ n6))
  (define n8 (succ n7))
  (define n9 (succ n8))
  (define pow (lambda (b) (lambda (e) (e b))))

  )
```

Verification:

```
(((succ zero) inc) 0)
(((succ n1) inc) 0)
(((succ n2) inc) 0)
(((succ n3) inc) 0)

((((pow n2) n3) inc) 0)
((((pow n3) n2) inc) 0)

((((pow n3) n4) inc) 0)
```