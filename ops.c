#include<string.h>
#include "ops.h"

const char alu4_table[][8] = {
  "add", "sub",
  "shl", "shr", "sar",
  "and", "or", "xor",
  "cmpult", "cmpule",
  "cmpne", "cmpeq",
  "cmplt", "cmple"
};

const int alu4_code[] = {
   0,  1,
   2,  3, 4,
   5,  6, 7,
  22, 23,
  24, 25,
  26, 27
};
const char misc1_table[][8] = {
  "jr"
};
const int misc1_code[] = {
  12
};

const char misc2_table[][8] = {
  "ldl", "jl"
};
const int misc2_code[] = {
  2, 11
};

const char misc3_table[][8] = {
  "ldh", "st", "stb", "ld", "ldb", "bne", "beq"
};

const int misc3_code[] = {
  3, 6, 7, 8, 9, 13, 15
};

int find_alu4(char *op) {
  unsigned i;
  for(i = 0; i < sizeof(alu4_code)/sizeof(int); ++i) {
    if(strcmp(op, alu4_table[i]) == 0)
      return alu4_code[i];
  }
  return -1;
}

int find_misc1(char *op) {
  unsigned i;
  for(i = 0; i < sizeof(misc1_code)/sizeof(int); ++i) {
    if(strcmp(op, misc1_table[i]) == 0)
      return misc1_code[i];
  }
  return -1;
}

int find_misc2(char *op) {
  unsigned i;
  for(i = 0; i < sizeof(misc2_code)/sizeof(int); ++i) {
    if(strcmp(op, misc2_table[i]) == 0)
      return misc2_code[i];
  }
  return -1;
}

int find_misc3(char *op) {
  unsigned i;
  for(i = 0; i < sizeof(misc3_code)/sizeof(int); ++i) {
    if(strcmp(op, misc3_table[i]) == 0)
      return misc3_code[i];
  }
  return -1;
}
