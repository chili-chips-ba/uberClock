# See LICENSE for license details.

#include "riscv_test.h"
#undef RVTEST_RV64S
#define RVTEST_RV64S RVTEST_RV32M
#define __MACHINE_MODE


#include "test_macros.h"

RVTEST_RV64S
RVTEST_CODE_BEGIN

#ifdef __MACHINE_MODE
  #define sscratch mscratch
  #define sstatus mstatus
  #define scause mcause
  #define sepc mepc
  #define sret mret
  #define stvec_handler mtvec_handler
  #undef SSTATUS_SPP
  #define SSTATUS_SPP MSTATUS_MPP
#endif

  # For RV64, make sure UXL encodes RV64.  (UXL does not exist for RV32.)
#if __riscv_xlen == 64
  # If running in M mode, use mstatus.MPP to check existence of U mode.
  # Otherwise, if in S mode, then U mode must exist and we don't need to check.
#ifdef __MACHINE_MODE
  li t0, MSTATUS_MPP
  csrc mstatus, t0
  csrr t1, mstatus
  and t0, t0, t1
  bnez t0, 1f
#endif
  # If U mode is present, UXL should be 2 (XLEN = 64-bit)
  TEST_CASE(18, a0, SSTATUS_UXL & (SSTATUS_UXL << 1), csrr a0, sstatus; li a1, SSTATUS_UXL; and a0, a0, a1)
#ifdef __MACHINE_MODE
  j 2f
1:
  # If U mode is not present, UXL should be 0
  TEST_CASE(19, a0, 0, csrr a0, sstatus; li a1, SSTATUS_UXL; and a0, a0, a1)
2:
#endif
#endif

  TEST_CASE(20, a0,         0, csrw sscratch, zero; csrr a0, sscratch);
  TEST_CASE(21, a0,         0, csrrwi a0, sscratch, 0; csrrwi a0, sscratch, 0xF);
  TEST_CASE(22, a0,      0x1f, csrrsi x0, sscratch, 0x10; csrr a0, sscratch);

  csrwi sscratch, 3
  TEST_CASE( 2, a0,         3, csrr a0, sscratch);
  TEST_CASE( 3, a1,         3, csrrci a1, sscratch, 1);
  TEST_CASE( 4, a2,         2, csrrsi a2, sscratch, 4);
  TEST_CASE( 5, a3,         6, csrrwi a3, sscratch, 2);
  TEST_CASE( 6, a1,         2, li a0, 0xbad1dea; csrrw a1, sscratch, a0);
  TEST_CASE( 7, a1, 0xbad1dea, li a0, 0x0001dea; csrrc a1, sscratch, a0);
  TEST_CASE( 8, a1, 0xbad0000, li a0, 0x000beef; csrrs a1, sscratch, a0);
  TEST_CASE( 9, a0, 0xbadbeef, li a0, 0xbad1dea; csrrw a0, sscratch, a0);
  TEST_CASE(10, a0, 0xbad1dea, li a0, 0x0001dea; csrrc a0, sscratch, a0);
  TEST_CASE(11, a0, 0xbad0000, li a0, 0x000beef; csrrs a0, sscratch, a0);
  TEST_CASE(12, a0, 0xbadbeef, csrr a0, sscratch);

#ifdef __MACHINE_MODE
  # Is F extension present?
  csrr a0, misa
  andi a0, a0, (1 << ('F' - 'A'))
  beqz a0, 1f
  # If so, make sure FP stores have no effect when mstatus.FS is off.
  li a1, MSTATUS_FS
  csrs mstatus, a1
#ifdef __riscv_flen
  fmv.s.x f0, x0
  csrc mstatus, a1
  la a1, fsw_data
  TEST_CASE(13, a0, 1, fsw f0, (a1); lw a0, (a1));
#else
  # Fail if this test is compiled without F but executed on a core with F.
  TEST_CASE(13, zero, 1)
#endif
1:

  # Figure out if 'U' is set in misa
  csrr a0, misa   # a0 = csr(misa)
  srli a0, a0, 20 # a0 = a0 >> 20
  andi a0, a0, 1  # a0 = a0 & 1
  beqz a0, finish # if no user mode, skip the rest of these checks

  # Enable access to the cycle counter
  csrwi mcounteren, 1

  # Figure out if 'S' is set in misa
  csrr a0, misa   # a0 = csr(misa)
  srli a0, a0, 18 # a0 = a0 >> 20
  andi a0, a0, 1  # a0 = a0 & 1
  beqz a0, 1f

  # Enable access to the cycle counter
  csrwi scounteren, 1
1:
#endif /* __MACHINE_MODE */

  # jump to user land
  li t0, SSTATUS_SPP
  csrc sstatus, t0
  la t0, 1f
  csrw sepc, t0
  sret
  1:

  # Make sure writing the cycle counter causes an exception.
  # Don't run in supervisor, as we don't delegate illegal instruction traps.
#ifdef __MACHINE_MODE
  TEST_CASE(14, a0, 255, li a0, 255; csrrw a0, cycle, x0);
#endif

  # Make sure reading status in user mode causes an exception.
  # Don't run in supervisor, as we don't delegate illegal instruction traps.
#ifdef __MACHINE_MODE
  TEST_CASE(15, a0, 255, li a0, 255; csrr a0, sstatus)
#else
  TEST_CASE(15, x0, 0, nop)
#endif

finish:
  RVTEST_PASS

  # We should only fall through to this if scall failed.
  TEST_PASSFAIL

  .align 2
  .global stvec_handler
stvec_handler:
  # Trapping on tests 13-15 is good news.
  li t0, 13
  bltu TESTNUM, t0, 1f
  li t0, 15
  bleu TESTNUM, t0, privileged
1:

  # catch RVTEST_PASS and kick it up to M-mode
  csrr t0, scause
  li t1, CAUSE_USER_ECALL
  bne t0, t1, fail
  RVTEST_PASS

privileged:
  # Make sure scause indicates a lack of privilege.
  csrr t0, scause
  li t1, CAUSE_ILLEGAL_INSTRUCTION
  bne t0, t1, fail
  # Return to user mode, but skip the trapping instruction.
  csrr t0, sepc
  addi t0, t0, 4
  csrw sepc, t0
  sret

RVTEST_CODE_END

  .data
RVTEST_DATA_BEGIN

fsw_data: .word 1

RVTEST_DATA_END

