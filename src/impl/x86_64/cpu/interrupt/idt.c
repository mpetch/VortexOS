#include "idt.h"
#include "isr.h"
#include "io.h"
#include "vga.h"
#include <stdint.h>

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

void (*interrupt_handlers[IDT_ENTRIES])();

extern void load_segment_selectors();
extern void idt_load(uint64_t);

static void idt_set_gate(int num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

static void pic_remap(int offset1, int offset2) {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA); // Save masks
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Start initialization sequence (in cascade mode)
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
    io_wait();
    outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
    io_wait();
    outb(PIC1_DATA, 4); // ICW3: Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    outb(PIC2_DATA, 2); // ICW3: Tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1); // Restore saved masks.
    outb(PIC2_DATA, a2);  // Get current mask
}

void register_interrupt_handler(uint8_t n, void (*handler)()) {
    interrupt_handlers[n] = handler;
}

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0, (uint64_t)isr0_stub, 0x08, 0x8E);
    idt_set_gate(1, (uint64_t)isr1_stub, 0x08, 0x8E);
    idt_set_gate(8, (uint64_t)isr8_stub, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13_stub, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14_stub, 0x08, 0x8E);
    idt_set_gate(32, (uint64_t)isr32_stub, 0x08, 0x8E);
    idt_set_gate(33, (uint64_t)isr33_stub, 0x08, 0x8E);
    idt_set_gate(34, (uint64_t)isr34_stub, 0x08, 0x8E);
    idt_set_gate(35, (uint64_t)isr35_stub, 0x08, 0x8E);
    idt_set_gate(36, (uint64_t)isr36_stub, 0x08, 0x8E);
    idt_set_gate(37, (uint64_t)isr37_stub, 0x08, 0x8E);
    idt_set_gate(38, (uint64_t)isr38_stub, 0x08, 0x8E);
    idt_set_gate(39, (uint64_t)isr39_stub, 0x08, 0x8E);
    idt_set_gate(40, (uint64_t)isr40_stub, 0x08, 0x8E);
    idt_set_gate(41, (uint64_t)isr41_stub, 0x08, 0x8E);
    idt_set_gate(42, (uint64_t)isr42_stub, 0x08, 0x8E);

    pic_remap(0x20, 0x28);

    idt_load((uint64_t)&idtp);

    load_segment_selectors();

    enable_interrupts();
}