OUTPUT_ARCH(arm)
ENTRY(minemu_bootstrap_high)

PHDRS
{
  bootstrap PT_LOAD FLAGS(5);
  kernel PT_LOAD FLAGS(7);
}

SECTIONS
{
  . = 0x40008000;
  .bootstrap :
  {
    KEEP(*(.bootstrap.text))
  } :bootstrap

  . = 0xc0030000;
  .vectors : AT(0x40030000)
  {
    KEEP(*(.vectors))
  } :kernel

  .text : AT(ADDR(.text) - 0x80000000)
  {
    *(.text.startup)
    *(.text*)
  } :kernel

  .rodata : AT(ADDR(.rodata) - 0x80000000)
  {
    *(.rodata*)
  } :kernel

  .data : AT(ADDR(.data) - 0x80000000)
  {
    *(.data*)
  } :kernel

  .bss (NOLOAD) :
  {
    *(.bss*)
    *(COMMON)
  } :kernel

  . = ALIGN(8);
  .stacks (NOLOAD) :
  {
    . += 4096;
    __minemu_svc_stack_top = .;
    . += 4096;
    __minemu_irq_stack_top = .;
    . += 4096;
    __minemu_abt_stack_top = .;
    . += 4096;
    __minemu_und_stack_top = .;
  } :kernel

  __minemu_svc_stack_top_paddr = __minemu_svc_stack_top - 0x80000000;
  __minemu_irq_stack_top_paddr = __minemu_irq_stack_top - 0x80000000;
  __minemu_abt_stack_top_paddr = __minemu_abt_stack_top - 0x80000000;
  __minemu_und_stack_top_paddr = __minemu_und_stack_top - 0x80000000;

  /* The user loader initializes these pages before mapping them into USR.
   * Reserve their addresses without making the Boot ROM zero the whole pool.
   */
  . = ALIGN(4096);
  .user_pages (NOLOAD) : { *(.user_pages) } :NONE
  ASSERT(. <= 0xc4000000, "kernel storage exceeds the RAM direct map")
}
