# Don't use any macro except halt.
fib:
  add r2, r0, r0, 0
  add r3, r0, r0, 1
  add r4, r0, r0, 0
LOOP_BEGIN:
  cmplt r5, r4, r1, 0
  beq r5, r0, LOOP_END
  add r5, r2, r3, 0
  add r2, r3, r0, 0
  add r3, r5, r0, 0
  add r4, r4, r0, 1
  jl  r29, LOOP_BEGIN
LOOP_END:
  add r1, r2, r0, 0
  jl  r29, RETURN_L

.global main
main:
  add r1, r0, r0, 11
  jl  r29, fib
RETURN_L:
  ldh r29, r0, 0x8000
  st  r1, r29, 0x1000
  halt
