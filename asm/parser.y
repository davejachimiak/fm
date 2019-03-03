%{
#include "parser.h"
#include "lexer.h"
#include "../vm/instructions.h"
#include <time.h>
#include <sys/ioctl.h>

void yyerror();
int do_debug = 0;
unsigned long column;

void debug(char * msg) {
  if (do_debug) {
    fprintf(stderr, "[fm-asm][DEBUG] %s\n", msg);
  }
}

InstructionUnits * instruction_units;
unsigned long program_length;

void inc_program_length() {
  program_length = program_length + 9;
}

void init_instruction_units() {
  instruction_units = malloc(sizeof(InstructionUnits));
  instruction_units->instruction = NULL;
  instruction_units->next = NULL;
}

struct InstructionUnit * init_instruction_unit(char * instruction) {
  struct InstructionUnit * instruction_unit = malloc(sizeof(struct InstructionUnit));

  instruction_unit->next = NULL;
  instruction_unit->instruction = instruction;

  return instruction_unit;
}

char * anonymous_symbol() {
  static int length = 16;
  char * symbol = malloc(length);
  char c;
  uint8_t n;

  for (int i=0; i < (length - 1); i++) {
    n = (uint8_t)rand();
    n = (n & 63);
    n = (n + 64) & 127;
    symbol[i] = (char)n;
  }

  symbol[length - 1] = '\0';

  return symbol;
}

typedef struct SymbolLookupEntry {
  char * symbol;
  enum { DATA_SECTION, LABELS } kind;
  struct SymbolLookupEntry * next;
  int lineno;
} SymbolLookup;

SymbolLookup * symbol_lookup;

void init_symbol_lookup() {
  symbol_lookup = malloc(sizeof(SymbolLookup));

  symbol_lookup->symbol = NULL;
  symbol_lookup->next = NULL;
}

struct SymbolLookupEntry * init_symbol_lookup_entry() {
  struct SymbolLookupEntry * entry = malloc(sizeof(struct SymbolLookupEntry));

  entry->next = NULL;

  return entry;
}

void assert_symbol_inequality(struct SymbolLookupEntry * entry, char * symbol) {
  if (strcmp(entry->symbol, symbol) == 0) {
    fprintf(stderr, "[fm-asm] error: duplicate symbol '%s' defined in lines %i and %i\n", symbol, entry->lineno, yylineno - 1);
    exit(1);
  }
}

void append_to_symbol_lookup(int kind, char * symbol) {
  if (symbol_lookup->symbol == NULL) {
    symbol_lookup->symbol = symbol;
    symbol_lookup->kind = kind;
    symbol_lookup->lineno = yylineno - 1;
    return;
  }

  assert_symbol_inequality(symbol_lookup, symbol);

  SymbolLookup * entry = symbol_lookup;

  while (entry->next != NULL) {
    entry = entry->next;
    assert_symbol_inequality(entry, symbol);
  }

  struct SymbolLookupEntry * new_entry = init_symbol_lookup_entry();

  new_entry->symbol = symbol;
  new_entry->kind = kind;
  new_entry->kind = kind;
  new_entry->lineno = yylineno - 1;

  entry->next = new_entry;
}

typedef struct DataSectionEntry {
  char * symbol;
  union {
    char * string;
    long integer;
    double _float;
  } value;
  enum { STRING_DE, INTEGER_DE, FLOAT_DE } type;
  unsigned long length;
  struct DataSectionEntry * next;
} DataSection;

DataSection * data_section;

void init_data_section() {
  data_section = malloc(sizeof(DataSection));
  data_section->symbol = NULL;
  data_section->next = NULL;
}

struct DataSectionEntry * create_integer_data_section_entry(char * symbol, long value) {
  struct DataSectionEntry * entry = malloc(sizeof(struct DataSectionEntry));

  entry->symbol = symbol;
  entry->value.integer = value;
  entry->type = INTEGER_DE;
  entry->next = NULL;

  return entry;
}

struct DataSectionEntry * create_float_data_section_entry(char * symbol, double value) {
  struct DataSectionEntry * entry = malloc(sizeof(struct DataSectionEntry));

  entry->symbol = symbol;
  entry->value._float = value;
  entry->type = FLOAT_DE;
  entry->next = NULL;

  return entry;
}

struct DataSectionEntry * create_string_data_section_entry(char * symbol, char * value) {
  struct DataSectionEntry * entry = malloc(sizeof(struct DataSectionEntry));
  unsigned long length = strlen(value);

  entry->symbol = symbol;
  entry->value.string = value;
  entry->type = STRING_DE;
  entry->length = length;
  entry->next = NULL;

  return entry;
}

void append_to_data_section(struct DataSectionEntry * new_entry) {
  if (data_section->symbol == NULL) {
    data_section = new_entry;
    append_to_symbol_lookup(DATA_SECTION, data_section->symbol);
    return;
  }

  struct DataSectionEntry * entry = data_section;

  while (entry->next != NULL) {
    entry = entry->next;
  }

  entry->next = new_entry;

  append_to_symbol_lookup(DATA_SECTION, new_entry->symbol);
}

unsigned long get_symbol_address_from_data_section(char * symbol) {
  struct DataSectionEntry * entry = data_section;
  unsigned long relative_symbol_address = 0;

  while (strcmp(entry->symbol, symbol) != 0) {
    if (entry->type == STRING_DE) {
      relative_symbol_address = relative_symbol_address + entry->length + 8;
    } else {
      relative_symbol_address = relative_symbol_address + 8;
    }

    entry = entry->next;

    if (entry == NULL) {
      fprintf(stderr, "[fm-asm] ERROR: symbol not defined: %s\n", symbol);

      exit(1);
    }
  }

  unsigned long symbol_address = relative_symbol_address + program_length;

  return symbol_address;
}

typedef struct Label {
  char * symbol;
  unsigned long address;
  struct Label * next;
} Label;

Label * labels;

Label * init_label() {
  struct Label * label = malloc(sizeof(Label));

  label->next = NULL;

  return label;
}

void append_to_labels(char * symbol, unsigned long address) {
  if (labels->symbol == NULL) {
    labels->symbol = symbol;
    labels->address = address;
    append_to_symbol_lookup(LABELS, symbol);
    return;
  }

  Label * label = labels;

  while (label->next != NULL) {
    label = label->next;
  }

  struct Label * new_label = init_label();

  new_label->symbol = symbol;
  new_label->address = address;

  label->next = new_label;

  append_to_symbol_lookup(LABELS, symbol);
}

unsigned long get_symbol_address_from_label(char * symbol) {
  struct Label * entry = labels;

  while (strcmp(entry->symbol, symbol) != 0) {
    entry = entry->next;
  }

  return entry->address;
}

unsigned long get_symbol_address(char * symbol) {
  struct SymbolLookupEntry * entry = symbol_lookup;

  while (strcmp(entry->symbol, symbol) != 0) {
    entry = entry->next;

    if (entry == NULL) {
      fprintf(stderr, "[fm-asm] Error: undefined symbol \"%s\"\n", symbol);
      exit(1);
    }
  }

  if (entry->kind == DATA_SECTION) {
    return get_symbol_address_from_data_section(symbol);
  } else {
    return get_symbol_address_from_label(symbol);
  }
}


%}

%defines "./asm/parser.h"

%code requires {
const char * filename;

typedef struct InstructionUnit {
  char * instruction;
  char * label;
  enum { NULL_ARG, SYMBOL_ARG, INTEGER_ARG, FLOAT_ARG } arg_type;
  union {
    char * symbol;
    signed long integer;
    double _double;
  } argument;
  struct InstructionUnit * next;
} InstructionUnits;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

}

%union {
  char * string;
  double _double;
  signed long integer;
  struct InstructionUnit * instruction_units;
}

%token NEWLINE SEPERATOR;
%token <string> SYMBOL STRING;
%token <_double> FLOAT;
%token <integer> INTEGER;
%type <instruction_units> instruction_units instruction_unit bare_instruction_unit;

%%
input
: instruction_units
| instruction_units data_section
| %empty
;

data_section
: SEPERATOR
| SEPERATOR NEWLINE assignments
;

assignments
: assignment
| assignments assignment
;

assignment
: SYMBOL '=' STRING NEWLINE {
  struct DataSectionEntry * entry = create_string_data_section_entry($1, $3);
  append_to_data_section(entry);
}
| SYMBOL '=' INTEGER NEWLINE {
  struct DataSectionEntry * entry = create_integer_data_section_entry($1, $3);
  append_to_data_section(entry);
}
| SYMBOL '=' FLOAT NEWLINE {
  struct DataSectionEntry * entry = create_float_data_section_entry($1, $3);
  append_to_data_section(entry);
}
| NEWLINE
;

instruction_units
: instruction_unit {
  instruction_units = $1;
  inc_program_length();
}
| instruction_units instruction_unit {
  $1->next = $2;

  inc_program_length();

  $$ = $2;
}
;

instruction_unit
: SYMBOL ':' bare_instruction_unit {
  $3->label = $1;

  append_to_labels($1, program_length);

  $$ = $3;
}
| bare_instruction_unit {
  $$ = $1;
}
;

bare_instruction_unit
: SYMBOL NEWLINE {
  struct InstructionUnit * instruction_unit = init_instruction_unit($1);
  instruction_unit->arg_type = NULL_ARG;

  $$ = instruction_unit;
}
| SYMBOL INTEGER NEWLINE {
  struct InstructionUnit * instruction_unit = init_instruction_unit($1);

  instruction_unit->argument.integer = $2;
  instruction_unit->arg_type = INTEGER_ARG;

  $$ = instruction_unit;
}
| SYMBOL FLOAT NEWLINE {
  struct InstructionUnit * instruction_unit = init_instruction_unit($1);

  instruction_unit->argument._double = $2;
  instruction_unit->arg_type = FLOAT_ARG;

  $$ = instruction_unit;
}
| SYMBOL STRING NEWLINE {
  struct InstructionUnit * instruction_unit = init_instruction_unit($1);

  char * symbol = anonymous_symbol();
  struct DataSectionEntry * entry = create_string_data_section_entry(symbol, $2);

  append_to_data_section(entry);

  instruction_unit->argument.symbol = symbol;
  instruction_unit->arg_type = SYMBOL_ARG;

  $$ = instruction_unit;
}
| SYMBOL SYMBOL NEWLINE {
  struct InstructionUnit * instruction_unit = init_instruction_unit($1);

  instruction_unit->argument.symbol = $2;
  instruction_unit->arg_type = SYMBOL_ARG;

  $$ = instruction_unit;
}
;
%%

void yyerror(const char * message) {
  FILE * file = fopen(filename, "r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int count = 0;
  int length = strlen(yytext);
  int start_column = column - length + 1;

  while ((read = getline(&line, &len, file)) != -1) {
    count++;

    if (count <= yylineno) {
      fprintf(stderr, ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, line);
    }
  }

  fprintf(stderr, ANSI_COLOR_RED "%*s^\n" ANSI_COLOR_RESET, start_column - 1, "");

  char * token;

  if ((strcmp(yytext, "\n") == 0) || (strcmp(yytext, "\r") == 0)){
    token = "[newline]";
  } else {
    token = yytext;
  }

  fprintf(stderr, ANSI_COLOR_RED "[fm-asm %i:%i] Parse error. (token: %s)\n" ANSI_COLOR_RESET, yylineno, start_column, token);

  exit(1);
}

void write_instruction(struct InstructionUnit * instruction_unit) {
  char * argument;
  char code = fm_ins_name_to_binary(instruction_unit->instruction);
  unsigned long symbol_address;

  fwrite(&code, 1, 1, stdout);

  switch (instruction_unit->arg_type) {
  case INTEGER_ARG:
    fwrite(&instruction_unit->argument.integer, 8, 1, stdout);
    break;
  case FLOAT_ARG:
    fwrite(&instruction_unit->argument._double, 8, 1, stdout);
    break;
  case SYMBOL_ARG:
    symbol_address = get_symbol_address(instruction_unit->argument.symbol);
    fwrite(&symbol_address, 8, 1, stdout);
    break;
  case NULL_ARG:
    fwrite("\x00\x00\x00\x00\x00\x00\x00\x00", 8, 1, stdout);
    break;
  default:
    fprintf(stderr, "[fm-asm] ERROR: uncaught argument type.\n");
    exit(1);
    break;
  }
}

void write_data_section_entry(struct DataSectionEntry * entry) {
  switch (entry->type) {
    case STRING_DE:
      fwrite(&entry->length, 8, 1, stdout);
      fwrite(entry->value.string, entry->length, 1, stdout);
      break;
    case INTEGER_DE:
      fwrite(&entry->value.integer, 8, 1, stdout);
      break;
    case FLOAT_DE:
      fwrite(&entry->value._float, 8, 1, stdout);
      break;
  }
}

void init_labels() {
  labels = malloc(sizeof(Label));
  labels->symbol = NULL;
  labels->next = NULL;
}

int main(int argc, const char * argv[]) {
  do_debug = getenv("DEBUG") != NULL;

  if (argc <= 1) {
    fprintf(stderr, "fm-asm <filename>\n");
    exit(1);
  }

  srand((unsigned int)time(NULL));

  init_symbol_lookup();
  init_data_section();
  init_labels();

  filename = argv[1];
  yyin = fopen(filename, "r");
  yyparse();

  struct InstructionUnit * instruction_unit = instruction_units;

  if (instruction_unit->instruction != NULL) {
    write_instruction(instruction_unit);
  } else {
    return 0;
  }

  while (instruction_unit->next != NULL) {
    instruction_unit = instruction_unit->next;
    write_instruction(instruction_unit);
  }

  struct DataSectionEntry * entry = data_section;

  if (entry->symbol != NULL) {
    write_data_section_entry(entry);
  } else {
    return 0;
  }

  while (entry->next != NULL) {
    entry = entry->next;
    write_data_section_entry(entry);
  }

  return 0;
}
