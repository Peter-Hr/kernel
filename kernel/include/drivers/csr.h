// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Charles University

#ifndef _DRIVERS_CSR_H_
#define _DRIVERS_CSR_H_

/** Bitmask that signifies that the MPP
 *  (machine previous privilege) is set to M (machine mode) in MSTATUS CSR
 */
#define CSR_MSTATUS_MPP_M 0x1800

/** The CSR number of the MEPC (machine exception program counter) CSR */
#define CSR_MEPC 0x341
/** The CSR number of the MCAUSE (machine cause) CSR */
#define CSR_MCAUSE 0x342
/** The CSR number of the MTVAL (machine trap value) CSR */
#define CSR_MTVAL 0x343

#endif
