#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <libdragon.h>

#include "generate.h"
#include "pipeline.h"
#include "registers.h"
#include "decode.h"
#include "disasm.h"
#include "emit.h"

// Fair and fast random generation (using xorshift32, with explicit seed)
static uint32_t rand_state = 1;
static uint32_t rand(void) {
    uint32_t x = rand_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 5;
    return rand_state = x;
}

// RANDN(n): generate a random number from 0 to n-1
#define RANDN(n) ({ \
    __builtin_constant_p((n)) ? \
        (rand()%(n)) : \
        (uint32_t)(((uint64_t)rand() * (n)) >> 32); \
})

static uint32_t* generate_instr(uint32_t* code_start, uint32_t* code_ptr, bool* delay, int flags) {
    uint32_t rt;
    uint32_t rs;
    uint32_t rd;
    uint32_t imm;
    uint32_t sa;
    uint32_t base;
    uint32_t offset;
    uint32_t target;
    uint32_t e, de, vsar_e;
    uint32_t vt, vd, vs, vc;
    bool link;
    bool in_delay = *delay;
    *delay = false;

    #define npc (code_ptr - code_start + 1)
    #define rand_r() RANDN(4)
    #define rand_v() RANDN(4)
retry:
    int group = 0;
    if (flags == GEN_SU) group = RANDN(7);
    else if (flags == GEN_VU) group = 7;
    else if (flags == (GEN_SU | GEN_VU)) group = RANDN(11);
    switch (group) {
    // nop
    case 0: emit_nop(); break;
    // jump/branch
    case 1:
        if (in_delay) goto retry;
        *delay = true;
        link = true;
        switch(RANDN(12)) {
        case 0: emit_j(npc + 1); break;
        case 1: emit_jal(npc + 1); break;
        // TODO: other registers
        case 2: emit_beq(reg_zero, reg_zero, 1); break;
        case 3: emit_bne(reg_zero, reg_zero, 1); break;
        case 4: emit_blez(reg_zero, 1); break;
        case 5: emit_bgtz(reg_zero, 1); break;
        case 6: emit_bltz(reg_zero, 1); break;
        case 7: emit_bgez(reg_zero, 1); break;
        case 8: emit_bltzal(reg_zero, 1); break;
        case 9: emit_bgezal(reg_zero, 1); break;
        case 10: link = false;
        case 11:
            rd = rand_r();
            target = (npc + 3) << 2;
            emit_lui(reg_at, target >> 16);
            emit_ori(reg_at, reg_at, target);
            if (!link)
                emit_jr(reg_at);
            else
                emit_jalr(rd, reg_at);
            break;
        } break;
    // alu
    case 2:
        rt = rand_r();
        rs = rand_r();
        rd = rand_r();
        sa = imm = RANDN(3);
        switch(RANDN(24)) {
        case 0: emit_addi(rt, rs, imm); break;
        case 1: emit_addiu(rt, rs, imm); break;
        case 2: emit_slti(rt, rs, imm); break;
        case 3: emit_sltiu(rt, rs, imm); break;
        case 4: emit_andi(rt, rs, imm); break;
        case 5: emit_ori(rt, rs, imm); break;
        case 6: emit_xori(rt, rs, imm); break;
        case 7: emit_lui(rt, imm); break;
        case 8: emit_sll(rd, rt, sa); break;
        case 9: emit_srl(rd, rt, sa); break;
        case 10: emit_sra(rd, rt, sa); break;
        case 11: emit_sllv(rd, rt, rs); break;
        case 12: emit_srlv(rd, rt, rs); break;
        case 13: emit_srav(rd, rt, rs); break;
        case 14: emit_add(rd, rs, rt); break;
        case 15: emit_addu(rd, rs, rt); break;
        case 16: emit_sub(rd, rs, rt); break;
        case 17: emit_subu(rd, rs, rt); break;
        case 18: emit_and(rd, rs, rt); break;
        case 19: emit_or(rd, rs, rt); break;
        case 20: emit_xor(rd, rs, rt); break;
        case 21: emit_nor(rd, rs, rt); break;
        case 22: emit_slt(rd, rs, rt); break;
        case 23: emit_sltu(rd, rs, rt); break;
        } break;
    // load/store
    case 3:
        rt = rand_r();
        base = rand_r();
        offset = RANDN(3);
        switch (RANDN(9)) {
        case 0: emit_lb(rt, offset, base); break;
        case 1: emit_lh(rt, offset, base); break;
        case 2: emit_lw(rt, offset, base); break;
        case 3: emit_lbu(rt, offset, base); break;
        case 4: emit_lhu(rt, offset, base); break;
        case 5: emit_lwu(rt, offset, base); break;
        case 6: emit_sb(rt, offset, base); break;
        case 7: emit_sh(rt, offset, base); break;
        case 8: emit_sw(rt, offset, base); break;
        } break;
    // scc
    case 4:
        rt = rand_r();
        switch (RANDN(2)) {
        case 0: emit_mfc0(rt, reg_zero); break;
        case 1: emit_mtc0(rt, reg_zero); break;
        } break;
    // vu
    case 5:
        rt = rand_r();
        rd = rand_v();
        vc = RANDN(4);
        e = 0;
        switch (RANDN(4)) {
        case 0: emit_mfc2(rt, rd, e); break;
        case 1: emit_cfc2(rt, vc);    break;
        case 2: emit_mtc2(rt, rd, e); break;
        case 3: emit_ctc2(rt, vc);    break;
        } break;
    // vu load/store
    case 6:
        vt = rand_v();
        e = 0;
        base = rand_r();
        offset = RANDN(3);
        switch (RANDN(23)) {
        case 0: emit_lbv(vt, e, offset, base); break;
        case 1: emit_lsv(vt, e, offset, base); break;
        case 2: emit_llv(vt, e, offset, base); break;
        case 3: emit_ldv(vt, e, offset, base); break;
        case 4: emit_lqv(vt, e, offset, base); break;
        case 5: emit_lrv(vt, e, offset, base); break;
        case 6: emit_lpv(vt, e, offset, base); break;
        case 7: emit_luv(vt, e, offset, base); break;
        case 8: emit_lhv(vt, e, offset, base); break;
        case 9: emit_lfv(vt, e, offset, base); break;
        case 10: emit_ltv(vt, e, offset, base); break;
        case 11: emit_sbv(vt, e, offset, base); break;
        case 12: emit_ssv(vt, e, offset, base); break;
        case 13: emit_slv(vt, e, offset, base); break;
        case 14: emit_sdv(vt, e, offset, base); break;
        case 15: emit_sqv(vt, e, offset, base); break;
        case 16: emit_srv(vt, e, offset, base); break;
        case 17: emit_spv(vt, e, offset, base); break;
        case 18: emit_suv(vt, e, offset, base); break;
        case 19: emit_shv(vt, e, offset, base); break;
        case 20: emit_sfv(vt, e, offset, base); break;
        case 21: emit_swv(vt, e, offset, base); break;
        case 22: emit_stv(vt, e, offset, base); break;
        } break;
    // vu alu
    case 7:
    case 8:
    case 9:
    case 10:
        e = 0;
        de = rand_v();
        vsar_e = 8 + RANDN(4);
        vd = rand_v();
        vs = rand_v();
        vt = rand_v();
        switch (RANDN(44)) {
        case 0: emit_vmulf(vd, vs, vt, e); break;
        case 1: emit_vmulu(vd, vs, vt, e); break;
        case 2: emit_vrndp(vd, vs, vt, e); break;
        case 3: emit_vmulq(vd, vs, vt, e); break;
        case 4: emit_vmudl(vd, vs, vt, e); break;
        case 5: emit_vmudm(vd, vs, vt, e); break;
        case 6: emit_vmudn(vd, vs, vt, e); break;
        case 7: emit_vmudh(vd, vs, vt, e); break;
        case 8: emit_vmacf(vd, vs, vt, e); break;
        case 9: emit_vmacu(vd, vs, vt, e); break;
        case 10: emit_vrndn(vd, vs, vt, e); break;
        case 11: emit_vmacq(vd, vs, vt, e); break;
        case 12: emit_vmadl(vd, vs, vt, e); break;
        case 13: emit_vmadm(vd, vs, vt, e); break;
        case 14: emit_vmadn(vd, vs, vt, e); break;
        case 15: emit_vmadh(vd, vs, vt, e); break;
        case 16: emit_vadd(vd, vs, vt, e); break;
        case 17: emit_vsub(vd, vs, vt, e); break;
        case 18: emit_vabs(vd, vs, vt, e); break;
        case 19: emit_vaddc(vd, vs, vt, e); break;
        case 20: emit_vsubc(vd, vs, vt, e); break;
        case 21: emit_vsar(vd, vs, vt, vsar_e); break;
        case 22: emit_vlt(vd, vs, vt, e); break;
        case 23: emit_veq(vd, vs, vt, e); break;
        case 24: emit_vne(vd, vs, vt, e); break;
        case 25: emit_vge(vd, vs, vt, e); break;
        case 26: emit_vcl(vd, vs, vt, e); break;
        case 27: emit_vch(vd, vs, vt, e); break;
        case 28: emit_vcr(vd, vs, vt, e); break;
        case 29: emit_vmrg(vd, vs, vt, e); break;
        case 30: emit_vand(vd, vs, vt, e); break;
        case 31: emit_vnand(vd, vs, vt, e); break;
        case 32: emit_vor(vd, vs, vt, e); break;
        case 33: emit_vnor(vd, vs, vt, e); break;
        case 34: emit_vxor(vd, vs, vt, e); break;
        case 35: emit_vnxor(vd, vs, vt, e); break;
        case 36: emit_vrcp(vd, de, vt, e); break;
        case 37: emit_vrcpl(vd, de, vt, e); break;
        case 38: emit_vrcph(vd, de, vt, e); break;
        case 39: emit_vmov(vd, de, vt, e); break;
        case 40: emit_vrsq(vd, de, vt, e); break;
        case 41: emit_vrsql(vd, de, vt, e); break;
        case 42: emit_vrsqh(vd, de, vt, e); break;
        case 43: emit_vnop(vd, de, vt, e); break;
        } break;
    }
    #undef npc
    return code_ptr;
}

uint32_t generate_code(void* code, int inst_count, int* cycle_estimate, int flags) {
    //rand_state = TICKS_READ();

    uint32_t* code_start = code;
    uint32_t* code_ptr = code_start;
#define npc (code_ptr - code_start + 1)

    emit_addiu(reg_s2, reg_zero, DP_WSTATUS_RESET_CLOCK_COUNTER);
    emit_mtc0(reg_s2, COP0_DP_STATUS);
    emit_mfc0(reg_s0, COP0_DP_CLOCK);
    emit_nop();
    emit_nop();
    emit_nop();

    uint32_t pc_start = (code_ptr - code_start) << 2;

#if 1
    bool delay = false;
    decoded_t d_prev, d_curr;
    decode_instr(&d_prev, 0);
    for (int i = 0; i < inst_count; i++) {
        int next_flags = flags;
        // always begin and end with an SU instruction so prolog/epilog nops (which aren't measured) aren't dual issued
        if (i == 0 || i == inst_count - 1) next_flags &= GEN_SU;
        bool delay_out = delay;
        uint32_t* next_ptr = generate_instr(code_start, code_ptr, &delay_out, next_flags);
        decode_instr(&d_curr, next_ptr[-1]);
        code_ptr = next_ptr;
        d_prev = d_curr;
        delay = delay_out;
    }
    if (delay) emit_nop();
#endif

    uint32_t pc_end = (code_ptr - code_start) << 2;

    emit_nop();
    emit_nop();
    emit_mfc0(reg_s1, COP0_DP_CLOCK);
    emit_sw(reg_s0, 0, reg_zero);
    emit_sw(reg_s1, 4, reg_zero);
    emit_break();
    emit_nop();

#if 1
    char buf[64];
    for (uint32_t pc = pc_start; pc != pc_end; pc += 4) {
        uint32_t* c = code_start + (pc >> 2);
        uint32_t op = *c;
        disasm_instr(buf, pc, op);
        debugf("  %03" PRIX32 " %08" PRIX32 " %s\n", pc, op, buf);
    }
#endif

    int cycles = 0;

#if 1
    pipeline_t pipeline;
    pipeline_init(&pipeline);
    pipeline.imem_ptr = code_start;
    pipeline.pc_start = pc_start;
    pipeline.pc_end = pc_end;
    pipeline.if_in.pc = pc_start;
    for (cycles = 0; !pipeline.broken && cycles < 2000; ++cycles) {
        pipeline_step(&pipeline);
    }
    cycles -= 5; // remove 4 cycles for warm-up and 1 cycle for break
#endif

    *cycle_estimate = cycles;

    uint32_t code_size = (code_ptr - code_start) << 2;
    code_size = (code_size + 7) & ~7;
    return code_size;

#undef npc
}
