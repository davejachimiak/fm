#include <stdio.h>
#include <stdlib.h>
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
  default:
    fprintf(stderr, "fm (fm_ins_binary_to_name): unknown instruction: %02x\n", ins);
    exit(4);
    break;
  }

  return "";
}
