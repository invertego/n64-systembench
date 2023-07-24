#include <stdbool.h>
#include <stdint.h>
#include <memory.h>
#include <inttypes.h>
#include <libdragon.h>

#include "pipeline.h"
#include "registers.h"
#include "decode.h"
#include "disasm.h"

//#define pdebugf debugf
#define pdebugf(...)

static bool branch_taken(int opc) {
    // assumes register is always zero
    switch (opc) {
    case I_BEQ: return true;
    case I_BNE: return false;
    case I_BLEZ: return true;
    case I_BGTZ: return false;
    case I_BLTZ: return false;
    case I_BGEZ: return true;
    case I_BLTZAL: return false;
    case I_BGEZAL: return true;
    }
    return true;
}

static uint32_t imem_read(pipeline_t* p) {
    uint32_t pc = p->if_in.pc & 0xfff;
    if (pc >= p->pc_start && pc < p->pc_end) {
        return p->imem_ptr[pc >> 2];
    }
    return 0x0000000D; // BREAK
}

static void stage_if(pipeline_t* p) {
    pdebugf("if %" PRIx32 "\n", p->if_in.pc);
    bool issue_su = false;
    bool issue_vu = false;

    if (p->delay_slot) {
        pdebugf("if branch bubble %03lX\n", p->if_in.pc);
        if (p->if_in.pc & 4) {
            p->if_in.single_issue = true;
        }
        goto end;
    }

    bool single_issue = p->if_in.single_issue;
    uint32_t end = (p->if_in.pc + 8);// & ~7;
    decoded_t d_prev;
    decode_instr(&d_prev, 0);
    while (p->if_in.pc != end) {
        uint32_t inst = imem_read(p);
        decoded_t d;
        decode_instr(&d, inst);
        if (!IS_VU(d.opc)) {
            if (issue_su) break;
            if (d_prev.v_out & (d.v_in | d.v_out)) {
                pdebugf("if su dep %x %x\n", d_prev.v_out, d.v_in);
                break;
            }
            issue_su = true;
            //debugf("su %03lX %08lX\n", p->if_in.pc, inst);
            p->rd_in.su.inst = inst;
            p->rd_in.su.pc = p->if_in.pc;
            p->rd_in.su.bubble = false;
        } else {
            if (issue_vu) break;
            if (d_prev.v_out & (d.v_in | d.v_out)) {
                pdebugf("if vu dep %x %x\n", d_prev.v_out, d.v_in);
                break;
            }
            #if 0
            // experimenting with looser dependency check
            uint32_t vd = 1u << ((inst >>  6) & 0x1f);
            uint32_t vs = 1u << ((inst >> 11) & 0x1f);
            uint32_t vt = 1u << ((inst >> 16) & 0x1f);
            if ((d_prev.v_out & (vt | vs | vd)) && d_prev.opc == I_LTV && d.opc == I_VNOP) {
                break;
            }
            #endif
            issue_vu = true;
            //debugf("vu %03lX %08lX\n", p->if_in.pc, inst);
            p->rd_in.vu.inst = inst;
            p->rd_in.vu.pc = p->if_in.pc;
            p->rd_in.vu.bubble = false;
        }
        if (d.opc == I_BREAK) break;
        p->if_in.pc += 4;
        p->if_in.single_issue = IS_BRANCH(d.opc);
        if (single_issue || p->if_in.single_issue) break;
        d_prev = d;
    }

end:
    if (!issue_su) {
        //debugf("su <none>\n");
        p->rd_in.su.inst = 0;
        p->rd_in.su.pc = 0;
        p->rd_in.su.bubble = true;
    }
    if (!issue_vu) {
        //debugf("vu <none>\n");
        p->rd_in.vu.inst = 0;
        p->rd_in.vu.pc = 0;
        p->rd_in.vu.bubble = true;
    }
    //debugf("====\n");
}

static bool is_hazard(pipeline_t* p, int r_in) {
    if (r_in > reg_zero && p->lock[r_in] &&
        // hazard can be bypassed only if *all* writers are bypassable
        !(p->df_acc_in.su.r_out != r_in && p->wb_in.su.r_out == r_in && IS_BYPASSABLE(p->wb_in.su.opc)) &&
        !(p->df_acc_in.su.r_out == r_in && p->wb_in.su.r_out != r_in && IS_BYPASSABLE(p->df_acc_in.su.opc)) &&
        !(p->df_acc_in.su.r_out == r_in && p->wb_in.su.r_out == r_in && IS_BYPASSABLE(p->df_acc_in.su.opc) && IS_BYPASSABLE(p->wb_in.su.opc))
        ) {
        pdebugf("su rd hazard %d\n", r_in);
        return true;
    }
    return false;
}

static bool is_hazard_v(pipeline_t* p, int v_in) {
    if (v_in && (
        /*p->vlock[v_in] ||*/
        (p->df_acc_in.su.v_out & v_in) ||
        (p->wb_in.su.v_out & v_in) ||
        (p->wb_out.su.v_out & v_in) ||
        (p->df_acc_in.vu.v_out & v_in) ||
        (p->wb_in.vu.v_out & v_in) ||
        (p->wb_out.vu.v_out & v_in)
        )) {
        return true;
    }
    return false;
}

static bool stage_rd(pipeline_t* p) {
    // SU
    pdebugf("su rd %" PRIx32 " %08" PRIx32 "\n", p->rd_in.su.pc, p->rd_in.su.inst);
    decoded_t d_su;
    decode_instr(&d_su, p->rd_in.su.inst);

    // VU
    pdebugf("vu rd %" PRIx32 " %08" PRIx32 "\n", p->rd_in.vu.pc, p->rd_in.vu.inst);
    decoded_t d_vu;
    decode_instr(&d_vu, p->rd_in.vu.inst);

    // SU
    if (is_hazard(p, d_su.r_in) || is_hazard(p, d_su.r_in2)) {
        goto bubble;
    }
    if (is_hazard_v(p, d_su.v_in)) {
        pdebugf("su rd hazard v %x\n", d_su.v_in);
        goto bubble;
    }

    // VU
    if (is_hazard_v(p, d_vu.v_in)) {
        pdebugf("vu rd hazard v %x\n", d_vu.v_in);
        goto bubble;
    }

    // SU
    if (d_su.r_out > reg_zero) {
        pdebugf("su rd lock %d\n", d_su.r_out);
        ++p->lock[d_su.r_out];
    }
    if (d_su.v_out) {
        pdebugf("su rd lock v %x\n", d_su.v_out);
        for (int i = 0; i < 32; i++) {
            if (d_su.v_out & (1 << i)) ++p->vlock[i];
        }
    }

    // VU
    if (d_vu.v_out) {
        pdebugf("vu rd lock v %x\n", d_vu.v_out);
        for (int i = 0; i < 32; i++) {
            if (d_vu.v_out & (1 << i)) ++p->vlock[i];
        }
    }

    // SU
    p->ex_mul_in.su.pc = p->rd_in.su.pc;
    p->ex_mul_in.su.r_out = d_su.r_out;
    p->ex_mul_in.su.v_out = d_su.v_out;
    p->ex_mul_in.su.opc = d_su.opc;
    p->ex_mul_in.su.bubble = p->rd_in.su.bubble;

    // VU
    p->ex_mul_in.vu.pc = p->rd_in.vu.pc;
    p->ex_mul_in.vu.v_out = d_vu.v_out;
    p->ex_mul_in.vu.opc = d_vu.opc;
    p->ex_mul_in.vu.bubble = p->rd_in.vu.bubble;

    return true;

bubble:
    // SU
    p->ex_mul_in.su.pc = 0;
    p->ex_mul_in.su.r_out = reg_invalid;
    p->ex_mul_in.su.v_out = 0;
    p->ex_mul_in.su.opc = 0;
    p->ex_mul_in.su.bubble = true;

    // VU
    p->ex_mul_in.vu.pc = 0;
    p->ex_mul_in.vu.v_out = 0;
    p->ex_mul_in.vu.opc = 0;
    p->ex_mul_in.vu.bubble = true;

    return false;
}

static bool stage_ex_mul(pipeline_t* p) {
    // SU
    pdebugf("su ex %" PRIx32 "\n", p->ex_mul_in.su.pc);

    // VU
    pdebugf("vu mul %" PRIx32 "\n", p->ex_mul_in.vu.pc);

    // SU
    if (IS_LOAD(p->wb_out.su.opc) && IS_STORE(p->ex_mul_in.su.opc)) {
        pdebugf("su ex load store interlock\n");

        // SU
        p->df_acc_in.su.r_out = reg_invalid;
        p->df_acc_in.su.v_out = 0;
        p->df_acc_in.su.opc = 0;

        // VU
        p->df_acc_in.vu.v_out = 0;
        p->df_acc_in.vu.opc = 0;

        return false;
    }

    // SU / VU
    if (IS_BRANCH(p->ex_mul_in.su.opc) && branch_taken(p->ex_mul_in.su.opc)) {
        pdebugf("su ex enter delay slot\n");
        p->delay_slot = true;
    } else if (p->delay_slot && (!p->ex_mul_in.su.bubble || !p->ex_mul_in.vu.bubble)) {
        if (!p->ex_mul_in.su.bubble) {
            pdebugf("su ex leave delay slot\n");
        } else if (!p->ex_mul_in.vu.bubble) {
            pdebugf("vu mul leave delay slot\n");
        }
        p->delay_slot = false;
    }

    // SU
    p->df_acc_in.su.r_out = p->ex_mul_in.su.r_out;
    p->df_acc_in.su.v_out = p->ex_mul_in.su.v_out;
    p->df_acc_in.su.opc = p->ex_mul_in.su.opc;

    // VU
    p->df_acc_in.vu.v_out = p->ex_mul_in.vu.v_out;
    p->df_acc_in.vu.opc = p->ex_mul_in.vu.opc;

    return true;
}

static void stage_df_acc(pipeline_t* p) {
    // SU
    pdebugf("su df %d\n", p->df_acc_in.su.opc);

    //VU
    pdebugf("vu acc %d\n", p->df_acc_in.vu.opc);
    
    // SU
    p->wb_in.su.r_out = p->df_acc_in.su.r_out;
    p->wb_in.su.v_out = p->df_acc_in.su.v_out;
    p->wb_in.su.opc = p->df_acc_in.su.opc;

    //VU
    p->wb_in.vu.v_out = p->df_acc_in.vu.v_out;
    p->wb_in.vu.opc = p->df_acc_in.vu.opc;
}

static void stage_wb(pipeline_t* p) {
    // SU
    pdebugf("su wb %d\n", p->wb_in.su.opc);

    //VU
    pdebugf("vu wb %d\n", p->wb_in.vu.opc);

    // SU
    if (p->wb_in.su.r_out > reg_zero) {
        pdebugf("su wb unlock %d\n", p->wb_in.su.r_out);
        --p->lock[p->wb_in.su.r_out];
    }
    if (p->wb_in.su.v_out) {
        pdebugf("su wb unlock v %x\n", p->wb_in.su.v_out);
        for (int i = 0; i < 32; i++) {
            if (p->wb_in.su.v_out & (1 << i)) --p->vlock[i];
        }
    }
    if (p->wb_in.su.opc == I_BREAK) {
        p->broken = true;
    }

    //VU
    if (p->wb_in.vu.v_out) {
        pdebugf("vu wb unlock v %x\n", p->wb_in.vu.v_out);
        for (int i = 0; i < 32; i++) {
            if (p->wb_in.vu.v_out & (1 << i)) --p->vlock[i];
        }
    }

    // SU
    p->wb_out.su.v_out = p->wb_in.su.v_out;
    p->wb_out.su.opc = p->wb_in.su.opc;

    //VU
    p->wb_out.vu.v_out = p->wb_in.vu.v_out;
    p->wb_out.vu.opc = p->wb_in.vu.opc;
}

void pipeline_init(pipeline_t* p) {
    memset(p, 0, sizeof(*p));
    p->ex_mul_in.su.r_out = reg_invalid;
    p->df_acc_in.su.r_out = reg_invalid;
    p->wb_in.su.r_out = reg_invalid;
}

void pipeline_step(pipeline_t* p) {
    stage_wb(p);
    stage_df_acc(p);
    if (stage_ex_mul(p)) {
        if (stage_rd(p)) {
            stage_if(p);
        }
    }
}
