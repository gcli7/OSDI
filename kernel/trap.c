#include <kernel/trap.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <inc/assert.h>
#include <inc/mmu.h>
#include <inc/x86.h>
#include <kernel/cpu.h>

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;


/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t)idt
};

/* Trap handlers */
TrapHandler trap_hnd[256] = { 0 };

static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}

void
print_trapframe(struct Trapframe *tf)
{
	printk("TRAP frame at %p \n", tf);
	print_regs(&tf->tf_regs);
	printk("  es   0x----%04x\n", tf->tf_es);
	printk("  ds   0x----%04x\n", tf->tf_ds);
	printk("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		printk("  cr2  0x%08x\n", rcr2());
	printk("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		printk(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		printk("\n");
	printk("  eip  0x%08x\n", tf->tf_eip);
	printk("  cs   0x----%04x\n", tf->tf_cs);
	printk("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		printk("  esp  0x%08x\n", tf->tf_esp);
		printk("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void page_fault_handler(struct Trapframe *tf) {
	cprintf("[0756118] Page fault @ 0x%08x\n", rcr2());
	while (1);
}

/* For debugging */
void
print_regs(struct PushRegs *regs)
{
	printk("  edi  0x%08x\n", regs->reg_edi);
	printk("  esi  0x%08x\n", regs->reg_esi);
	printk("  ebp  0x%08x\n", regs->reg_ebp);
	printk("  oesp 0x%08x\n", regs->reg_oesp);
	printk("  ebx  0x%08x\n", regs->reg_ebx);
	printk("  edx  0x%08x\n", regs->reg_edx);
	printk("  ecx  0x%08x\n", regs->reg_ecx);
	printk("  eax  0x%08x\n", regs->reg_eax);
}

void register_handler(int trapno, TrapHandler hnd, void (*trap_entry)(void), int isTrap, int dpl)
{
	if (trapno >= 0 && trapno < 256 && trap_entry != NULL)
	{
		trap_hnd[trapno] = hnd;
		/* Set trap gate */
		SETGATE(idt[trapno], isTrap, GD_KT, trap_entry, dpl);
	}
}

//
// Restores the register values in the Trapframe with the 'iret' instruction.
// This exits the kernel and starts executing some environment's code.
//
// This function does not return.
//
void
env_pop_tf(struct Trapframe *tf)
{
	__asm __volatile("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		: : "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}

static void
trap_dispatch(struct Trapframe *tf)
{
    /*       Handle specific interrupts.
     *       You need to check the interrupt number in order to tell
     *       which interrupt is currently happening since every interrupt
     *       comes to this function called by default_trap_handler.
     *
     * NOTE: Checkout the Trapframe data structure for interrupt number,
     *       which we had pushed into the stack when going through the
     *       declared interface in trap_entry.S
     *
     *       The interrupt number is defined in inc/trap.h
     *
     *       We prepared the keyboard handler and timer handler for you
     *       already. Please reference in kernel/kbd.c and kernel/timer.c
     */
     /*
	switch (tf->tf_trapno) {
		case IRQ_OFFSET + IRQ_TIMER:
			timer_handler();
			break;
		case IRQ_OFFSET + IRQ_KBD:
			kbd_intr();
			break;
		case T_PGFLT:
		page_fault_handler(tf);
			break;
		default:
			// Unexpected trap: The user process or the kernel has a bug.
			print_trapframe(tf);
	}
	*/
	// Handle spurious interrupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
		printk("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}

	last_tf = tf;
	/* Lab3: Check the trap number and call the interrupt handler. */
	if (trap_hnd[tf->tf_trapno] != NULL)
	{
	
		if ((tf->tf_cs & 3) == 3)
		{
			// Disable interrupt first
			// Think: Why we disable interrupt here?
			__asm __volatile("cli");

			// Copy trap frame (which is currently on the stack)
			// into 'thiscpu->cpu_task->tf', so that running the environment
			// will restart at the trap point.
			thiscpu->cpu_task->tf = *tf;
			tf = &(thiscpu->cpu_task->tf);
				
		}
		// Do ISR
		trap_hnd[tf->tf_trapno](tf);
		
		// Pop the kernel stack 
		env_pop_tf(tf);
		return;
	}

	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	panic("Unexpected trap!");
	
}

void default_trap_handler(struct Trapframe *tf)
{
	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);
}

void trap_init()
{
    /*       You should initialize the interrupt descriptor table.
     *       You should setup at least keyboard interrupt and timer interrupt as
     *       the lab's requirement.
     *
     *       Noted that there is another file kernel/trap_entry.S, in which places
     *       all the entry of interrupt handler.
     *       Thus, you can declare an interface there by macro providing there and
     *       use that function pointer when setting up the corresponding IDT entry.
     *
     *       By doing so, we can have more flexibility in adding new IDT entry and
     *       reuse the routine when interrupt occurs.
     *
     *       Remember to load you IDT with x86 assembly instruction lidt.
     *
     * Note:
     *       You might be benefitted from the macro SETGATE inside mmu.h
     *       There are defined macros for Segment Selectors in mmu.h
     *       Also, check out inc/x86.h for easy-to-use x86 assembly instruction
     *       There is a data structure called Pseudodesc in mmu.h which might
     *       come in handy for you when filling up the argument of "lidt"
     */
	/* Keyboard interrupt setup */
	//extern void isr_kbd();
	//SETGATE(idt[IRQ_OFFSET + IRQ_KBD], 0, GD_KT, isr_kbd, 0);
	/* Timer Trap setup */
	//extern void isr_timer();
	//SETGATE(idt[IRQ_OFFSET + IRQ_TIMER], 1, GD_KT, isr_timer, 0);
	/* Page_fault trap setup */
	//extern void isr_page_fault();
	//SETGATE(idt[T_PGFLT], 1, GD_KT, isr_page_fault, 0);
	/* Load IDT */
	//lidt(&pseudo_idt);
	int i;
	/* Initial interrupt descrip table for all traps */
	extern void Default_ISR();
	for (i = 0; i < 256; i++)
	{
		SETGATE(idt[i], 1, GD_KT, Default_ISR, 0);
		trap_hnd[i] = NULL;
	}


    /* Using default_trap_handler */
	extern void GPFLT();
	SETGATE(idt[T_GPFLT], 1, GD_KT, GPFLT, 0);
	extern void STACK_ISR();
	SETGATE(idt[T_STACK], 1, GD_KT, STACK_ISR, 0);

    /* Using custom trap handler */
	extern void PGFLT();
    register_handler(T_PGFLT, page_fault_handler, PGFLT, 1, 0);

	lidt(&idt_pd);
}
