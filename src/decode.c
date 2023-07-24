#include <stdint.h>
#include <memory.h>

#include "decode.h"

static const int opcode_flags[I_COUNT] = {
    [I_SLL] = IF_BYPASS,
    [I_SRL] = IF_BYPASS,
    [I_SRA] = IF_BYPASS,
    [I_SLLV] = IF_BYPASS,
    [I_SRLV] = IF_BYPASS,
    [I_SRAV] = IF_BYPASS,
    [I_JR] = IF_BRANCH_UNCOND,
    [I_JALR] = IF_BYPASS | IF_BRANCH_UNCOND,
    [I_BREAK] = 0,
    [I_ADD] = IF_BYPASS,
    [I_ADDU] = IF_BYPASS,
    [I_SUB] = IF_BYPASS,
    [I_SUBU] = IF_BYPASS,
    [I_AND] = IF_BYPASS,
    [I_OR] = IF_BYPASS,
    [I_XOR] = IF_BYPASS,
    [I_NOR] = IF_BYPASS,
    [I_SLT] = IF_BYPASS,
    [I_SLTU] = IF_BYPASS,
    [I_BLTZ] = IF_BRANCH_COND,
    [I_BGEZ] = IF_BRANCH_COND,
    [I_BLTZAL] = IF_BYPASS | IF_BRANCH_COND,
    [I_BGEZAL] = IF_BYPASS | IF_BRANCH_COND,
    [I_J] = IF_BRANCH_UNCOND,
    [I_JAL] = IF_BYPASS | IF_BRANCH_UNCOND,
    [I_BEQ] = IF_BRANCH_COND,
    [I_BNE] = IF_BRANCH_COND,
    [I_BLEZ] = IF_BRANCH_COND,
    [I_BGTZ] = IF_BRANCH_COND,
    [I_ADDI] = IF_BYPASS,
    [I_ADDIU] = IF_BYPASS,
    [I_SLTI] = IF_BYPASS,
    [I_SLTIU] = IF_BYPASS,
    [I_ANDI] = IF_BYPASS,
    [I_ORI] = IF_BYPASS,
    [I_XORI] = IF_BYPASS,
    [I_LUI] = IF_BYPASS,
    [I_MFC0] = IF_LOADSTORE_COP,
    [I_MTC0] = IF_LOADSTORE_COP,
    [I_LB] = IF_LOAD_MEM,
    [I_LH] = IF_LOAD_MEM,
    [I_LW] = IF_LOAD_MEM,
    [I_LBU] = IF_LOAD_MEM,
    [I_LHU] = IF_LOAD_MEM,
    [I_LWU] = IF_LOAD_MEM,
    [I_SB] = IF_STORE_MEM,
    [I_SH] = IF_STORE_MEM,
    [I_SW] = IF_STORE_MEM,

    [I_MFC2] = IF_LOADSTORE_COP,
    [I_CFC2] = IF_LOADSTORE_COP,
    [I_MTC2] = IF_LOADSTORE_COP,
    [I_CTC2] = IF_LOADSTORE_COP,
    [I_VMULF] = IF_VU,
    [I_VMULU] = IF_VU,
    [I_VRNDP] = IF_VU,
    [I_VMULQ] = IF_VU,
    [I_VMUDL] = IF_VU,
    [I_VMUDM] = IF_VU,
    [I_VMUDN] = IF_VU,
    [I_VMUDH] = IF_VU,
    [I_VMACF] = IF_VU,
    [I_VMACU] = IF_VU,
    [I_VRNDN] = IF_VU,
    [I_VMACQ] = IF_VU,
    [I_VMADL] = IF_VU,
    [I_VMADM] = IF_VU,
    [I_VMADN] = IF_VU,
    [I_VMADH] = IF_VU,
    [I_VADD] = IF_VU,
    [I_VSUB] = IF_VU,
    [I_VABS] = IF_VU,
    [I_VADDC] = IF_VU,
    [I_VSUBC] = IF_VU,
    [I_VSAR] = IF_VU,
    [I_VLT] = IF_VU,
    [I_VEQ] = IF_VU,
    [I_VNE] = IF_VU,
    [I_VGE] = IF_VU,
    [I_VCL] = IF_VU,
    [I_VCH] = IF_VU,
    [I_VCR] = IF_VU,
    [I_VMRG] = IF_VU,
    [I_VAND] = IF_VU,
    [I_VNAND] = IF_VU,
    [I_VOR] = IF_VU,
    [I_VNOR] = IF_VU,
    [I_VXOR] = IF_VU,
    [I_VNXOR] = IF_VU,
    [I_VRCP] = IF_VU,
    [I_VRCPL] = IF_VU,
    [I_VRCPH] = IF_VU,
    [I_VMOV] = IF_VU,
    [I_VRSQ] = IF_VU,
    [I_VRSQL] = IF_VU,
    [I_VRSQH] = IF_VU,
    [I_VNOP] = IF_VU,
    [I_LBV] = IF_LOAD_MEM,
    [I_LSV] = IF_LOAD_MEM,
    [I_LLV] = IF_LOAD_MEM,
    [I_LDV] = IF_LOAD_MEM,
    [I_LQV] = IF_LOAD_MEM,
    [I_LRV] = IF_LOAD_MEM,
    [I_LPV] = IF_LOAD_MEM,
    [I_LUV] = IF_LOAD_MEM,
    [I_LHV] = IF_LOAD_MEM,
    [I_LFV] = IF_LOAD_MEM,
    [I_LTV] = IF_LOAD_MEM,
    [I_SBV] = IF_STORE_MEM,
    [I_SSV] = IF_STORE_MEM,
    [I_SLV] = IF_STORE_MEM,
    [I_SDV] = IF_STORE_MEM,
    [I_SQV] = IF_STORE_MEM,
    [I_SRV] = IF_STORE_MEM,
    [I_SPV] = IF_STORE_MEM,
    [I_SUV] = IF_STORE_MEM,
    [I_SHV] = IF_STORE_MEM,
    [I_SFV] = IF_STORE_MEM,
    [I_SWV] = IF_STORE_MEM,
    [I_STV] = IF_STORE_MEM,
};

int get_opc_flags(int opc) {
    if (opc < 0 || opc >= I_COUNT) return 0;
    return opcode_flags[opc];
}

void decode_instr(decoded_t* d_out, uint32_t op) {
    #define SA     (int)((op >>  6) & 31)
    #define RDn    (int)((op >> 11) & 31)
    #define RTn    (int)((op >> 16) & 31)
    #define RSn    (int)((op >> 21) & 31)
    #define VDn    ((op >>  6) & 31)
    #define VSn    ((op >> 11) & 31)
    #define VTn    ((op >> 16) & 31)
    #define IMMi7  ((int8_t)(op << 1) >> 1)
    #define IMMi16 (int16_t)(op)
    #define IMMu16 (uint16_t)(op)
    #define IMMu26 (op & 0x03ffffff)
    #define VMASK(n) (1 << (n))
    #define VBLOCK(n) (0xFF << ((n) & ~7))
    decoded_t d;

    memset(&d, 0xff, sizeof(d));
    d.v_in = 0;
    d.v_out = 0;
    d.vc_in = 0;
    d.vc_out = 0;

    switch (op >> 26) {
    case 0x00: // SPECIAL
        switch(op & 0x3f) {
        case 0x00: d.opc = I_SLL;  d.r_out = RDn; d.r_in = RTn;                d.imm = SA; break;
        case 0x02: d.opc = I_SRL;  d.r_out = RDn; d.r_in = RTn;                d.imm = SA; break;
        case 0x03: d.opc = I_SRA;  d.r_out = RDn; d.r_in = RTn;                d.imm = SA; break;
        case 0x04: d.opc = I_SLLV; d.r_out = RDn; d.r_in = RTn; d.r_in2 = RSn;             break;
        case 0x06: d.opc = I_SRLV; d.r_out = RDn; d.r_in = RTn; d.r_in2 = RSn;             break;
        case 0x07: d.opc = I_SRAV; d.r_out = RDn; d.r_in = RTn; d.r_in2 = RSn;             break;
        case 0x08: d.opc = I_JR;                  d.r_in = RSn;                            break;
        case 0x09: d.opc = I_JALR; d.r_out = RDn; d.r_in = RSn;                            break;
        case 0x0d: d.opc = I_BREAK;                                                        break;
        case 0x20: d.opc = I_ADD;  d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x21: d.opc = I_ADDU; d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x22: d.opc = I_SUB;  d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x23: d.opc = I_SUBU; d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x24: d.opc = I_AND;  d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x25: d.opc = I_OR;   d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x26: d.opc = I_XOR;  d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x27: d.opc = I_NOR;  d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x2a: d.opc = I_SLT;  d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        case 0x2b: d.opc = I_SLTU; d.r_out = RDn; d.r_in = RSn; d.r_in2 = RTn;             break;
        } break;
    case 0x01: // REGIMM
        switch((op >> 16) & 0x1f) {
        case 0x00: d.opc = I_BLTZ;   d.r_in = RSn;               d.imm = IMMi16 << 2; break;
        case 0x01: d.opc = I_BGEZ;   d.r_in = RSn;               d.imm = IMMi16 << 2; break;
        case 0x10: d.opc = I_BLTZAL; d.r_out = 31; d.r_in = RSn; d.imm = IMMi16 << 2; break;
        case 0x11: d.opc = I_BGEZAL; d.r_out = 31; d.r_in = RSn; d.imm = IMMi16 << 2; break;
        } break;
    case 0x02: d.opc = I_J;                                                 d.imm = IMMu26 << 2; break;
    case 0x03: d.opc = I_JAL;   d.r_out = 31;                               d.imm = IMMu26 << 2; break;
    case 0x04: d.opc = I_BEQ;                  d.r_in = RSn; d.r_in2 = RTn; d.imm = IMMi16 << 2; break;
    case 0x05: d.opc = I_BNE;                  d.r_in = RSn; d.r_in2 = RTn; d.imm = IMMi16 << 2; break;
    case 0x06: d.opc = I_BLEZ;                 d.r_in = RSn;                d.imm = IMMi16 << 2; break;
    case 0x07: d.opc = I_BGTZ;                 d.r_in = RSn;                d.imm = IMMi16 << 2; break;
    case 0x08: d.opc = I_ADDI;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16;      break;
    case 0x09: d.opc = I_ADDIU; d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16;      break;
    case 0x0a: d.opc = I_SLTI;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16;      break;
    case 0x0b: d.opc = I_SLTIU; d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16;      break;
    case 0x0c: d.opc = I_ANDI;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMu16;      break;
    case 0x0d: d.opc = I_ORI;   d.r_out = RTn; d.r_in = RSn;                d.imm = IMMu16;      break;
    case 0x0e: d.opc = I_XORI;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMu16;      break;
    case 0x0f: d.opc = I_LUI;   d.r_out = RTn;                              d.imm = IMMu16;      break;
    case 0x10: // SCC
        switch((op >> 21) & 0x1f) {
        case 0x00: d.opc = I_MFC0; d.r_out = RTn;               d.imm = RDn; break;
        case 0x04: d.opc = I_MTC0;                d.r_in = RTn; d.imm = RDn; break;
        } break;
    case 0x12: // VU
        if ((op >> 25) & 1) {
            #define E  (int)((op >> 21) & 15)
            #define DE (int)((op >> 11) &  7)
            switch(op & 0x3f) {
            case 0x00: d.opc = I_VMULF; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x01: d.opc = I_VMULU; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x02: d.opc = I_VRNDP; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x03: d.opc = I_VMULQ; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x04: d.opc = I_VMUDL; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x05: d.opc = I_VMUDM; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x06: d.opc = I_VMUDN; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x07: d.opc = I_VMUDH; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x08: d.opc = I_VMACF; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x09: d.opc = I_VMACU; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x0a: d.opc = I_VRNDN; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x0b: d.opc = I_VMACQ; d.v_out = VMASK(VDn); break;
            case 0x0c: d.opc = I_VMADL; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x0d: d.opc = I_VMADM; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x0e: d.opc = I_VMADN; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x0f: d.opc = I_VMADH; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x10: d.opc = I_VADD; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0); break;
            case 0x11: d.opc = I_VSUB; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0); break;
            case 0x13: d.opc = I_VABS; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0); break;
            case 0x14: d.opc = I_VADDC; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0); break;
            case 0x15: d.opc = I_VSUBC; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0); break;
            case 0x1d: d.opc = I_VSAR; d.v_out = VMASK(VDn); break;
            case 0x20: d.opc = I_VLT; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1); break;
            case 0x21: d.opc = I_VEQ; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1); break;
            case 0x22: d.opc = I_VNE; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1); break;
            case 0x23: d.opc = I_VGE; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1); break;
            case 0x24: d.opc = I_VCL; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1) | VMASK(2); break;
            case 0x25: d.opc = I_VCH; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1) | VMASK(2); break;
            case 0x26: d.opc = I_VCR; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1) | VMASK(2); break;
            case 0x27: d.opc = I_VMRG; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); d.vc_in = d.vc_out = VMASK(0) | VMASK(1); break;
            case 0x28: d.opc = I_VAND; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x29: d.opc = I_VNAND; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x2a: d.opc = I_VOR; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x2b: d.opc = I_VNOR; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x2c: d.opc = I_VXOR; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x2d: d.opc = I_VNXOR; d.v_out = VMASK(VDn); d.v_in = VMASK(VSn) | VMASK(VTn); break;
            case 0x30: d.opc = I_VRCP; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x31: d.opc = I_VRCPL; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x32: d.opc = I_VRCPH; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x33: d.opc = I_VMOV; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x34: d.opc = I_VRSQ; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x35: d.opc = I_VRSQL; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x36: d.opc = I_VRSQH; d.v_out = VMASK(VDn); d.v_in = VMASK(VTn); break;
            case 0x37: d.opc = I_VNOP; break;
            }
            #undef E
            #undef DE
        } else {
            #define E (int)((op >> 7) & 15)
            switch((op >> 21) & 0x1f) {
            case 0x00: d.opc = I_MFC2; d.r_out = RTn; d.v_in = VMASK(VSn); break;
            case 0x02: d.opc = I_CFC2; d.r_out = RTn; d.vc_in = VMASK(RDn & 3); break;
            case 0x04: d.opc = I_MTC2; d.r_in = RTn; d.v_out = VMASK(VSn); break;
            case 0x06: d.opc = I_CTC2; d.r_in = RTn; d.vc_out = VMASK(RDn & 3); break;
            }
            #undef E
        } break;
    case 0x20: d.opc = I_LB;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16; break;
    case 0x21: d.opc = I_LH;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16; break;
    case 0x23: d.opc = I_LW;  d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16; break;
    case 0x24: d.opc = I_LBU; d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16; break;
    case 0x25: d.opc = I_LHU; d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16; break;
    case 0x27: d.opc = I_LWU; d.r_out = RTn; d.r_in = RSn;                d.imm = IMMi16; break;
    case 0x28: d.opc = I_SB;                 d.r_in = RTn; d.r_in2 = RSn; d.imm = IMMi16; break;
    case 0x29: d.opc = I_SH;                 d.r_in = RTn; d.r_in2 = RSn; d.imm = IMMi16; break;
    case 0x2b: d.opc = I_SW;                 d.r_in = RTn; d.r_in2 = RSn; d.imm = IMMi16; break;
    case 0x32: // LWC2
        #define E     (int)((op >> 7) & 15)
        switch((op >> 11) & 0x1f) {
        case 0x00: d.opc = I_LBV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x01: d.opc = I_LSV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x02: d.opc = I_LLV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x03: d.opc = I_LDV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x04: d.opc = I_LQV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x05: d.opc = I_LRV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x06: d.opc = I_LPV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x07: d.opc = I_LUV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x08: d.opc = I_LHV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x09: d.opc = I_LFV; d.v_out = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x0b: d.opc = I_LTV; d.v_out = VBLOCK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        } break;
        #undef E
    case 0x3a: // SWC2
        #define E     (int)((op >> 7) & 15)
        switch((op >> 11) & 0x1f) {
        case 0x00: d.opc = I_SBV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x01: d.opc = I_SSV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x02: d.opc = I_SLV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x03: d.opc = I_SDV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x04: d.opc = I_SQV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x05: d.opc = I_SRV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x06: d.opc = I_SPV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x07: d.opc = I_SUV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x08: d.opc = I_SHV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x09: d.opc = I_SFV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x0a: d.opc = I_SWV; d.v_in = VMASK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        case 0x0b: d.opc = I_STV; d.v_in = VBLOCK(VTn); d.r_in = RSn; d.imm = IMMi7; break;
        } break;
        #undef E
    }
    #undef SA
    #undef RDn
    #undef RTn
    #undef RSn
    #undef VDn
    #undef VSn
    #undef VTn
    #undef IMMi7
    #undef IMMi16
    #undef IMMu16
    #undef IMMu26
    #undef VMASK
    #undef VBLOCK

    *d_out = d;
    return;
}
