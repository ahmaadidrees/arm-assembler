/* codegen.c - machine code generation */

#include "armasm.h"

struct codegen_map_st codegen_opcode_map[] = CODEGEN_OPCODE_MAP;
struct codegen_map_st codegen_bcc_map[] = CODEGEN_BCC_MAP;

#define COND_BIT 28

void codegen_error(char *err) {
    printf("codegen_error: %s\n", err);
    exit(-1);
}

void codegen_table_init(struct codegen_table_st *ct, struct parse_node_st *tree) {
    ct->len = 0;
    ct->next = 0;
    ct->tree = tree;
}

void codegen_add_inst(struct codegen_table_st *ct, uint32_t inst) {
    ct->table[ct->len] = inst;
    ct->len += 1;
}

bool codegen_is_public_label(struct codegen_table_st *ct, struct codegen_label_pair *pl) {
    for (int i = 0; i < ct->public_count; i++) {
        if (!strcmp(ct->publics[i].label, pl->label)) {
            return true;
        }
    }
    return false;
}

void codegen_table_add_pair(struct codegen_label_pair *lp, int index, char *label, int offset) {
    strncpy(lp[index].label, label, SCAN_TOKEN_LEN);
    lp[index].offset = offset;
}

void codegen_table_set_pairs(struct codegen_table_st *ct, struct parse_node_st *np, int level) {
    int offset = level * 4;
    if(np->type == INST) {
        if(strlen(np->stmt.inst.label) > 0) {
            codegen_table_add_pair(ct->labels, ct->label_count, np->stmt.inst.label, offset);
            ct->public_count += 1;
        }
    } else if (np->type == SEQ) {
        if (np->stmt.seq.left->type == DIR) {
            codegen_table_add_pair(ct->publics, ct->public_count, np->stmt.seq.left->stmt.dir.label, offset);
            ct->public_count += 1;
        }
        else {
            codegen_table_set_pairs(ct, np->stmt.seq.left, level);
            level+=1;
        }
        codegen_table_set_pairs(ct, np->stmt.seq.right, level);
    }
}

uint32_t codegen_lookup(char *name, struct codegen_map_st *map, int map_len) {
    int i;
    for (i = 0; i < map_len; i++) {
        if (strncmp(name, map[i].name, SCAN_TOKEN_LEN) == 0) {
            return map[i].bits;
        }
    }

    codegen_error(name);
    return (uint32_t) -1;
}

void codegen_elf_write(struct codegen_table_st *ct, char *path) {
    int i;
    elf_context elf;
    struct codegen_label_pair *pl;
    int binding;
    elf_init(&elf);
    for(i = 0; i < ct->label_count; i++) {
        pl = &ct->labels[i];
        if (codegen_is_public_label(ct, pl)) {
            binding = STB_GLOBAL;
        } else {
            binding = STB_LOCAL;
        }
        elf_add_symbol(&elf, pl->label, pl->offset, binding);
    }
    for(i = 0; i < ct->len; i++) {
        elf_add_instr(&elf, ct->table[i]);
    }
    FILE *f = fopen(path, "w");
    if(!f) {
        perror(path);
        return;
    }
    if (!elf_write_file(&elf, f)) {
        printf("elf_write_file failed\n");
    }
    fclose(f);
}

uint32_t codegen_lookup_opcode(char *name) {
    int len = sizeof(codegen_opcode_map) / sizeof(codegen_opcode_map[0]);
    return codegen_lookup(name, codegen_opcode_map, len);
}

uint32_t codegen_lookup_bcc(char *name) {
    int len = sizeof(codegen_bcc_map) / sizeof(codegen_bcc_map[0]);
    return codegen_lookup(name, codegen_bcc_map, len);
}

void codegen_dp_common(struct codegen_table_st *ct, uint32_t imm, uint32_t op, 
    uint32_t rn, uint32_t rd, uint32_t op2) {

    const uint32_t DP_IMM_BIT = 25;
    const uint32_t DP_OP_BIT  = 21;
    const uint32_t DP_RN_BIT  = 16;
    const uint32_t DP_RD_BIT  = 12;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (imm << DP_IMM_BIT)
        | (op  << DP_OP_BIT)
        | (rn  << DP_RN_BIT)
        | (rd  << DP_RD_BIT)
        | op2;
    codegen_add_inst(ct, inst);
}

void codegen_dp3(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_dp_common(
        ct,
        0, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
        np->stmt.inst.dp3.rn,
        np->stmt.inst.dp3.rd,
        np->stmt.inst.dp3.rm);
}

void codegen_dp3i(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_dp_common(
        ct,
        1, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
	    np->stmt.inst.dp3i.rn,
        np->stmt.inst.dp3i.rd,
        np->stmt.inst.dp3i.imm);
}

void codegen_dp2(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_dp_common(
        ct,
        0, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
	    0,
        np->stmt.inst.dp2.rd,
        np->stmt.inst.dp2.rm);
}

void codegen_dp2i(struct codegen_table_st *ct, struct parse_node_st *np) {
   uint32_t op = codegen_lookup_opcode(np->stmt.inst.name);
    if((int)np->stmt.inst.dp2i.imm < 0) {
        op = 0b1111;
        np->stmt.inst.dp2i.imm = ~np->stmt.inst.dp2i.imm;
    }
    codegen_dp_common(
        ct,
        1, /*imm*/
        op,
	    0,
        np->stmt.inst.dp2i.rd,
        np->stmt.inst.dp2i.imm);
}

void codegen_mul_common(struct codegen_table_st *ct, uint32_t rd, 
    uint32_t rs, uint32_t rm) {

    const uint32_t MUL_RD_BIT = 16;
    const uint32_t MUL_RS_BIT = 8;
    const uint32_t MUL_MUL_BIT = 4;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (rd  << MUL_RD_BIT)
        | (rs  << MUL_RS_BIT)
        | (0b1001  << MUL_MUL_BIT)
        | rm;
    codegen_add_inst(ct, inst);
}

void codegen_mul(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_mul_common(
        ct,
        np->stmt.inst.mul.rd,
        np->stmt.inst.mul.rs,
        np->stmt.inst.mul.rm);
}

void codegen_mem_common(struct codegen_table_st *ct, uint32_t imm, 
    uint32_t updown, uint32_t byteword, uint32_t loadstore,
    uint32_t rn, uint32_t rd, uint32_t offset) {

    const uint32_t DP_SDT_BIT = 26;
    const uint32_t DP_IMM_BIT = 25;
    const uint32_t DP_PREPOST_BIT = 24;
    const uint32_t DP_UPDOWN_BIT = 23;
    const uint32_t DP_BYTEWORD_BIT = 22;
    const uint32_t DP_LOADSTORE_BIT = 20;    
    const uint32_t DP_RN_BIT  = 16;
    const uint32_t DP_RD_BIT  = 12;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (0b01 << DP_SDT_BIT)
        | (imm << DP_IMM_BIT)
        | (0b1 << DP_PREPOST_BIT)
        | (updown << DP_UPDOWN_BIT)
        | (byteword << DP_BYTEWORD_BIT)
        | (loadstore << DP_LOADSTORE_BIT)
        | (rn  << DP_RN_BIT)
        | (rd  << DP_RD_BIT)
        | offset;
    codegen_add_inst(ct, inst);
}

void codegen_mem3(struct codegen_table_st *ct, struct parse_node_st *np) {
    uint32_t loadstore = 0, byteword = 0;
    if (strncmp(np->stmt.inst.name, "ldrb", SCAN_TOKEN_LEN) == 0 | strncmp(np->stmt.inst.name, "strb", SCAN_TOKEN_LEN) == 0 ) {
        byteword = 1;
    } 
    if (strncmp(np->stmt.inst.name, "ldr", SCAN_TOKEN_LEN) == 0 | strncmp(np->stmt.inst.name, "ldrb", SCAN_TOKEN_LEN) == 0) {
        loadstore = 1;
    } 
    codegen_mem_common(
        ct,
        0, /* imm */
        1, /* updown */
        byteword, /* byteword */
        loadstore, /* loadstore - need to change when adding str */
        np->stmt.inst.mem3.rn,
        np->stmt.inst.mem3.rd,
        np->stmt.inst.mem3.rm);
}

void codegen_memi(struct codegen_table_st *ct, struct parse_node_st *np) {
    uint32_t loadstore = 0, byteword = 0;
    
    if (strncmp(np->stmt.inst.name, "ldrb", SCAN_TOKEN_LEN) == 0 || strncmp(np->stmt.inst.name, "strb", SCAN_TOKEN_LEN) == 0 ) {
        byteword = 1;
    } 
    if (strncmp(np->stmt.inst.name, "ldr", SCAN_TOKEN_LEN) == 0 || strncmp(np->stmt.inst.name, "ldrb", SCAN_TOKEN_LEN) == 0) {
        loadstore = 1;
    } 

    codegen_mem_common(
        ct,
        0, /* imm*/
        1, /* updown */
        byteword, /* byteword */
        loadstore, /* loadstore */
        np->stmt.inst.memi.rn,
        np->stmt.inst.memi.rd,
        np->stmt.inst.memi.imm);
}

void codegen_cmp_common(struct codegen_table_st *ct, uint32_t imm, uint32_t op, uint32_t set,
    uint32_t rn, uint32_t rm) {

    const uint32_t CMP_IMM_BIT = 25;
    const uint32_t CMP_OP_BIT  = 21;
    const uint32_t CMP_SET_BIT = 20;
    const uint32_t CMP_RN_BIT  = 16;
    const uint32_t DP_RM_BIT  = 0;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (imm << CMP_IMM_BIT)
        | (op  << CMP_OP_BIT)
        | (set << CMP_SET_BIT)
        | (rn  << CMP_RN_BIT)
        | (rm  << DP_RM_BIT); 
    codegen_add_inst(ct, inst);
}

void codegen_cmp(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_cmp_common(
        ct,
        0, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
        1, /*s*/
        np->stmt.inst.cmp.rn,
        np->stmt.inst.cmp.rm);
}

void codegen_cmpi(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_cmp_common(
        ct,
        1, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
        1, /*s*/
        np->stmt.inst.cmpi.rn,
        np->stmt.inst.cmpi.imm);
}

void codegen_lsl_common(struct codegen_table_st *ct, uint32_t op, uint32_t rd, 
    uint32_t imm, uint32_t rm) {

    const uint32_t LSL_OP_BIT = 21;
    const uint32_t LSL_RD_BIT  = 12;
    const uint32_t LSL_IMM_BIT  = 8;
    const uint32_t LSL_RM_BIT  = 0;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (op << LSL_OP_BIT)
        | (rd  << LSL_RD_BIT)
        | (imm  << LSL_IMM_BIT)
        | (rm  << LSL_RM_BIT); 
    codegen_add_inst(ct, inst);
}

void codegen_lsli(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_lsl_common(
        ct,
        codegen_lookup_opcode(np->stmt.inst.name),
        np->stmt.inst.lsli.rd,
        1, /*imm*/ 
        np->stmt.inst.lsli.rm);
}

int codegen_get_index(struct parse_node_st *np, char* label, int level) {
    int l1 = -1;
    int l2 = -1;
    
    if (np -> type == INST) {
        if (strncmp(label,np -> stmt.inst.label, SCAN_TOKEN_LEN) == 0) {
            l1 = level;
        }
    } else if (np -> type == SEQ) {
        int new_level;
        l1 = codegen_get_index(np -> stmt.seq.left, label, level);
        if (np -> stmt.seq.left -> type == DIR) {
            new_level = level;
        } else {
            new_level = level + 1;
        }
         l2 = codegen_get_index(np -> stmt.seq.right, label, new_level);
        if (l1 == -1) {
            l1 = l2;
        }
    }
    return l1;
}

void codegen_b(struct codegen_table_st *ct, struct parse_node_st *np) {
    uint32_t branchlink = 0;
    if (strncmp(np -> stmt.inst.name, "bl", SCAN_TOKEN_LEN) == 0) {
      branchlink = 1;
    }

    int index = (ct -> len);
    int target_index = codegen_get_index(ct -> tree, np -> stmt.inst.b.label, 0);

    int offset = target_index - (index + 2);
    uint32_t bc_code = codegen_lookup_bcc(np -> stmt.inst.name);

    uint32_t inst = (bc_code << COND_BIT)
        | (0b101 << 25)
        | (branchlink << 24)
        | (offset & 0xFFFFFF);
    codegen_add_inst(ct, inst);
}


void codegen_bx(struct codegen_table_st *ct, struct parse_node_st *np) {
    const uint32_t BX_CODE_BIT = 4;
    const uint32_t bx_code = 0b000100101111111111110001;

    uint32_t inst = (COND_AL << COND_BIT)
        | (bx_code << BX_CODE_BIT)
        | np->stmt.inst.bx.rn;
    codegen_add_inst(ct, inst);
}

void codegen_inst(struct codegen_table_st *ct, struct parse_node_st *np) {
    if (np->type == DIR) {
        codegen_table_add_pair(ct->labels, ct->label_count, np->stmt.inst.label, ct->len * 4);
        ct->label_count++;
    }
    switch (np->stmt.inst.type) {
        case DP3  : codegen_dp3(ct, np); break;
        case DP3I : codegen_dp3i(ct, np); break;
        case DP2  : codegen_dp2(ct, np); break;
        case DP2I : codegen_dp2i(ct, np); break;
        case MUL  : codegen_mul(ct, np); break;
	    case CMP  : codegen_cmp(ct, np); break;
        case CMPI : codegen_cmpi(ct, np); break;
        case LSI  : codegen_lsli(ct, np); break;
        case MEM3 : codegen_mem3(ct, np); break;
        case MEMI : codegen_memi(ct, np); break;
        case BX   : codegen_bx(ct, np); break;
	    case B    : codegen_b(ct, np); break;
        default   : codegen_error("unknown stmt.inst.type");
    }
}

void codegen_stmt(struct codegen_table_st *ct, struct parse_node_st *np) {

    if (np->type == INST) {
        codegen_inst(ct, np);
    } else if (np->type == SEQ) {
        codegen_stmt(ct, np->stmt.seq.left);
        codegen_stmt(ct, np->stmt.seq.right);
    } else if (np->type == DIR) {
        codegen_table_add_pair(ct->publics, ct->public_count, np->stmt.dir.label, ct->len * 4);
        ct->public_count++;
    }
}

void codegen_print_hex(struct codegen_table_st *ct) {
    int i;

    printf("v2.0 raw\n");
    for (i = 0; i < ct->len; i++) {
        printf("%08X\n", ct->table[i]);
    }
}
void codegen_print_obj(struct codegen_table_st *ct) {
    int i;

    printf("v2.0 raw\n");
    for (i = 0; i < ct->len; i++) {
        printf("%08X\n", ct->table[i]);
    }
}

void codegen_write(struct codegen_table_st *ct, char *path) {
    int i;
    FILE *obj = fopen(path, "w");

    fprintf(obj, "v2.0 raw\n");
    for (i = 0; i < ct->len; i++) {
        fprintf(obj, "%08X\n", ct->table[i]);
    }
    fclose(obj);
}

void codegen_hex_write(struct codegen_table_st *ct, struct parse_node_st *np, char *path) {
    codegen_write(ct, path);
}

void codegen_obj_write(struct codegen_table_st *ct, struct parse_node_st *np, char *path) {
    printf("path: %s\n", path);
    codegen_elf_write(ct, path);
}