    .text
    .p2align 2
    .p2align 4,,15
    .globl wait_pcycles
    .type    wait_pcycles, @function
wait_pcycles:
    { R0 = ASR(R0,#2) }
    { LOOP0(.startLoop,R0) }
    .falign
.startLoop:
    { V0.b = VADD(V3.b,V3.b)}
    { V1.b = VADD(V4.b,V4.b)}:endloop0
    { JUMPR R31 }
    .size    wait_pcycles, .-wait_pcycles

