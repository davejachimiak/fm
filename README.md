# fm

fm is a stand-alone virtual machine for functional languages. It's an
implementation of the Mauer Machine, which was described in [_Compiler
Design_](https://www.amazon.com/Compiler-Design-International-Computer-Science/dp/0201422905)
and [_Compiler Design: Virtual
Machines_](https://www.amazon.com/Compiler-Design-Machines-Reinhard-Wilhelm/dp/3642149081).

fm defines code layout and behavior unspecified in the Mauer Machine, namely

* 64-bit stack entries
* access to some glibc functions via instructions

## Status

fm is in toy phase; it's far from being production-ready. It's missing:

* garbage collection
* sockets
* threads
* ~an assembler~ done. see [`./asm`](https://github.com/davejachimiak/fm/tree/master/asm)
* an object file format and linker
* other useful stuff

## Use

To build the vm:

```sh
make fm
```

To run the vm:

```sh
./build/fm <fm-bytecode-file>
```

To build the assembler:

```sh
make assembler
```

To run the assember:

```sh
./build/fm-asm <fm-assembly-file>
```

## Bytecode layout

An fm program is a bytecode file. fm expects the bytecode file to begin
with instruction units. An optional data section should follow the
series of instruction units.

See `./vm/tests` for examples.

### Instruction format

A bytecote file should begin with a series of instruction units. Each
instruction unit should be 9 bytes. The first byte should be the
instruction code and the following 8 bytes should some a 64-bit value.
The last 8 bytes of the instruction unit is the instruction argument.
fm expects and ignores the 8-bytes after the first instruction byte even
when instructions do not require an argument.

### Data section

The data section should follow the instructions. Instruction arguments
that refer to data addresses should indicate the zero-indexed
byte-position relative to the beginning of the bytecode file.

## Registers

### `program_counter`

This keeps track of the next instruction to invoke. It's usually
incremented by 9 after an instruction is executed (to get to the next
9-byte instruction unit), except in the case of jumps in if-then-else
expressions, match expressions, and function entry/exit.

### `stack_pointer`

This points to the top of the stack.

### `function_entry_pointer`

This points to the stack entry when `apply` began evaluating the current
function (or when the vm started), and changes whenever `apply`, `targ`,
and `return` are invoked.

### `global_pointer`

Points to the global vector of free variables for the current function.

### `frame_pointer`

Points to the bottom of the stack frame. Helps the machine refer to
previous register values from outer functions when leaving closure evals
and function applications.

## The stack

fm's stack is made up 8-byte entries. The ways in which
instructions manipulate the stack are described in "Instructions" below.

Arithmetic expressions read the stack entries they operate on as signed
64-bit integers, or signed 64-bit doubles in the case of of the `f64*`
instructions.

Each instruction increments the program counter unless otherwise noted.

## Instructions

### `halt` (`x00`)

This exits the program. The exit code is the value at the top of the
stack.

### `loadc(constant)` (`x01`)

This pushes `constant` to the stack.

For example, a `constant` `x01x30x39x00x00x00x00x00x00` loads the int
`12345` to the top of the stack.

### `add` (`x02`)

Adds the two entries on top of the stack, pops the stack, sets the top
of the stack to the result.

### `sub` (`x03`)

Subtracts the entry at the top of the stack from the second-from-the-top
entry, pops the stack, sets the top of the stack to the result.

### `mul` (`x04`)

Multiplies the two entries on top of the stack, pops the stack, sets the top
of the stack to the result.

### `div` (`x05`)

Divides the entry at the top of the stack by the second-from-the-top
entry, pops the stack, sets the top of the stack to the result.

### `mkbasic` (`x06`)

Replaces the entry of the top of the stack with the value wrapped in a
basic tag.

### `getbasic` (`x07`)

Replaces the basic-tagged entry at the top of the stack with the value
it's wrapping.

### `leq` (`x08`)

Performs a less-then-or-equal comparison between entry second from the
top of the stack and the entry at the top, pops the stack, sets the top
of the stack to the result.

### `neg` (`x09`)

Replaces the entry at the top of the stack with its negative value.

### `jump(instruction_byte)` (`x0B`)

Sets the program counter to `instruction_byte`. In other words,
`instruction_byte` should address the exact byte of the instruction unit
to jump to relative to the start of the program. For example, the first
instruction of the program would be at `instruction_byte` 0 and the
second instruction of the program would be at `instruction_byte` 9.

### `jumpz(instruction_byte)` (`x0C`)

Conditional jump. If top-most stack entry is 0 (falsey), this jumps to
the `instruction_byte`. If not, it increments the program counter.

This pops the stack regardless of whether the condition was met.

### `pushloc(local_address)` (`x0D`)

Pushes an argument or local variable to the stack. `local_address`
determines which argument or local variable to push. Arguments zero and
below refer to arguments and zero and above refer to local variables.

### `mkvec(size)` (`x0E`)

Take `size` entries from the top of the stack, make a Vector object with
them, pop the stack `size - 1` times, and set the object into the entry at
the top of the stack.

### `pushglob(index)` (`x0F`)

Push a global variable to the stack. `index` determines which variable
to push from the global vector in the current function.

### `mkclos(instruction_byte)` (`x10`)

Expects a vector object at the top of the stack, which contains the
global (free) variables referred to in the closure.

`mkclos` replaces the vector object at the top of the stack with a
closure object. `instruction_byte` should refer to the byte of the
instruction unit of the closure in the program. When closures are
evaluated, the `program_counter` is set to this `instruction_byte`.

### `eval` (`x11`)

If the entry at the top of the stack is a closure, `eval` evaluates it.

If the entry at the top of the stack is not a closure, `eval` is a
no-op.

### `update` (`x12`)

For use at the end of code generated for closures. `update`:

* removes the stack frame for the closure, leaving the top-most entry,
  which is the result of the evaluated closure
* replaces the closure in the heap with the result of the evaluated
  closure
* sets the program counter back to where it was before the evaluation of
  the closure to resume evaluation of the program

### `slide(stack_positions)` (`x13`)

Usually used to remove local variables from the stack before returning a
from a function evaluation. `slide` does this by sliding the top-most
entry on the stack, which is the result of evaluating the function, down
the stack by `stack_postitions`.

### `mkfunval(instruction_byte)` (`x14`)

Replaces the value at the top of the stack, which should be a pointer to
a vector for use as the global (free) variables in the function
evaluation, with a function object.
 
`instruction_byte` should be the zero-indexed byte of the first
instruction of the function in the fm program.

### `targ(arity)` (`x15`)

This instruction should begin functions.

If the number of arguments pushed to the stack before function
application is below `arity`, then `targ`:

* replaces function-under-evalution's function object with a new one
  with references to those arguments
* pops the stack frame created by `mark` to resume evaluating the outer
  function

This is how fm supports curried functions.

If the number of supplied arguments is not below `arity`, `targ` just
increments the program counter to resume evaluation of the function.

### `return(arity)` (`x16`)

This instruction should end functions.

If there are more arguments pushed to the stack before function
application than `arity`, then `return`:

* slides the entry at the top of the stack, which should be a function
  object, down the stack by `arity` entries
* applies the function object at the top of the stack to the rest of the
  arguments

This is how fm finishes evaluating partially-applied functions. See
`tests/argument_oversupply_function_application` for an example.

If there are the same number of arguments pushed to the stack as `arity`
then `return` releases the function's stack frame, leaving the value of
the evaluated function at the top of the stack, and resumes evaluation
of the outer function.

### `mark(return_byte)` (`x17`)

Creates a stack frame for function application, marking the current
global pointer, frame pointer, function-entry stack pointer, and
`return_byte` on the stack. This allows `targ` and `return` to resume
evaluation of the outer function after exiting the inner function
evaluation.

### `apply` (`x18`)

Expects a function object at the top of the stack.

`apply` applies a function to the arguments previously pushed to the
stack.

### `alloc(n)` (`x19`)

Allocates `n` dummy references and pushes them to the stack. This is
used in conjunction with `rewrite` when defining recursive functions.

### `rewrite(relative_stack_location)` (`x1A`)

Rewrites the heap address pointed to by the stack entry at
`stack_pointer - relative_stack_location` with the heap value of the
pointer at `stack_pointer` (top of the stack).

This is helpful when defining recursive functions. For example,
allocating a dummy reference with `alloc(1)`, defining a recursive
function which has access to that dummy reference through its
free-variable vector, and then rewriting the dummy reference with the
recursive function's function object allows applications of that
function to apply a copy of the function.

### `copyglob` (`x1B`)

This pushes the current global pointer to the stack. `copyglob` is an
instruction to optimize the creation of global (free) vectors. For
example, say the free variables for an upcoming function declaration are
the same as what's passed into the current function. Instead of pushing
the variables from the current global vector and making a new vector out
of them, `copyglob` just pushes the current global pointer to the stack.

### `getvec(n)` (`x1C`)

This instruction unwraps `n` values from the vector at the top of the
the stack onto the stack. This is good for unwrapping variables from a
de-sugared tuple `let` expression so that the `in` part of the `let`
statment can refer act on those variables by pushing references of them
to the stack via `pushloc`.

### `nil` (`x1D`)

Pushes a Nil object to the stack. Used to denote an empty list or the
end of the list if it's in the second cell of a cons.

### `cons` (`x1E`)

`cons`:

* creates a cons object
* makes the top entry of the stack the head of the cons
* makes the second-most top entry of the stack the tail of the cons
* decrements the stack pointer
* sets the top entry of the stack to the cons object

### `tlist(instruction_byte)` (`x1F`)

Unwraps a list type at the top of the stack to the stack. Useful for
pattern matching on lists.

If the list is a cons, it sets the top of the stack to the head, pushes
the tail to the stack, and sets the code pointer to `instruction_byte`.
If the list is a nil, it pops it from the stack.

### `slide2(size)` (`x20`)

Slide `size` entries from the top of the stack down to the function's
entrypoint on the stack.

### `loadd(program_address)` (`x21`)

Push the 8 continguous bytes at `program_address` of the program onto
the stack. Good for pushing 8-byte number constants from the data
section onto the stack.

### `mkstringd(program_address)` (`x22`)

Push a string object from a string in the data section starting at the
`program_address` to the stack. The first eight bytes at
`program_address` must be a 64-bit unsigned integer that indicates the
size of the string. The string bytes must follow those eight bytes.

### `fopen` (`x23`)

Delegates to glibc's `fopen`. The first argument to `fopen` is a string
object at the top of the stack and the second argument comes from a
string object at the penultimate stack entry. This pops the stack, and
sets the entry at the top of the stack to the file pointer returned by
`fopen`.

### `fputs` (`x24`)

Delegates to glibc's `fputs`. The first argument to `fputs` is a string
object at the top of the stack and the second argument comes from a file
pointer at the penultimate stack entry. This pops the stack, and sets
the integer result from `puts` to the entry at the top of the stack.

### `fgets` (`x25`)

Delegates to glibc's `fgets`. The second argument to `fgets` is an
unsigned 8-byte integer from the entry at the top of the stack, and the
third argument is a string object at the penultimate stack entry. (The
first argument is handled by fm internally.) This pops the stack,
creates a string object from the bytes read by `fgets`, and sets that
string object to the entry at the top of the stack.

### `appends` (`x26`)

This appends two strings into one by creating a new string object out of
the two string objects at the top of the stack. It appends the string
object in the penultimate stack entry to the end of that of top-most
stack entry, pops the stack, and sets the new string object to the entry
at the top of the stack.

### `f64tos(decimal_points)` (`x27`)

Replaces the raw 64-bit float on top of the stack with a string object
that represents the float with the number of `decimal_points`.

### `f64add` (`x28`)

Adds the two 64-bit, double-precision floats on top of the stack, pops
the stack, sets the top of the stack to the result. This instruction
differs from `add` in that the underlying C interprets the 8-byte stack
entries as 64-bit floats instead of integers.

### `f64sub` (`x29`)

Subtracts the 64-bit float on top of the stack from the float in the
second-from-the-top entry, pops the stack, sets the top of the stack to
the result. This instruction differs from `sub` in that the underlying C
interprets the 8-byte stack entries as 64-bit floats instead of
integers.

### `f64mul` (`x2A`)

Multiplies the two 64-bit floats on top of the stack, pops the stack,
sets the top of the stack to the result. This instruction differs from
`mul` in that the underlying C interprets the 8-byte stack entries as
64-bit floats instead of integers.

### `f64div` (`x2B`)

Divides the 64-bit float on top of the stack by the float in the
second-from-the-top entry, pops the stack, sets the top of the stack to
the result. This instruction differs from `div` in that the underlying C
interprets the 8-byte stack entries as 64-bit floats instead of
integers.

### `itos` (`x2C`)

Replaces the raw 64-bit long signed integer on top of the stack with a
string object representing the integer.
