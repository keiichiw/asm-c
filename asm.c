#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "ops.h"
#include "vector.h"
#include "parse.h"

#define ERR(...)                                \
  do {                                          \
  fprintf(stderr, __VA_ARGS__);                 \
  exit(1);                                      \
  } while(0)

#define DEBUG 1

#define D(...)                                  \
  do {                                          \
  if (DEBUG)                                    \
    fprintf(stderr, __VA_ARGS__);               \
  } while(0)


#define START_ADDR 0

int g_ln = START_ADDR;

typedef struct {
  int   pc;
  buffer buf;
  int   pseudo;
} inst;

typedef struct {
  char *lbl;
  int addr;
} label;

Vector *g_lines;
Vector *g_labels;

void
init_globals()
{
  g_lines  = vec_create(100);
  g_labels = vec_create(20);
}

void
push_label(char *str, int addr)
{
  label *l = malloc(sizeof(label));
  D("label %s\n", str);
  l->lbl = malloc(strlen(str));
  strcpy(l->lbl, str);
  l->addr = addr;
  vec_push(g_labels, l);
}


int
assoc_label(char *str)
{
  int sz = vec_lengh(g_labels);
  int i;
  for (i = 0; i < sz; ++i) {
    label *l = vec_get(g_labels, i);
    if (strcmp(l->lbl, str) == 0) {
      return l->addr;
    }
  }
  ERR("label %s not found\n", str);
}


void set_inst(inst *ins, int pc, buffer *buf, int pseudo)
{
  ins->pc  = pc;
  ins->buf = *buf;
  ins->pseudo = pseudo;
}

void
push_inst(inst *ins)
{
  inst *dst = malloc(sizeof(inst));

  memcpy(dst, ins, sizeof(inst));
  vec_push(g_lines, dst);
}



bool
check_int_range(int i, int b)
{
  int x = 1 << (b - 1);
  return -x <= i && i < x;
}

unsigned
code_i(int op, char *rx, char *ra, char *rb, int imm, int tag)
{
  unsigned x = regnum(rx);
  unsigned a = regnum(ra);
  unsigned b = regnum(rb);
  unsigned c0, c1, c2, c3;
  if (! check_int_range(imm, 8)) {
    ERR("imm is too large %d\n", imm);
  }
  c0 = ((imm & 7) << 5) + tag;
  c1 = ((b & 7) << 5) + ((imm >> 3) & 31);
  c2 = ((x & 1) << 7) + (a << 2) + (b >> 3);
  c3 = (op << 4) + (x >> 1);
  return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
}

unsigned
code_m(int op, char *rx, char *ra, int prd, int d, int disp_mode)
{
  unsigned x = regnum(rx);
  unsigned a = regnum(ra);
  unsigned c0, c1, c2, c3;
  if (disp_mode == 0) {
    if (!(-0x8000 <= d && d <= 0xffff))
      ERR("imm too large %d\n", d);
  } else if (disp_mode == 1){
    if (!(check_int_range(d, 16)))
      ERR("disp too large %d\n", d);
  } else {
    if ((d & 3) != 0)
      ERR("disp must be a mult of 4\n");
    if (!(check_int_range(d, 18)))
      ERR("disp too large %d\n", d);
    d >>= 2;
  }
  c0 = d & 255;
  c1 = (d >> 8) & 255;
  c2 = ((x & 1) << 7) + (a << 2) + prd;
  c3 = (op << 4) + (x >> 1);

  return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;

}



unsigned
on_alu4(inst *ins, int tag)
{
  char operands[4][8];
  get_token(operands[0], &ins->buf);
  get_token(operands[1], &ins->buf);
  get_token(operands[2], &ins->buf);
  get_token(operands[3], &ins->buf);
  return code_i(0, operands[0], operands[1], operands[2],
         atoi(operands[3]), tag);
}



unsigned
calc_disp(int pc, int op, char *oprnd)
{

  int addr;

  if (oprnd[0] == '0') {
    addr = read_num(oprnd);
  } else {
    addr = assoc_label(oprnd);
  }

  if (op == 1) {
    return addr - pc - 4;
  } else {
    return addr;
  }

}

unsigned
on_misc0(inst *ins, int op, int prd, int dmode)
{
  return code_m(op, "r0", "r0", prd, 0, dmode);
}


unsigned
on_misc1(inst *ins, int op, int prd, int dmode)
{
  char operand[20];
  get_token(operand, &ins->buf);
  return code_m(op, operand, "r0", prd, 0, dmode);
}

unsigned
on_misc2(inst *ins, int op, int prd, int dmode, int dop)
{
  char operands[2][20];
  int disp;
  get_token(operands[0], &ins->buf);
  get_token(operands[1], &ins->buf);
  disp = calc_disp(ins->pc, dop, operands[1]);
  return code_m(op, operands[0], "r0", prd, disp, dmode);
}

unsigned
on_misc3(inst *ins, int op, int prd, int dmode, int dop)
{
  char operands[3][20];
  int disp;
  get_token(operands[0], &ins->buf);
  get_token(operands[1], &ins->buf);
  get_token(operands[2], &ins->buf);
  disp = calc_disp(ins->pc, dop, operands[2]);
  return code_m(op, operands[0], operands[1], prd, disp, dmode);
}

unsigned
on_jr(inst *ins)
{
  char operands[2][20];
  get_token(operands[0], &ins->buf);
  get_token(operands[1], &ins->buf);
  return code_m(5, operands[0], operands[1], 3, 0, 0);
}

int
disp_mode(char *token)
{
  if (strcmp(token, "ldl") == 0 ||
      strcmp(token, "ldh") == 0) {
    return 0;
  } else if (strcmp(token, "ldb") == 0 ||
             strcmp(token, "stb") == 0) {
    return 1;
  }
  return 2;
}

int
pred(char *token)
{
  return 0;
}

int
disp_op(char *op)
{
  if(strcmp(op, "jl") == 0  ||
     strcmp(op, "bne") == 0 ||
     strcmp(op, "beq") == 0 )
    return 1;
  else
    return 0;
}


unsigned
code(inst *ins)
{
  char token[20];
  int opcode;
  int dmode, prd, dop;
  get_token(token, &ins->buf);
  dmode = disp_mode(token);
  prd    = pred(token);
  dop = disp_op(token);
  if (strcmp(token, "halt") == 0) {
    // only allow halt macro
    return 0xffffffff;
  } else if ((opcode = find_alu4(token)) >= 0) {
    return on_alu4(ins, opcode);
  } else if ((opcode = find_misc0(token)) >= 0) {
    return on_misc0(ins, opcode, prd, dmode);
  } else if ((opcode = find_misc2(token)) >= 0) {
    return on_misc2(ins, opcode, prd, dmode, dop);
  } else if ((opcode = find_misc3(token)) >= 0) {
    return on_misc3(ins, opcode, prd, dmode, dop);
  } else if (strcmp(token, "jr") == 0) {
    return on_jr(ins);
  } else {
    D("TODO : %s\n", token);
    return -1;
  }

}



void
read_buffer(buffer *buf)
{
  int len = strlen(buf->str);
  buffer tbuf = *buf;
  char op[20];
  get_token(op, &tbuf);

  if (len <= 1) {
    // blank line
  } else if (buf->str[0] == '.') {

    if (strcmp(op, ".global") == 0) {
      // ignore
    } else if (strcmp(op, ".string") == 0) {
      inst ins;
      int len = strlen(buf->str) - 9;
      set_inst(&ins, g_ln, &tbuf, 1);
      push_inst(&ins);
      g_ln += len;
    } else if (strcmp(op, ".align") == 0) {
      char num[5];
      int n;
      get_token(num, buf);
      n = atoi(buf->str + buf->cur);
      D("n=%d\n", n);
      D("[%s]\n", buf->str + buf->cur);
      D("%s\n", buf->str + buf->cur);
      g_ln = ((g_ln + (n-1)) / n) * n;
    } else {
      D("unknown directive: %s\n", op);
    }

  } else if (buf->str[len - 1] == ':') {
    // Label
    buf->str[len - 1] = '\0';
    push_label(buf->str, g_ln);
  } else if (strcmp(op, "write") == 0) { // write macro
    inst ins;
    buffer mybf;
    char regstr[10];
    int reg;
    get_token(regstr, &tbuf);
    reg = regnum(regstr);
    mybf.cur = 0;
    strcpy(mybf.str, "ldh r29, r0, 0x8000");
    set_inst(&ins, g_ln, &mybf, 0);
    push_inst(&ins);
    g_ln += 4;
    strcpy(mybf.str, "st rN, r29, 0x1000");
    mybf.str[4] = '0' + reg;
    set_inst(&ins, g_ln, &mybf, 0);
    push_inst(&ins);
    g_ln += 4;
  } else if (strcmp(op, "br") == 0) { // br macro
    inst ins;
    buffer mybf;
    char str[30] = "jl r29, ";
    char lbl[10];
    int len;
    get_token(lbl, &tbuf);
    len = strlen(lbl);
    memcpy(str + strlen(str), lbl, len);
    D("%s\n", str);
    mybf.cur = 0;
    strcpy(mybf.str, str);
    set_inst(&ins, g_ln, &mybf, 0);
    push_inst(&ins);
    g_ln += 4;
  } else {
    inst ins;
    set_inst(&ins, g_ln, buf, 0);
    push_inst(&ins);
    g_ln += 4;
  }
}

int
on_string(inst *ins, FILE *fp)
{
  char str[30];
  int len;
  get_line(str, &ins->buf);
  len = strlen(str);
  str[len-1]='\0';
  fwrite(str+1, 1, len-1, fp);
  return len - 1;
}

void
emit_insts(FILE *fp)
{
  int i;
  int len = vec_lengh(g_lines);
  int pc = START_ADDR;
  int fs = g_ln - pc;
  D("sz = %d\n", fs);
  fwrite(&fs, sizeof(unsigned), 1, fp);

  for(i = 0; i < len; ++i) {
    inst *ins = vec_get(g_lines, i);
    if (pc < ins->pc) {
      int j, n = ins->pc - pc;
      char z = 0;
      for(j = 0; j < n; ++j) {
        fwrite(&z, 1, 1, fp);
      }
      pc += n;
      D("align: %d\n", n);
    }
    if (ins->pseudo == 1) { // .string
      D("0x%06x 0x%06x string\n", pc, ins->pc);
      pc += on_string(ins, fp);
    } else {
      unsigned b = code(ins);
      D("0x%06x 0x%06x [%08x]\n", pc, ins->pc, b);
      fwrite(&b, sizeof(b), 1, fp);
      pc += 4;
    }
  }

}

void
br_main()
{
  buffer buf;

  buf.cur = 0;
  strcpy(buf.str, "ldl r29, main");
  read_buffer(&buf);

  buf.cur = 0;
  strcpy(buf.str, "jr r29, r29");
  read_buffer(&buf);
}

int
main(int argc, char *argv[])
{
  FILE *fp;
  buffer buf;

  if (argc != 2) {
    ERR("Usage: ./as [filename]\n");
    return 0;
  }

  if ((fp = fopen(argv[1],"r")) == NULL) {
    ERR("file open error!\n");
    exit(1);
  }

  init_globals();

  br_main();

  while(fgets(buf.str, 256, fp) != NULL) {
    buf.cur = 0;
    rm_comment(buf.str);
    rstrip(buf.str);
    read_buffer(&buf);
  }

  fclose(fp);

  fp = fopen("a.out", "wb");
  emit_insts(fp);
  fclose(fp);

  return 0;
}
