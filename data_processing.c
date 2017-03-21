#include <stdio.h>
#include <stdlib.h>

#include "bits.h"
#include "branch.h"
#include "debug_utils.h"
#include "state.h"

#include "data_processing.h"

// Data Processing, Src2, immediate
struct dp_src2_immediate {
    unsigned int rot  : 4 ;
    unsigned int imm8 : 8 ;
};

// Data Processing, Src2, (Constant shifted) register
struct dp_src2_reg {
    unsigned int shamt5       : 5 ;
    unsigned int sh           : 2 ;
    unsigned int is_reg_shift : 1 ; // set to 0 here because shifted by a constant
    unsigned int rm           : 4 ;
};

// Data Processing, Src2, Register shifted by another register
struct dp_src2_reg_shifted {
    unsigned int rs           : 4 ;
    unsigned int              : 1 ; // unused
    unsigned int sh           : 2 ;
    unsigned int is_reg_shift : 1 ; // set to 1 here because shifted by a register
    unsigned int rm           : 4 ;
};

struct dp_instr decode_dp_instr (unsigned int raw)
{
    return (struct dp_instr) {
        .cond   =     select_bits(raw, 31, 28) ,
        .op     =     select_bits(raw, 27, 26) ,
        .i      =     select_bits(raw, 25, 25) ,
        .cmd    =     select_bits(raw, 24, 21) ,
        .s      =     select_bits(raw, 20, 20) ,
        .rn     =     select_bits(raw, 19, 16) ,
        .rd     =     select_bits(raw, 15, 12) ,
        .src2   =     select_bits(raw, 11, 0) 
    };
}

struct dp_src2_immediate decode_dp_src2_immediate (unsigned int raw)
{
    return (struct dp_src2_immediate) {
        .rot  = select_bits(raw, 11, 8),
        .imm8 = select_bits(raw,  7, 0)
    };
}

struct dp_src2_reg decode_dp_src2_reg (unsigned int raw)
{
    return (struct dp_src2_reg) {
        .shamt5       = select_bits(raw, 11, 7),
        .sh           = select_bits(raw,  6, 5),
        .is_reg_shift = select_bits(raw,  4, 4),
        .rm           = select_bits(raw,  3, 0),
    };
}

int rotate_immediate (int rot, int imm8)
{
    rot *= 2;
    return (imm8 << rot)|(imm8 >> (32 - rot));
}

void add_or_subtract(struct state* state, struct dp_instr* inst, bool is_add)
{
    int src2_value = 0 ;
    if (inst->i == 1) {
        struct dp_src2_immediate imm = decode_dp_src2_immediate(inst->src2);
        src2_value = rotate_immediate(imm.rot, imm.imm8);
    } else {
        struct dp_src2_reg reg = decode_dp_src2_reg(inst->src2);
        if (reg.is_reg_shift) {
            fprintf(stderr, "Src2 register shifted is not supported yet.\n");
            exit(EXIT_FAILURE);
        } else {
            if (reg.shamt5 != 0) {
                fprintf(stderr, "Src2 shifts are not supported at this time. \n");
                exit(EXIT_FAILURE);
            }
            src2_value = (int) state->regs[reg.rm];
        }
    }
    if (!is_add) {
        src2_value = -src2_value;
    }
    debug("Add %d to r%d", src2_value, inst->rn);
    state->regs[inst->rd] = (int) (state->regs[inst->rn]) + src2_value;
}

void mov(struct state* state, struct dp_instr* instr)
{
    // rd = src2
    int value;
    if (instr->i == 1) {
        debug("Immediate value %d", instr->src2);
        value = instr->src2;
    } else {
        unsigned int shift = select_bits(instr->src2, 11, 8);
        unsigned int reg = select_bits(instr->src2, 7, 0);
        value = state->regs[reg];
        debug("Value (%d) from r%d shifted by %d", value, reg, shift);
        if (shift != 0) {
            value <<= shift;
            debug("Shifted value: %d", value);
        }
    }

    state->regs[instr->rd] = value;
}

void armemu_one_dp(struct state* state, struct dp_instr* inst)
{
    switch (inst->cmd) {
        case 0x2:
            add_or_subtract(state, inst, false);
            break;
        case 0x4:
            add_or_subtract(state, inst, true);
            break;
        case 0x9:
            branch_and_exchange(state, inst);
            break;
        case 0xd:
            mov(state, inst);
            break;
        default:
            fprintf(stderr, "Unknown Data Processing instruction with cmd %02x.\n"
                            "Try: \n"
                            "    objdump -S armemu --start-address 0x%02x\n",
                    inst->cmd, state->regs[PC]);
            exit(EXIT_FAILURE);
    }

}
