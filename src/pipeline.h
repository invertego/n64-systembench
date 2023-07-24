#pragma once

typedef struct {
    const uint32_t* imem_ptr;
    uint32_t pc_start;
    uint32_t pc_end;

    uint8_t lock[32];
    uint8_t vlock[32];
    bool delay_slot;
    bool broken;
    
    struct {
        uint32_t pc;
        bool single_issue;
    } if_in;

    struct {
        struct {
            uint32_t pc;
            uint32_t inst;
            bool bubble;
        } su;
        struct {
            uint32_t pc;
            uint32_t inst;
            bool bubble;
        } vu;
    } rd_in;

    struct {
        struct {
            uint32_t pc;
            int opc;
            int r_out;
            int v_out;
            bool bubble;
        } su;
        struct {
            uint32_t pc;
            int opc;
            int v_out;
            bool bubble;
        } vu;
    } ex_mul_in;

    struct {
        struct {
            int opc;
            int r_out;
            int v_out;
        } su;
        struct {
            int opc;
            int v_out;
        } vu;
    } df_acc_in;

    struct {
        struct {
            int opc;
            int r_out;
            int v_out;
        } su;
        struct {
            int opc;
            int v_out;
        } vu;
    } wb_in;

    struct {
        struct {
            int opc;
            int v_out;
        } su;
        struct {
            int opc;
            int v_out;
        } vu;
    } wb_out;
} pipeline_t;

void pipeline_init(pipeline_t* p);
void pipeline_step(pipeline_t* p);
