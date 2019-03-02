#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <math.h>
#include <assert.h>

#include "instructions.h"

static_assert(sizeof(long) == 8, "Error: fm only supports architectures and OSs that have 8-byte longs. Cannot compile.");

// tags
static char BASIC_T = 'B';
static char VECTOR_T = 'V';
static char CLOSURE_T = 'C';
static char FUNCTION_T = 'F';
static char NIL_T = 'N';
static char CONS_T = 'S';

typedef struct {
  char type;
  long value;
} basic;

typedef struct {
  char type;
  long size;
  long * v;
} vector;

typedef struct {
  char type;
  long code_pointer;
  long global_pointer;
} closure;

typedef struct {
  char type;
  long code_pointer;
  long argument_pointer;
  long global_pointer;
} func;

typedef struct {
  char type;
  long head;
  long tail;
} list;

typedef struct {
  long length;
  char * chars;
} string;

// stack, heap, and registers

typedef struct {
  long * stack;
  long size;
} stack;

stack * fm_stack;

void init_stack() {
  long initial_size = 1024;

  fm_stack = malloc(sizeof(stack));
  long * stack = malloc(sizeof(long) * initial_size);

  fm_stack->stack = stack;
  fm_stack->size = initial_size;
}

void increase_stack() {
  long size = fm_stack->size + 1024;
  long * stack = realloc(fm_stack->stack, sizeof(long) * size);

  fm_stack->stack = stack;
  fm_stack->size = size;
}

long stack_entry(long idx);
void set_stack_entry(long idx, long value);
char * heap[1024];
long heap_counter = 0;
// program counter
long program_counter = 0;
// stack pointer
signed long stack_pointer = -1;
// stack pointer at function entry
signed long function_entry_pointer = -1;
// global pointer
signed long global_pointer = -1;
// frame pointer
signed long frame_pointer = -1;

void ensure_enough_stack(signed long offset) {
  if ((stack_pointer + offset) > (fm_stack->size - 1)) {
    increase_stack();
  }
}

// object creation
long new_basic(long value) {
  long heap_ptr = heap_counter;
  basic * ptr = malloc(sizeof(basic));

  ptr->type = BASIC_T;
  ptr->value = value;

  heap[heap_counter] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long new_vector(long size) {
  long heap_ptr = heap_counter;
  vector * ptr = malloc(sizeof(vector));

  ptr->type = VECTOR_T;
  ptr->size = size;

  long * v = malloc(size * sizeof(long));

  for(long i=0; i < size; i++) {
    memcpy(&v[i], &fm_stack->stack[stack_pointer - size + 1 + i], sizeof(long));
  }

  ptr->v = v;

  heap[heap_counter] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long new_closure(long code_pointer, long global_pointer) {
  long heap_ptr = heap_counter;
  closure * ptr = malloc(sizeof(closure));

  ptr->type = CLOSURE_T;
  ptr->code_pointer = code_pointer;
  ptr->global_pointer = global_pointer;

  heap[heap_counter] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long new_function(long code_pointer, long argument_pointer, long global_pointer) {
  func * ptr = malloc(sizeof(func));
  long heap_ptr = heap_counter;

  ptr->type = FUNCTION_T;
  ptr->code_pointer = code_pointer;
  ptr->argument_pointer = argument_pointer;
  ptr->global_pointer = global_pointer;

  heap[heap_counter] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long new_nil() {
  list * ptr = malloc(sizeof(list));
  long heap_ptr = heap_counter;

  ptr->type = NIL_T;

  heap[heap_counter] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long new_cons() {
  list * ptr = malloc(sizeof(list));
  long heap_ptr = heap_counter;

  ptr->type = CONS_T;
  ptr->head = stack_entry(stack_pointer);
  ptr->tail = stack_entry(stack_pointer + 1);

  heap[heap_counter] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long new_string_from_data(char * program, long location) {
  string * ptr = malloc(sizeof(string));
  long heap_ptr = heap_counter;
  long length = *(long *)&program[location];
  char * chars = malloc(length);
  memcpy(chars, &program[location + 8], length);

  ptr->length = length;
  ptr->chars = chars;

  heap[heap_ptr] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long _fopen() {
  long heap_ptr = heap_counter;
  FILE * file = fopen(((string *)heap[stack_entry(stack_pointer)])->chars, ((string *)heap[stack_entry(stack_pointer - 1)])->chars);

  heap[heap_ptr] = (char *)file;
  heap_counter++;

  return heap_ptr;
}

long _fputs() {
  return fputs(((string *)heap[stack_entry(stack_pointer)])->chars, (FILE *)heap[stack_entry(stack_pointer - 1)]);
}

long _fgets() {
  long length = *(long *)&fm_stack->stack[stack_pointer];
  char * chars = malloc(length + 1);

  fgets(chars, length + 1, (FILE *)heap[stack_entry(stack_pointer - 1)]);

  string * ptr = malloc(sizeof(string));
  long heap_ptr = heap_counter;

  ptr->length = length;
  ptr->chars = chars;

  heap[heap_ptr] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long append_strings() {
  char * string1 = ((string *)heap[stack_entry(stack_pointer)])->chars;
  long string1_length = ((string *)heap[stack_entry(stack_pointer)])->length;
  char * string2 = ((string *)heap[stack_entry(stack_pointer - 1)])->chars;
  long string2_length = ((string *)heap[stack_entry(stack_pointer - 1)])->length;
  long length = string1_length + string2_length;

  char * chars = malloc(length);
  strcpy(chars, string1);
  strcat(chars, string2);

  string * ptr = malloc(sizeof(string));
  long heap_ptr = heap_counter;

  ptr->length = length;
  ptr->chars = chars;

  heap[heap_ptr] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

long float_64_to_string(double value, long decimal_points) {
  string * ptr = malloc(sizeof(string));
  long heap_ptr = heap_counter;
  long abs_value = (long)fabs(value);

  int int_length = 0;

  while (abs_value > 0) {
    int_length++;
    abs_value = abs_value / 10;
  }

  int length = decimal_points + int_length;

  char * chars = malloc(length);

  gcvt(value, length, chars);

  ptr->length = length;
  ptr->chars = chars;

  heap[heap_ptr] = (char *)ptr;
  heap_counter++;

  return heap_ptr;
}

void inc_program_counter() {
  program_counter = program_counter + 9;
}

void popenv() {
  // replace old global pointer
  global_pointer = stack_entry(stack_pointer - 3);
  // jump to post-function-application instruction
  program_counter = stack_entry(frame_pointer);
  // release stack frame after placing return value on eventual top-of-stack
  set_stack_entry(frame_pointer - 3, stack_entry(stack_pointer));
  stack_pointer = frame_pointer - 3;
  // replace old stack entrypoint
  function_entry_pointer = stack_entry(frame_pointer - 1);
  // replace old frame pointer
  frame_pointer = stack_entry(frame_pointer - 2);
}

void apply() {
  if (((func *)heap[stack_entry(stack_pointer)])->type != FUNCTION_T) {
    fprintf(stderr, "fm: apply invoked put top of stack is not a function object. Actual object: %c\n", ((char *)heap[stack_entry(stack_pointer)])[0]);
    exit(3);
  }

  long func_pointer = stack_entry(stack_pointer);
  long arg_pointer = ((func *)heap[func_pointer])->argument_pointer;

  for(int i=0; i < ((vector *)heap[arg_pointer])->size; i++) {
    set_stack_entry(stack_pointer + i, ((vector *)heap[arg_pointer])->v[i]);
  }

  program_counter = ((func *)heap[func_pointer])->code_pointer;

  // set the global pointer for access to free variables
  global_pointer = ((func *)heap[func_pointer])->global_pointer;

  // reset the stack pointer to where the last function is.
  // we're also setting function_entry_pointer. in the function, now at program_counter, any reference x in 
  // pushloc x will now be relative to function_entry_pointer.
  stack_pointer = function_entry_pointer = stack_pointer + ((vector *)heap[arg_pointer])->size - 1;
}

void allocate_dummy_references(long size) {
  for(int i=0; i < size; i++) {
    heap[heap_counter] = malloc(0);
    set_stack_entry(stack_pointer + 1 + i, heap_counter);
    heap_counter++;
  }

  stack_pointer = stack_pointer + size;
}

void debug_machine(char * program) {
  fprintf(stderr, "--------------------\n");
  char * instruction = fm_ins_binary_to_name(program[program_counter]);
  fprintf(stderr, "current instruction: %s %ld\n\n", instruction, *(signed long *)&program[program_counter + 1]);

  if (stack_pointer != -1) {

    fprintf(stderr, "stack:\n");

    for(long i=0; i < (stack_pointer + 1); i++) {
      fprintf(stderr, "%lu | %ld\n", stack_pointer - i, stack_entry(stack_pointer - i));
    }

    fprintf(stderr, "\n");
  }

  fprintf(stderr, "registers:\n");
  fprintf(stderr, "program_counter / 9: %ld\n", program_counter / 9);
  fprintf(stderr, "stack_pointer: %ld\n", stack_pointer);
  fprintf(stderr, "function_entry_pointer: %ld\n", function_entry_pointer);
  fprintf(stderr, "fp: %ld\n", frame_pointer);
  fprintf(stderr, "global_pointer: %ld\n", global_pointer);
  fprintf(stderr, "heap_counter: %lu\n", heap_counter);
  fprintf(stderr, "mem alloc'd for stack: %lu\n", fm_stack->size);
  fprintf(stderr, "--------------------\n");

  fprintf(stderr, "\n");
}

long stack_entry(long idx) {
  return fm_stack->stack[idx];
}

double stack_entry_as_float_64(long idx) {
  return *(double *)&(fm_stack->stack[idx]);
}

void set_stack_entry(long idx, long value) {
  fm_stack->stack[idx] = value;
}

int main(int argc, char **argv) {
  FILE * f = fopen(argv[1], "rb");

  fseek(f, 0L, SEEK_END);
  int filesize = ftell(f);
  fseek(f, 0L, SEEK_SET);

  char program[filesize];

  fread(program, 1, filesize, f);

  long * arg;
  long * basic_value_pointer;
  double float_64_val;
  long stack_pointer_frame_pointer_diff;
  vector * vector_pointer;
  list * list_pointer;

  int debug = (argc > 2) && !(strncmp(argv[2], "--debug", 7));

  init_stack();

  for(;;) {
    if (debug) {
      debug_machine(program);
    }

    switch(program[program_counter]) {
      case HALT:
        return stack_entry(stack_pointer);
      case LOADC:
        arg = (signed long *)&program[program_counter + 1];
        ensure_enough_stack(1);
        stack_pointer++;
        set_stack_entry(stack_pointer, *arg);
        inc_program_counter();
        break;
      case ADD:
        set_stack_entry(stack_pointer - 1, stack_entry(stack_pointer - 1) + stack_entry(stack_pointer));
        stack_pointer--;
        inc_program_counter();
        break;
      case SUB:
        set_stack_entry(stack_pointer - 1, stack_entry(stack_pointer - 1) - stack_entry(stack_pointer));
        stack_pointer--;
        inc_program_counter();
        break;
      case MUL:
        set_stack_entry(stack_pointer - 1, stack_entry(stack_pointer - 1) * stack_entry(stack_pointer));
        stack_pointer--;
        inc_program_counter();
        break;
      case DIV:
        set_stack_entry(stack_pointer - 1, stack_entry(stack_pointer - 1) / stack_entry(stack_pointer));
        stack_pointer--;
        inc_program_counter();
        break;
      case MKBASIC:
        set_stack_entry(stack_pointer, new_basic(stack_entry(stack_pointer)));
        inc_program_counter();
        break;
      case GETBASIC:
        if (((basic *)heap[stack_entry(stack_pointer)])->type != BASIC_T) {
          fprintf(stderr, "fm: getbasic invoked but a basic pointer is not at the top of the stack. %c\n", ((basic *)heap[stack_pointer])->type);
          exit(1);
        }

        set_stack_entry(stack_pointer, ((basic *)heap[stack_entry(stack_pointer)])->value);
        inc_program_counter();
        break;
      case LEQ:
        set_stack_entry(stack_pointer - 1, stack_entry(stack_pointer - 1) <= stack_entry(stack_pointer));
        stack_pointer--;
        inc_program_counter();
        break;
      case NEG:
        set_stack_entry(stack_pointer, -stack_entry(stack_pointer));
        inc_program_counter();
        break;
      case JUMP:
        arg = (long *)&program[program_counter + 1];
        program_counter = *arg;
        break;
      case JUMPZ:
        if (stack_entry(stack_pointer) == 0) {
          arg = (long *)&program[program_counter + 1];
          program_counter = *arg;
        } else {
          inc_program_counter();
        }
        stack_pointer--;
        break;
      case PUSHLOC:
        arg = (signed long *)&program[program_counter + 1];
        ensure_enough_stack(1);
        set_stack_entry(stack_pointer + 1, stack_entry(function_entry_pointer + *arg));
        stack_pointer++;
        inc_program_counter();
        break;
      case MKVEC:
        arg = (long *)&program[program_counter + 1];
        ensure_enough_stack(1 - *arg);
        set_stack_entry(stack_pointer + 1 - *arg, new_vector(*arg));
        stack_pointer = stack_pointer + 1 - *arg;
        inc_program_counter();
        break;
      case PUSHGLOB:
        if (((vector *)heap[global_pointer])->type != VECTOR_T) {
          fprintf(stderr, "fm: pushglob invoked but global pointer does not point to a vector.\n");
          exit(1);
        }

        arg = (long *)&program[program_counter + 1];
        ensure_enough_stack(1);
        set_stack_entry(stack_pointer + 1, ((vector *)heap[global_pointer])->v[*arg]);
        stack_pointer++;
        inc_program_counter();
        break;
      case MKCLOS:
        arg = (long *)&program[program_counter + 1];
        set_stack_entry(stack_pointer, new_closure(*arg, stack_entry(stack_pointer)));
        inc_program_counter();
        break;
      case EVAL:
        if (((closure *)heap[stack_entry(stack_pointer)])->type == CLOSURE_T) {
          ensure_enough_stack(3);
          // allocate stack frame
          set_stack_entry(stack_pointer + 1, global_pointer);
          set_stack_entry(stack_pointer + 2, frame_pointer);
          set_stack_entry(stack_pointer + 3, program_counter + 9);
          // set new global pointer from closure
          global_pointer = ((closure *)heap[stack_entry(stack_pointer)])->global_pointer;
          // jump to code pointer from closure
          program_counter = ((closure *)heap[stack_entry(stack_pointer)])->code_pointer;
          frame_pointer = stack_pointer = stack_pointer + 3;
        } else {
          inc_program_counter();
        }

        break;
      case UPDATE:
        global_pointer = stack_entry(frame_pointer - 2);
        // frame_pointer is where the old program_counter was
        program_counter = stack_entry(frame_pointer);

        // put value given by closure application to top of stack
        set_stack_entry(frame_pointer - 2, stack_entry(stack_pointer));
        // replace old stack pointer;
        stack_pointer = frame_pointer - 2;

        // rejuvinate old frame pointer
        frame_pointer = stack_entry(frame_pointer - 1);

        // replace old closure with value produced by the closure application
        heap[stack_entry(stack_pointer - 1)] = heap[stack_entry(stack_pointer)];
        // pop closure from the stack
        stack_pointer--;
        break;
      case SLIDE:
        arg = (long *)&program[program_counter + 1];
        set_stack_entry(stack_pointer - *arg, stack_entry(stack_pointer));
        stack_pointer = stack_pointer - *arg;
        inc_program_counter();
        break;
      case MKFUNVAL:
        arg = (long *)&program[program_counter + 1];
        set_stack_entry(stack_pointer, new_function(*arg, new_vector(0), stack_entry(stack_pointer)));
        inc_program_counter();
        break;
      case TARG:
        arg = (long *)&program[program_counter + 1];
        stack_pointer_frame_pointer_diff = stack_pointer - frame_pointer;

        // are there a number of arguments pushed to the stack less than the
        // arity of the function?
        if (stack_pointer_frame_pointer_diff < *arg) {
          set_stack_entry(frame_pointer + 1, new_function(program_counter, new_vector(stack_pointer_frame_pointer_diff), global_pointer));
          stack_pointer = frame_pointer + 1;
          popenv();
        } else {
          inc_program_counter();
        }

        break;
      case RETURN:
        arg = (long *)&program[program_counter + 1];

        if (stack_pointer - frame_pointer - 1 <= *arg) {
          popenv();
        } else {
          // Handle oversupply of arguments.
          //
          // Slide value at top of stack down by the number of arguments
          // expected by current function.
          set_stack_entry(stack_pointer - *arg, stack_entry(stack_pointer));
          stack_pointer = stack_pointer - *arg;
          // Apply next function in chain to remaining arguments.
          apply();
        }

        break;
      case MARK:
        arg = (long *)&program[program_counter + 1];
        ensure_enough_stack(4);
        set_stack_entry(stack_pointer + 1, global_pointer);
        set_stack_entry(stack_pointer + 2, frame_pointer);
        set_stack_entry(stack_pointer + 3, function_entry_pointer);
        set_stack_entry(stack_pointer + 4, *arg);
        frame_pointer = stack_pointer = stack_pointer + 4;
        inc_program_counter();
        break;
      case APPLY:
        apply();
        break;
      case ALLOC:
        arg = (long *)&program[program_counter + 1];
        allocate_dummy_references(*arg);
        inc_program_counter();
        break;
      case REWRITE:
        arg = (long *)&program[program_counter + 1];
        heap[stack_entry(stack_pointer - *arg)] = heap[stack_entry(stack_pointer)];
        stack_pointer--;
        inc_program_counter();
        break;
      case COPYGLOB:
        ensure_enough_stack(1);
        set_stack_entry(stack_pointer + 1, global_pointer);
        stack_pointer++;
        inc_program_counter();
        break;
      case GETVEC:
        arg = (long *)&program[program_counter + 1];

        vector_pointer = ((vector *)heap[stack_entry(stack_pointer)]);

        if (vector_pointer->type != VECTOR_T) {
          fprintf(stderr, "fm: getvec invoked, but stack_entry(stack_pointer) does not point to vector object.\n");
          exit(1);
        }

        if (vector_pointer->size != *arg) {
          fprintf(stderr, "fm: getvec invoked, but vector at stack_entry(stack_pointer) does not equal %lu.\n", *arg);
          exit(1);
        }

        ensure_enough_stack(*arg - 1);

        for (int i=0; i < *arg; i++) {
          set_stack_entry(stack_pointer + i, vector_pointer->v[i]);
        }

        stack_pointer = stack_pointer + *arg - 1;

        inc_program_counter();
        break;
      case NIL:
        ensure_enough_stack(1);
        stack_pointer++;
        set_stack_entry(stack_pointer, new_nil());
        inc_program_counter();
        break;
      case CONS:
        stack_pointer--;
        set_stack_entry(stack_pointer, new_cons());
        inc_program_counter();
        break;
      case TLIST:
        arg = (long *)&program[program_counter + 1];
        list_pointer = (list *)heap[stack_entry(stack_pointer)];

        switch (list_pointer->type) {
          case 'N':
            stack_pointer--;
            inc_program_counter();
            break;
          case 'S':
            ensure_enough_stack(1);
            set_stack_entry(stack_pointer, list_pointer->head);
            stack_pointer++;
            set_stack_entry(stack_pointer, list_pointer->tail);
            program_counter = *arg;
            break;
          default:
            fprintf(stderr, "fm: tlist invoked but list not at top of stack\n");
            exit(3);
        }

        break;
      case SLIDE2:
        arg = (long *)&program[program_counter + 1];

        for (int i=0; i < (*arg + 1); i++) {
          set_stack_entry(function_entry_pointer + i, stack_entry(stack_pointer - *arg + i));
        }

        stack_pointer = stack_pointer - *arg;

        inc_program_counter();
        break;
      case LOADD:
        arg = (long *)&program[program_counter + 1];
        ensure_enough_stack(1);
        stack_pointer++;
        set_stack_entry(stack_pointer, *(long *)&program[*arg]);
        inc_program_counter();
        break;
      case MKSTRINGD:
        arg = (long *)&program[program_counter + 1];
        ensure_enough_stack(1);
        stack_pointer++;
        set_stack_entry(stack_pointer, new_string_from_data((char *)&program, *arg));
        inc_program_counter();
        break;
      case FOPEN:
        set_stack_entry(stack_pointer - 1, _fopen());
        stack_pointer--;
        inc_program_counter();
        break;
      case FPUTS:
        set_stack_entry(stack_pointer - 1, _fputs());
        stack_pointer--;
        inc_program_counter();
        break;
      case FGETS:
        set_stack_entry(stack_pointer - 1, _fgets());
        stack_pointer--;
        inc_program_counter();
        break;
      case APPENDS:
        set_stack_entry(stack_pointer - 1, append_strings());
        stack_pointer--;
        inc_program_counter();
        break;
      case F64TOS:
        arg = (long *)&program[program_counter + 1];
        set_stack_entry(stack_pointer, float_64_to_string(stack_entry_as_float_64(stack_pointer), *arg));
        inc_program_counter();
        break;
      case F64ADD:
        float_64_val = stack_entry_as_float_64(stack_pointer - 1) + stack_entry_as_float_64(stack_pointer);
        set_stack_entry(stack_pointer - 1, *(signed long *)&float_64_val);
        stack_pointer--;
        inc_program_counter();
        break;
      case F64SUB:
        float_64_val = stack_entry_as_float_64(stack_pointer - 1) - stack_entry_as_float_64(stack_pointer);
        set_stack_entry(stack_pointer - 1, *(signed long *)&float_64_val);
        stack_pointer--;
        inc_program_counter();
        break;
      case F64MUL:
        float_64_val = stack_entry_as_float_64(stack_pointer - 1) * stack_entry_as_float_64(stack_pointer);
        set_stack_entry(stack_pointer - 1, *(signed long *)&float_64_val);
        stack_pointer--;
        inc_program_counter();
        break;
      case F64DIV:
        float_64_val = stack_entry_as_float_64(stack_pointer - 1) / stack_entry_as_float_64(stack_pointer);
        set_stack_entry(stack_pointer - 1, *(signed long *)&float_64_val);
        stack_pointer--;
        inc_program_counter();
        break;
      case EOF:
        fprintf(stderr, "fm: No halt before EOF.\n");
        exit(1);
      default:
        fprintf(stderr, "fm: unknown instruction: %02x\n", program[program_counter]);
        exit(2);
    }
  }

  return 0;
}
