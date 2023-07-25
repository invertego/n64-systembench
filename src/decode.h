#pragma once

typedef enum {
    I_INVALID = -1,
    I_SLL = 0,
    I_SRL,
    I_SRA,
    I_SLLV,
    I_SRLV,
    I_SRAV,
    I_JR,
    I_JALR,
    I_BREAK,
    I_ADD,
    I_ADDU,
    I_SUB,
    I_SUBU,
    I_AND,
    I_OR,
    I_XOR,
    I_NOR,
    I_SLT,
    I_SLTU,
    I_BLTZ,
    I_BGEZ,
    I_BLTZAL,
    I_BGEZAL,
    I_J,
    I_JAL,
    I_BEQ,
    I_BNE,
    I_BLEZ,
    I_BGTZ,
    I_ADDI,
    I_ADDIU,
    I_SLTI,
    I_SLTIU,
    I_ANDI,
    I_ORI,
    I_XORI,
    I_LUI,
    I_MFC0,
    I_MTC0,
    I_LB,
    I_LH,
    I_LW,
    I_LBU,
    I_LHU,
    I_LWU,
    I_SB,
    I_SH,
    I_SW,
    I_MFC2,
    I_CFC2,
    I_MTC2,
    I_CTC2,
    I_VMULF,
    I_VMULU,
    I_VRNDP,
    I_VMULQ,
    I_VMUDL,
    I_VMUDM,
    I_VMUDN,
    I_VMUDH,
    I_VMACF,
    I_VMACU,
    I_VRNDN,
    I_VMACQ,
    I_VMADL,
    I_VMADM,
    I_VMADN,
    I_VMADH,
    I_VADD,
    I_VSUB,
    I_VABS,
    I_VADDC,
    I_VSUBC,
    I_VSAR,
    I_VLT,
    I_VEQ,
    I_VNE,
    I_VGE,
    I_VCL,
    I_VCH,
    I_VCR,
    I_VMRG,
    I_VAND,
    I_VNAND,
    I_VOR,
    I_VNOR,
    I_VXOR,
    I_VNXOR,
    I_VRCP,
    I_VRCPL,
    I_VRCPH,
    I_VMOV,
    I_VRSQ,
    I_VRSQL,
    I_VRSQH,
    I_VNOP,
    I_LBV,
    I_LSV,
    I_LLV,
    I_LDV,
    I_LQV,
    I_LRV,
    I_LPV,
    I_LUV,
    I_LHV,
    I_LFV,
    I_LTV,
    I_SBV,
    I_SSV,
    I_SLV,
    I_SDV,
    I_SQV,
    I_SRV,
    I_SPV,
    I_SUV,
    I_SHV,
    I_SFV,
    I_SWV,
    I_STV,
    I_COUNT,
} opcode_t;

typedef struct {
    int opc;
    int r_in;
    int r_in2;
    int r_out;
    int v_in;
    int v_out;
    int v_fake;
    int vc_in;
    int vc_out;
    int imm;
} decoded_t;

typedef enum {
    IF_BYPASS = 1 << 0,
    IF_LOAD_MEM = 1 << 1,
    IF_LOADSTORE_COP = 1 << 2,
    IF_STORE_MEM = 1 << 3,
    IF_BRANCH_UNCOND = 1 << 4,
    IF_BRANCH_COND = 1 << 5,
    IF_VU = 1 << 6,
} opcode_flag_t;

#define TEST_IFLAGS(opc, flags) !!(get_opc_flags(opc) & (flags))
#define IS_BRANCH(opc) TEST_IFLAGS(opc, IF_BRANCH_UNCOND | IF_BRANCH_COND)
#define IS_BYPASSABLE(opc) TEST_IFLAGS(opc, IF_BYPASS)
#define IS_LOAD(opc) TEST_IFLAGS(opc, IF_LOAD_MEM | IF_LOADSTORE_COP)
#define IS_STORE(opc) TEST_IFLAGS(opc, IF_STORE_MEM | IF_LOADSTORE_COP)
#define IS_VU(opc) TEST_IFLAGS(opc, IF_VU)

int get_opc_flags(int opc);

void decode_instr(decoded_t* d_out, uint32_t op);
