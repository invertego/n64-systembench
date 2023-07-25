#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include "disasm.h"

static const char* reg_names[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1",
    "gp", "sp", "fp", "ra"
};

static const char* vu_reg_names[32] = {
    "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
    "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
    "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"
};

void disasm_instr(char* buf, uint32_t pc, uint32_t op) {
    #define SA     (int)((op >>  6) & 31)
    #define RDn    (int)((op >> 11) & 31)
    #define RTn    (int)((op >> 16) & 31)
    #define RSn    (int)((op >> 21) & 31)
    #define VDn    ((op >>  6) & 31)
    #define VSn    ((op >> 11) & 31)
    #define VTn    ((op >> 16) & 31)
    #define RD     reg_names[RDn]
    #define RT     reg_names[RTn]
    #define RS     reg_names[RSn]
    #define VD     vu_reg_names[VDn]
    #define VS     vu_reg_names[VSn]
    #define VT     vu_reg_names[VTn]
    #define IMMi7  ((int8_t)(op << 1) >> 1)
    #define IMMi16 (int16_t)(op)
    #define IMMu16 (uint16_t)(op)
    #define IMMu26 (op & 0x03ffffff)
    uint32_t npc = pc + 4;

    sprintf(buf, "INVALID");

    switch (op >> 26) {
    case 0x00: // SPECIAL
        switch(op & 0x3f) {
        case 0x00: sprintf(buf, "SLL %s, %s, %d", RD, RT, SA); break;
        case 0x02: sprintf(buf, "SRL %s, %s, %d", RD, RT, SA); break;
        case 0x03: sprintf(buf, "SRA %s, %s, %d", RD, RT, SA); break;
        case 0x04: sprintf(buf, "SLLV %s, %s, %s", RD, RT, RS); break;
        case 0x06: sprintf(buf, "SRLV %s, %s, %s", RD, RT, RS); break;
        case 0x07: sprintf(buf, "SRAV %s, %s, %s", RD, RT, RS); break;
        case 0x08: sprintf(buf, "JR %s", RS); break;
        case 0x09: sprintf(buf, "JALR %s, %s", RD, RS); break;
        case 0x0d: sprintf(buf, "BREAK"); break;
        case 0x20: sprintf(buf, "ADD %s, %s, %s", RD, RS, RT); break;
        case 0x21: sprintf(buf, "ADDU %s, %s, %s", RD, RS, RT); break;
        case 0x22: sprintf(buf, "SUB %s, %s, %s", RD, RS, RT); break;
        case 0x23: sprintf(buf, "SUBU %s, %s, %s", RD, RS, RT); break;
        case 0x24: sprintf(buf, "AND %s, %s, %s", RD, RS, RT); break;
        case 0x25: sprintf(buf, "OR %s, %s, %s", RD, RS, RT); break;
        case 0x26: sprintf(buf, "XOR %s, %s, %s", RD, RS, RT); break;
        case 0x27: sprintf(buf, "NOR %s, %s, %s", RD, RS, RT); break;
        case 0x2a: sprintf(buf, "SLT %s, %s, %s", RD, RS, RT); break;
        case 0x2b: sprintf(buf, "SLTU %s, %s, %s", RD, RS, RT); break;
        } break;
    case 0x01: // REGIMM
        switch((op >> 16) & 0x1f) {
        case 0x00: sprintf(buf, "BLTZ %s, 0x%" PRIX32 "", RS, npc + (IMMi16 << 2)); break;
        case 0x01: sprintf(buf, "BGEZ %s, 0x%" PRIX32 "", RS, npc + (IMMi16 << 2)); break;
        case 0x10: sprintf(buf, "BLTZAL %s, 0x%" PRIX32 "", RS, npc + (IMMi16 << 2)); break;
        case 0x11: sprintf(buf, "BGEZAL %s, 0x%" PRIX32 "", RS, npc + (IMMi16 << 2)); break;
        } break;
    case 0x02: sprintf(buf, "J 0x%" PRIX32 "", IMMu26 << 2); break;
    case 0x03: sprintf(buf, "JAL 0x%" PRIX32 "", IMMu26 << 2); break;
    case 0x04: sprintf(buf, "BEQ %s, %s, 0x%" PRIX32 "", RS, RT, npc + (IMMi16 << 2)); break;
    case 0x05: sprintf(buf, "BNE %s, %s, 0x%" PRIX32 "", RS, RT, npc + (IMMi16 << 2)); break;
    case 0x06: sprintf(buf, "BLEZ %s, 0x%" PRIX32 "", RS, npc + (IMMi16 << 2)); break;
    case 0x07: sprintf(buf, "BGTZ %s, 0x%" PRIX32 "", RS, npc + (IMMi16 << 2)); break;
    case 0x08: sprintf(buf, "ADDI %s, %s, %d", RT, RS, IMMi16); break;
    case 0x09: sprintf(buf, "ADDIU %s, %s, %d", RT, RS, IMMi16); break;
    case 0x0a: sprintf(buf, "SLTI %s, %s, %d", RT, RS, IMMi16); break;
    case 0x0b: sprintf(buf, "SLTIU %s, %s, %d", RT, RS, IMMi16); break;
    case 0x0c: sprintf(buf, "ANDI %s, %s, %d", RT, RS, IMMu16); break;
    case 0x0d: sprintf(buf, "ORI %s, %s, %d", RT, RS, IMMu16); break;
    case 0x0e: sprintf(buf, "XORI %s, %s, %d", RT, RS, IMMu16); break;
    case 0x0f: sprintf(buf, "LUI %s, %d", RT, IMMu16); break;
    case 0x10: // SCC
        switch((op >> 21) & 0x1f) {
        case 0x00: sprintf(buf, "MFC0 %s, %d", RT, RDn); break;
        case 0x04: sprintf(buf, "MTC0 %s, %d", RT, RDn); break;
        } break;
    case 0x12: // VU
        if ((op >> 25) & 1) {
            #define E  (int)((op >> 21) & 15)
            #define DE (int)((op >> 11) &  7)
            switch(op & 0x3f) {
            case 0x00: sprintf(buf, "VMULF %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x01: sprintf(buf, "VMULU %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x02: sprintf(buf, "VRNDP %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x03: sprintf(buf, "VMULQ %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x04: sprintf(buf, "VMUDL %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x05: sprintf(buf, "VMUDM %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x06: sprintf(buf, "VMUDN %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x07: sprintf(buf, "VMUDH %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x08: sprintf(buf, "VMACF %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x09: sprintf(buf, "VMACU %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x0a: sprintf(buf, "VRNDN %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x0b: sprintf(buf, "VMACQ %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x0c: sprintf(buf, "VMADL %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x0d: sprintf(buf, "VMADM %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x0e: sprintf(buf, "VMADN %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x0f: sprintf(buf, "VMADH %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x10: sprintf(buf, "VADD %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x11: sprintf(buf, "VSUB %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x13: sprintf(buf, "VABS %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x14: sprintf(buf, "VADDC %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x15: sprintf(buf, "VSUBC %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x1d: sprintf(buf, "VSAR %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x20: sprintf(buf, "VLT %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x21: sprintf(buf, "VEQ %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x22: sprintf(buf, "VNE %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x23: sprintf(buf, "VGE %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x24: sprintf(buf, "VCL %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x25: sprintf(buf, "VCH %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x26: sprintf(buf, "VCR %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x27: sprintf(buf, "VMRG %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x28: sprintf(buf, "VAND %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x29: sprintf(buf, "VNAND %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x2a: sprintf(buf, "VOR %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x2b: sprintf(buf, "VNOR %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x2c: sprintf(buf, "VXOR %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x2d: sprintf(buf, "VNXOR %s, %s, %s[%d]", VD, VS, VT, E); break;
            case 0x30: sprintf(buf, "VRCP %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x31: sprintf(buf, "VRCPL %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x32: sprintf(buf, "VRCPH %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x33: sprintf(buf, "VMOV %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x34: sprintf(buf, "VRSQ %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x35: sprintf(buf, "VRSQL %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x36: sprintf(buf, "VRSQH %s[%d], %s[%d]", VD, DE, VT, E); break;
            case 0x37: sprintf(buf, "VNOP %s[%d], %s[%d]", VD, DE, VT, E); break;
            }
            #undef E
            #undef DE
        } else {
            #define E (int)((op >> 7) & 15)
            switch((op >> 21) & 0x1f) {
            case 0x00: sprintf(buf, "MFC2 %s, %s[%d]", RT, VS, E); break;
            case 0x02: sprintf(buf, "CFC2 %s, %d", RT, RDn); break;
            case 0x04: sprintf(buf, "MTC2 %s, %s[%d]", RT, VS, E); break;
            case 0x06: sprintf(buf, "CTC2 %s, %d", RT, RDn); break;
            }
            #undef E
        } break;
    case 0x20: sprintf(buf, "LB %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x21: sprintf(buf, "LH %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x23: sprintf(buf, "LW %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x24: sprintf(buf, "LBU %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x25: sprintf(buf, "LHU %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x27: sprintf(buf, "LWU %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x28: sprintf(buf, "SB %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x29: sprintf(buf, "SH %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x2b: sprintf(buf, "SW %s, %d(%s)", RT, IMMi16, RS); break;
    case 0x32: // LWC2
        #define E     (int)((op >> 7) & 15)
        switch((op >> 11) & 0x1f) {
        case 0x00: sprintf(buf, "LBV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x01: sprintf(buf, "LSV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x02: sprintf(buf, "LLV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x03: sprintf(buf, "LDV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x04: sprintf(buf, "LQV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x05: sprintf(buf, "LRV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x06: sprintf(buf, "LPV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x07: sprintf(buf, "LUV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x08: sprintf(buf, "LHV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x09: sprintf(buf, "LFV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        case 0x0b: sprintf(buf, "LTV %s[%d] %d(%s)", VT, E, IMMi7, RS); break;
        } break;
        #undef E
    case 0x3a: // SWC2
        #define E     (int)((op >> 7) & 15)
        switch((op >> 11) & 0x1f) {
        case 0x00: sprintf(buf, "SBV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x01: sprintf(buf, "SSV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x02: sprintf(buf, "SLV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x03: sprintf(buf, "SDV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x04: sprintf(buf, "SQV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x05: sprintf(buf, "SRV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x06: sprintf(buf, "SPV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x07: sprintf(buf, "SUV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x08: sprintf(buf, "SHV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x09: sprintf(buf, "SFV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x0a: sprintf(buf, "SWV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
        case 0x0b: sprintf(buf, "STV %s[%d], %d(%s)", VT, E, IMMi7, RS); break;
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
    #undef RD
    #undef RT
    #undef RS
    #undef VD
    #undef VS
    #undef VT
    #undef IMMi7
    #undef IMMi16
    #undef IMMu16
    #undef IMMu26

    if (op == 0) sprintf(buf, "NOP");
}
