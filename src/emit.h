#pragma once

#define emit_word(iw) (*code_ptr = (iw), ++code_ptr)
#define emit_nop() emit_word(0x00)

#define emit_jump(op, target) emit_word((op) << 26 | ((target) & 0x3FFFFFF))
#define emit_j(target)    emit_jump(0x02, target)
#define emit_jal(target)  emit_jump(0x03, target)

#define emit_branch(op, rs, rt, offset) emit_word((op) << 26 | (rs) << 21 | (rt) << 16 | ((offset) & 0xFFFF))
#define emit_beq(rs, rt, offset) emit_branch(0x04, rs, rt, offset)
#define emit_bne(rs, rt, offset) emit_branch(0x05, rs, rt, offset)
#define emit_blez(rs, offset)    emit_branch(0x06, rs, 0, offset)
#define emit_bgtz(rs, offset)    emit_branch(0x07, rs, 0, offset)

#define emit_alu(op, rt, rs, imm) emit_word((op) << 26 | (rs) << 21 | (rt) << 16 | ((imm) & 0xFFFF))
#define emit_addi(rt, rs, imm)  emit_alu(0x08, rt, rs, imm)
#define emit_addiu(rt, rs, imm) emit_alu(0x09, rt, rs, imm)
#define emit_slti(rt, rs, imm)  emit_alu(0x0A, rt, rs, imm)
#define emit_sltiu(rt, rs, imm) emit_alu(0x0B, rt, rs, imm)
#define emit_andi(rt, rs, imm)  emit_alu(0x0C, rt, rs, imm)
#define emit_ori(rt, rs, imm)   emit_alu(0x0D, rt, rs, imm)
#define emit_xori(rt, rs, imm)  emit_alu(0x0E, rt, rs, imm)
#define emit_lui(rt, imm)       emit_alu(0x0F, rt, 0, imm)

#define emit_loadstore(op, rt, offset, base) emit_word((op) << 26 | (base) << 21 | (rt) << 16 | ((offset) & 0xFFFF))
#define emit_lb(rt, offset, base)    emit_loadstore(0x20, rt, offset, base)
#define emit_lh(rt, offset, base)    emit_loadstore(0x21, rt, offset, base)
#define emit_lw(rt, offset, base)    emit_loadstore(0x23, rt, offset, base)
#define emit_lbu(rt, offset, base)   emit_loadstore(0x24, rt, offset, base)
#define emit_lhu(rt, offset, base)   emit_loadstore(0x25, rt, offset, base)
#define emit_lwu(rt, offset, base)   emit_loadstore(0x27, rt, offset, base)
#define emit_sb(rt, offset, base)    emit_loadstore(0x28, rt, offset, base)
#define emit_sh(rt, offset, base)    emit_loadstore(0x29, rt, offset, base)
#define emit_sw(rt, offset, base)    emit_loadstore(0x2B, rt, offset, base)

// SPECIAL

#define emit_shift_imm(op, rd, rt, sa) emit_word((rt) << 16 | (rd) << 11 | (sa) << 6 | (op))
#define emit_sll(rd, rt, sa)   emit_shift_imm(0x00, rd, rt, sa)
#define emit_srl(rd, rt, sa)   emit_shift_imm(0x02, rd, rt, sa)
#define emit_sra(rd, rt, sa)   emit_shift_imm(0x03, rd, rt, sa)

#define emit_alu_reg(op, rd, rt, rs) emit_word((rs) << 21 | (rt) << 16 | (rd) << 11 | (op))
#define emit_sllv(rd, rt, rs)  emit_alu_reg(0x04, rd, rt, rs)
#define emit_srlv(rd, rt, rs)  emit_alu_reg(0x06, rd, rt, rs)
#define emit_srav(rd, rt, rs)  emit_alu_reg(0x07, rd, rt, rs)

#define emit_jr(rs)          emit_word((rs) << 21 | 0x08)
#define emit_jalr(rd, rs)    emit_word((rs) << 21 | (rd) << 11 | 0x09)

#define emit_break()   emit_word(0x0D)

#define emit_add(rd, rs, rt)   emit_alu_reg(0x20, rd, rs, rt)
#define emit_addu(rd, rs, rt)  emit_alu_reg(0x21, rd, rs, rt)
#define emit_sub(rd, rs, rt)   emit_alu_reg(0x22, rd, rs, rt)
#define emit_subu(rd, rs, rt)  emit_alu_reg(0x23, rd, rs, rt)
#define emit_and(rd, rs, rt)   emit_alu_reg(0x24, rd, rs, rt)
#define emit_or(rd, rs, rt)    emit_alu_reg(0x25, rd, rs, rt)
#define emit_xor(rd, rs, rt)   emit_alu_reg(0x26, rd, rs, rt)
#define emit_nor(rd, rs, rt)   emit_alu_reg(0x27, rd, rs, rt)
#define emit_slt(rd, rs, rt)   emit_alu_reg(0x2a, rd, rs, rt)
#define emit_sltu(rd, rs, rt)  emit_alu_reg(0x2b, rd, rs, rt)

// REGIMM

#define emit_branch_reg(op, rs, offset) emit_word(0x01 << 26 | (rs) << 21 | (op) << 16 | ((offset) & 0xFFFF))
#define emit_bltz(rs, offset)    emit_branch_reg(0x00, rs, offset)
#define emit_bgez(rs, offset)    emit_branch_reg(0x01, rs, offset)
#define emit_bltzal(rs, offset)  emit_branch_reg(0x10, rs, offset)
#define emit_bgezal(rs, offset)  emit_branch_reg(0x11, rs, offset)

// SCC

#define emit_cop0(op, rt, rd) emit_word(0x10 << 26 | (op) << 21 | (rt) << 16 | (rd) << 11)
#define emit_mfc0(rt, rd) emit_cop0(0x00, rt, rd)
#define emit_mtc0(rt, rd) emit_cop0(0x04, rt, rd)

// VU

#define emit_cop2(op, rt, rd, e) emit_word(0x12 << 26 | (op) << 21 | (rt) << 16 | (rd) << 11 | (e) << 7)
#define emit_mfc2(rt, rd, e) emit_cop2(0x00, rt, rd, e)
#define emit_cfc2(rt, rd)    emit_cop2(0x02, rt, rd, 0)
#define emit_mtc2(rt, rd, e) emit_cop2(0x04, rt, rd, e)
#define emit_ctc2(rt, rd)    emit_cop2(0x06, rt, rd, 0)

#define emit_vu(op, vd, vs, vt, e) emit_word(0x12 << 26 | 1 << 25 | (e) << 21 | (vt) << 16 | (vs) << 11 | (vd) << 6 | (op))
#define emit_vmulf(vd, vs, vt, e) emit_vu(0x00, vd, vs, vt, e)
#define emit_vmulu(vd, vs, vt, e) emit_vu(0x01, vd, vs, vt, e)
#define emit_vrndp(vd, vs, vt, e) emit_vu(0x02, vd, vs, vt, e)
#define emit_vmulq(vd, vs, vt, e) emit_vu(0x03, vd, vs, vt, e)
#define emit_vmudl(vd, vs, vt, e) emit_vu(0x04, vd, vs, vt, e)
#define emit_vmudm(vd, vs, vt, e) emit_vu(0x05, vd, vs, vt, e)
#define emit_vmudn(vd, vs, vt, e) emit_vu(0x06, vd, vs, vt, e)
#define emit_vmudh(vd, vs, vt, e) emit_vu(0x07, vd, vs, vt, e)
#define emit_vmacf(vd, vs, vt, e) emit_vu(0x08, vd, vs, vt, e)
#define emit_vmacu(vd, vs, vt, e) emit_vu(0x09, vd, vs, vt, e)
#define emit_vrndn(vd, vs, vt, e) emit_vu(0x0a, vd, vs, vt, e)
#define emit_vmacq(vd)            emit_vu(0x0b, vd, 0, 0, 0)
#define emit_vmadl(vd, vs, vt, e) emit_vu(0x0c, vd, vs, vt, e)
#define emit_vmadm(vd, vs, vt, e) emit_vu(0x0d, vd, vs, vt, e)
#define emit_vmadn(vd, vs, vt, e) emit_vu(0x0e, vd, vs, vt, e)
#define emit_vmadh(vd, vs, vt, e) emit_vu(0x0f, vd, vs, vt, e)
#define emit_vadd(vd, vs, vt, e)  emit_vu(0x10, vd, vs, vt, e)
#define emit_vsub(vd, vs, vt, e)  emit_vu(0x11, vd, vs, vt, e)
#define emit_vabs(vd, vs, vt, e)  emit_vu(0x13, vd, vs, vt, e)
#define emit_vaddc(vd, vs, vt, e) emit_vu(0x14, vd, vs, vt, e)
#define emit_vsubc(vd, vs, vt, e) emit_vu(0x15, vd, vs, vt, e)
#define emit_vsar(vd, vs, vt, e)  emit_vu(0x1d, vd, vs, vt, e)
#define emit_vlt(vd, vs, vt, e)   emit_vu(0x20, vd, vs, vt, e)
#define emit_veq(vd, vs, vt, e)   emit_vu(0x21, vd, vs, vt, e)
#define emit_vne(vd, vs, vt, e)   emit_vu(0x22, vd, vs, vt, e)
#define emit_vge(vd, vs, vt, e)   emit_vu(0x23, vd, vs, vt, e)
#define emit_vcl(vd, vs, vt, e)   emit_vu(0x24, vd, vs, vt, e)
#define emit_vch(vd, vs, vt, e)   emit_vu(0x25, vd, vs, vt, e)
#define emit_vcr(vd, vs, vt, e)   emit_vu(0x26, vd, vs, vt, e)
#define emit_vmrg(vd, vs, vt, e)  emit_vu(0x27, vd, vs, vt, e)
#define emit_vand(vd, vs, vt, e)  emit_vu(0x28, vd, vs, vt, e)
#define emit_vnand(vd, vs, vt, e) emit_vu(0x29, vd, vs, vt, e)
#define emit_vor(vd, vs, vt, e)   emit_vu(0x2a, vd, vs, vt, e)
#define emit_vnor(vd, vs, vt, e)  emit_vu(0x2b, vd, vs, vt, e)
#define emit_vxor(vd, vs, vt, e)  emit_vu(0x2c, vd, vs, vt, e)
#define emit_vnxor(vd, vs, vt, e) emit_vu(0x2d, vd, vs, vt, e)
#define emit_vrcp(vd, de, vt, e)  emit_vu(0x30, vd, de, vt, e)
#define emit_vrcpl(vd, de, vt, e) emit_vu(0x31, vd, de, vt, e)
#define emit_vrcph(vd, de, vt, e) emit_vu(0x32, vd, de, vt, e)
#define emit_vmov(vd, de, vt, e)  emit_vu(0x33, vd, de, vt, e)
#define emit_vrsq(vd, de, vt, e)  emit_vu(0x34, vd, de, vt, e)
#define emit_vrsql(vd, de, vt, e) emit_vu(0x35, vd, de, vt, e)
#define emit_vrsqh(vd, de, vt, e) emit_vu(0x36, vd, de, vt, e)
#define emit_vnop()               emit_vu(0x37, 0, 0, 0, 0)

// LWC2
#define emit_lwc2(op, vt, e, offset, base) emit_word(0x32 << 26 | (base) << 21 | (vt) << 16 | (op) << 11 | (e) << 7 | ((offset) & 0x7F))
#define emit_lbv(vt, e, offset, base) emit_lwc2(0x00, vt, e, offset, base)
#define emit_lsv(vt, e, offset, base) emit_lwc2(0x01, vt, e, offset, base)
#define emit_llv(vt, e, offset, base) emit_lwc2(0x02, vt, e, offset, base)
#define emit_ldv(vt, e, offset, base) emit_lwc2(0x03, vt, e, offset, base)
#define emit_lqv(vt, e, offset, base) emit_lwc2(0x04, vt, e, offset, base)
#define emit_lrv(vt, e, offset, base) emit_lwc2(0x05, vt, e, offset, base)
#define emit_lpv(vt, e, offset, base) emit_lwc2(0x06, vt, e, offset, base)
#define emit_luv(vt, e, offset, base) emit_lwc2(0x07, vt, e, offset, base)
#define emit_lhv(vt, e, offset, base) emit_lwc2(0x08, vt, e, offset, base)
#define emit_lfv(vt, e, offset, base) emit_lwc2(0x09, vt, e, offset, base)
#define emit_ltv(vt, e, offset, base) emit_lwc2(0x0b, vt, e, offset, base)

// SWC2
#define emit_swc2(op, vt, e, offset, base) emit_word(0x3A << 26 | (base) << 21 | (vt) << 16 | (op) << 11 | (e) << 7 | ((offset) & 0x7F))
#define emit_sbv(vt, e, offset, base) emit_swc2(0x00, vt, e, offset, base)
#define emit_ssv(vt, e, offset, base) emit_swc2(0x01, vt, e, offset, base)
#define emit_slv(vt, e, offset, base) emit_swc2(0x02, vt, e, offset, base)
#define emit_sdv(vt, e, offset, base) emit_swc2(0x03, vt, e, offset, base)
#define emit_sqv(vt, e, offset, base) emit_swc2(0x04, vt, e, offset, base)
#define emit_srv(vt, e, offset, base) emit_swc2(0x05, vt, e, offset, base)
#define emit_spv(vt, e, offset, base) emit_swc2(0x06, vt, e, offset, base)
#define emit_suv(vt, e, offset, base) emit_swc2(0x07, vt, e, offset, base)
#define emit_shv(vt, e, offset, base) emit_swc2(0x08, vt, e, offset, base)
#define emit_sfv(vt, e, offset, base) emit_swc2(0x09, vt, e, offset, base)
#define emit_swv(vt, e, offset, base) emit_swc2(0x0a, vt, e, offset, base)
#define emit_stv(vt, e, offset, base) emit_swc2(0x0b, vt, e, offset, base)
