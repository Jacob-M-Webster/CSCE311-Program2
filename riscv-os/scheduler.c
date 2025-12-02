#include "scheduler.h"
#include "process.h"
#include "uart.h"

static int scheduler_running = 0;

void scheduler_start(void) {
    scheduler_running = 1;
    
    // Simple cooperative multitasking - just call each process function directly
    // In a real OS, we would switch contexts properly
    
    // The init process runs first (already called before scheduler starts)
    // Now manually call shell process
    struct process *shell = get_process_by_pid(2);
    if (shell) {
        shell->state = PROC_RUNNING;
        set_current_process(shell);
        shell->entry();  // This will run the shell demo
    }
    
    // After shell exits, we can fall through to idle
    uart_puts("[SCHEDULER] All processes complete\n");
}

void schedule(void) {
    // Simplified scheduler for this demo
    // In a real OS, this would save/restore full context
    if (!scheduler_running) return;
    
    process_wake_sleeping();
}

void yield(void) {
    schedule();
}