#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instructions.h"

char * fm_ins_binary_to_name(char ins) {
  switch (ins) {
  case HALT:
    return HALT_NAME;
    break;
  case LOADC:
    return LOADC_NAME;
    break;
  case ADD:
    return ADD_NAME;
    break;
  case SUB:
    return SUB_NAME;
    break;
  case MUL:
    return MUL_NAME;
    break;
  case DIV:
    return DIV_NAME;
    break;
  case MKBASIC:
    return MKBASIC_NAME;
    break;
  case GETBASIC:
    return GETBASIC_NAME;
    break;
  case LEQ:
    return LEQ_NAME;
    break;
  case NEG:
    return NEG_NAME;
    break;
  case JUMP:
    return JUMP_NAME;
    break;
  case JUMPZ:
    return JUMPZ_NAME;
    break;
  case PUSHLOC:
    return PUSHLOC_NAME;
    break;
  case MKVEC:
    return MKVEC_NAME;
    break;
  case PUSHGLOB:
    return PUSHGLOB_NAME;
    break;
  case MKCLOS:
    return MKCLOS_NAME;
    break;
  case EVAL:
    return EVAL_NAME;
    break;
  case UPDATE:
    return UPDATE_NAME;
    break;
  case SLIDE:
    return SLIDE_NAME;
    break;
  case MKFUNVAL:
    return MKFUNVAL_NAME;
    break;
  case TARG:
    return TARG_NAME;
    break;
  case RETURN:
    return RETURN_NAME;
    break;
  case MARK:
    return MARK_NAME;
    break;
  case APPLY:
    return APPLY_NAME;
    break;
  case ALLOC:
    return ALLOC_NAME;
    break;
  case REWRITE:
    return REWRITE_NAME;
    break;
  case COPYGLOB:
    return COPYGLOB_NAME;
    break;
  case GETVEC:
    return GETVEC_NAME;
    break;
  case NIL:
    return NIL_NAME;
    break;
  case CONS:
    return CONS_NAME;
    break;
  case TLIST:
    return TLIST_NAME;
    break;
  case SLIDE2:
    return SLIDE2_NAME;
    break;
  case LOADD:
    return LOADD_NAME;
    break;
  case MKSTRINGD:
    return MKSTRINGD_NAME;
    break;
  case FOPEN:
    return FOPEN_NAME;
    break;
  case FPUTS:
    return FPUTS_NAME;
    break;
  case FGETS:
    return FGETS_NAME;
    break;
  case APPENDS:
    return APPENDS_NAME;
    break;
  case F64TOS:
    return F64TOS_NAME;
    break;
  case F64ADD:
    return F64ADD_NAME;
    break;
  case F64SUB:
    return F64SUB_NAME;
    break;
  case F64MUL:
    return F64MUL_NAME;
    break;
  case F64DIV:
    return F64DIV_NAME;
    break;
  case ITOS:
    return ITOS_NAME;
    break;
  default:
    fprintf(stderr, "fm (fm_ins_binary_to_name): unknown instruction: %02x\n", ins);
    exit(4);
    break;
  }

  return "";
}

char fm_ins_name_to_binary(char * name) {
  if (strcmp(name, HALT_NAME) == 0) { return HALT; }
  if (strcmp(name, LOADC_NAME) == 0) { return LOADC; }
  if (strcmp(name, ADD_NAME) == 0) { return ADD; }
  if (strcmp(name, SUB_NAME) == 0) { return SUB; }
  if (strcmp(name, MUL_NAME) == 0) { return MUL; }
  if (strcmp(name, DIV_NAME) == 0) { return DIV; }
  if (strcmp(name, MKBASIC_NAME) == 0) { return MKBASIC; }
  if (strcmp(name, GETBASIC_NAME) == 0) { return GETBASIC; }
  if (strcmp(name, LEQ_NAME) == 0) { return LEQ; }
  if (strcmp(name, NEG_NAME) == 0) { return NEG; }
  if (strcmp(name, JUMP_NAME) == 0) { return JUMP; }
  if (strcmp(name, JUMPZ_NAME) == 0) { return JUMPZ; }
  if (strcmp(name, PUSHLOC_NAME) == 0) { return PUSHLOC; }
  if (strcmp(name, MKVEC_NAME) == 0) { return MKVEC; }
  if (strcmp(name, PUSHGLOB_NAME) == 0) { return PUSHGLOB; }
  if (strcmp(name, MKCLOS_NAME) == 0) { return MKCLOS; }
  if (strcmp(name, EVAL_NAME) == 0) { return EVAL; }
  if (strcmp(name, UPDATE_NAME) == 0) { return UPDATE; }
  if (strcmp(name, SLIDE_NAME) == 0) { return SLIDE; }
  if (strcmp(name, MKFUNVAL_NAME) == 0) { return MKFUNVAL; }
  if (strcmp(name, TARG_NAME) == 0) { return TARG; }
  if (strcmp(name, RETURN_NAME) == 0) { return RETURN; }
  if (strcmp(name, MARK_NAME) == 0) { return MARK; }
  if (strcmp(name, APPLY_NAME) == 0) { return APPLY; }
  if (strcmp(name, ALLOC_NAME) == 0) { return ALLOC; }
  if (strcmp(name, REWRITE_NAME) == 0) { return REWRITE; }
  if (strcmp(name, COPYGLOB_NAME) == 0) { return COPYGLOB; }
  if (strcmp(name, GETVEC_NAME) == 0) { return GETVEC; }
  if (strcmp(name, NIL_NAME) == 0) { return NIL; }
  if (strcmp(name, CONS_NAME) == 0) { return CONS; }
  if (strcmp(name, TLIST_NAME) == 0) { return TLIST; }
  if (strcmp(name, SLIDE2_NAME) == 0) { return SLIDE2; }
  if (strcmp(name, LOADD_NAME) == 0) { return LOADD; }
  if (strcmp(name, MKSTRINGD_NAME) == 0) { return MKSTRINGD; }
  if (strcmp(name, FOPEN_NAME) == 0) { return FOPEN; }
  if (strcmp(name, FPUTS_NAME) == 0) { return FPUTS; }
  if (strcmp(name, FGETS_NAME) == 0) { return FGETS; }
  if (strcmp(name, APPENDS_NAME) == 0) { return APPENDS; }
  if (strcmp(name, F64TOS_NAME) == 0) { return F64TOS; }
  if (strcmp(name, F64ADD_NAME) == 0) { return F64ADD; }
  if (strcmp(name, F64SUB_NAME) == 0) { return F64SUB; }
  if (strcmp(name, F64MUL_NAME) == 0) { return F64MUL; }
  if (strcmp(name, F64DIV_NAME) == 0) { return F64DIV; }
  if (strcmp(name, ITOS_NAME) == 0) { return ITOS; }

  fprintf(stderr, "fm (fm_ins_name_to_binary): unknown instruction: %s\n", name);
  exit(4);

  return '\0';
}
