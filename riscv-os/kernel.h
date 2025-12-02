#ifndef KERNEL_H
#define KERNEL_H

// Basic types
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int64_t;
typedef int int32_t;

#define NULL ((void*)0)

// Trap frame structure
struct trap_frame {
    uint64_t regs[32];
};

// CSR macros
#define read_csr(reg) ({ \
    unsigned long __tmp; \
    asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; \
})

#define write_csr(reg, val) ({ \
    asm volatile("csrw " #reg ", %0" :: "r"(val)); \
})

// Kernel functions
void kernel_main(void);
void trap_handler(struct trap_frame *frame);
void handle_timer_interrupt(void);
void handle_external_interrupt(void);
void enable_interrupts(void);
void set_timer(unsigned long cycles);

// Process entry points
void init_process(void);
void shell_process(void);
void idle_process(void);

#endif