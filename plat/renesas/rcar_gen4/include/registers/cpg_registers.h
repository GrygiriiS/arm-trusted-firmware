/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPG_REGISTERS_H
#define CPG_REGISTERS_H

/* CPG base address */
#define CPG_BASE	0xE6150000U
#define CPG_CPGWPR	(CPG_BASE)
#define	CPG_CPGWPCR	(CPG_BASE + 0x0004U)

#define	FRQCRB    (CPG_BASE + 0x0804)
#define	FRQCRC0   (CPG_BASE + 0x0808)
#define	FRQCRD    (CPG_BASE + 0x080C)
#define	PLLECR    (CPG_BASE + 0x0820)
#define	PLL1CR0   (CPG_BASE + 0x0830)
#define	PLL2CR0   (CPG_BASE + 0x0834)
#define	PLL3CR0   (CPG_BASE + 0x083C)
#define	PLL4CR0   (CPG_BASE + 0x0844)
#define	PLL6CR0   (CPG_BASE + 0x084C)
#define	PLL1STPCR (CPG_BASE + 0x0850)
#define	PLL2STPCR (CPG_BASE + 0x0854)
#define	PLL3STPCR (CPG_BASE + 0x085C)
#define	PLL4STPCR (CPG_BASE + 0x0864)
#define	PLL6STPCR (CPG_BASE + 0x086C)

#define	SD0CKCR          (CPG_BASE + 0x0870)
#define	RPCCKCR          (CPG_BASE + 0x0874)
#define	CANFDCKCR        (CPG_BASE + 0x0878)
#define	MSOCKCR          (CPG_BASE + 0x087C)
#define	CSICKCR          (CPG_BASE + 0x0880)
#define	DSIEXTCKCR       (CPG_BASE + 0x0884)
#define	POSTCKCR         (CPG_BASE + 0x0890)
#define	POST2CKCR        (CPG_BASE + 0x0894)
#define	POST3CKCR        (CPG_BASE + 0x0898)
#define	POST4CKCR        (CPG_BASE + 0x089C)

#define CKSRCSELCR         (CPG_BASE + 0x08A4)

/* Module Standby, Software Reset */
#define MSSR_BASE	CPG_BASE

/* Software Reset Register */
#define MSSR_SRCR(n)	(MSSR_BASE + 0x2C00U + (n) * 4)
/* Software Reset Clearing Register */
#define MSSR_SRSTCLR(n)	(MSSR_BASE + 0x2C80U + (n) * 4)
/* Module Stop Control Register */
#define MSSR_MSTPCR(n)	(MSSR_BASE + 0x2D00U + (n) * 4)
/* Module Stop Status Register */
#define MSSR_MSTPSR(n)	(MSSR_BASE + 0x2E00U + (n) * 4)

#endif /* CPG_REGISTERS_H */
