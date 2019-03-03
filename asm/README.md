# fm-asm

fm-asm assembly files resemble the structure of fm bytecode files. An
instruction section is followed by a data section. Three dashes separate
the instructions from the data section.

Instruction arguments can be integers, floats, strings, or symbols.
Symbols can be defined as instruction labels (A and B below) or as
(static) variables in the data section. Variables in the data section
can be assigned integers, floats, or strings.

## Example

```
# ./print_fm.fm-asm
   loadc 1
   loadc 0
   leq
   jumpz A
   loadc B
A: loadc 0
   halt
B: mkstringd "w"
   mkstringd stdout
   fopen
   mkstringd fm
   fputs
   jump A
---
stdout = "/dev/stdout"
fm = "fm"
```

Running `fm-asm ./print_fm.fm-asm > ./print_fm.fm` and running `fm
./print_fm.fm` would print "fm" to stdout.

See ./asm/tests for more examples.
