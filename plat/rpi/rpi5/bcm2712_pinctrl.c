/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <platform_def.h>
#include <common/debug.h>
#include <drivers/pinctrl.h>
#include <lib/mmio.h>
#include <lib/spinlock.h>

/* HW defs */
#define BCM2712_PFC_BASE U(0x1001504318)

#define BCM2712_PULL_NONE	0
#define BCM2712_PULL_DOWN	1
#define BCM2712_PULL_UP		2
#define BCM2712_PULL_MASK	0x3

#define BCM2712_FSEL_COUNT 9
#define BCM2712_FSEL_MASK  0xf

#define MUX_BIT_VALID	0x8000
#define REG_BIT_INVALID	0xffff

#define BIT_TO_REG(b) (((b) >> 5) << 2)
#define BIT_TO_SHIFT(b) ((b) & 0x1f)

#define MUX_BIT(mr, mb) (MUX_BIT_VALID + ((mr)*4)*8 + (mb)*4)
#define GPIO_REGS(n, mr, mb, pr, pb) \
	[n] = { MUX_BIT(mr, mb), ((pr)*4)*8 + (pb)*2 }

#define EMMC_REGS(n, pr, pb) \
	[n] = { 0, ((pr)*4)*8 + (pb)*2 }

static spinlock_t bcm2712_pfc_lock;

struct bcm2712_pin_regs {
	uint16_t mux_bit;
	uint16_t pad_bit;
};

static const struct bcm2712_pin_regs bcm2712_c0_gpio_pin_regs[] = {
	GPIO_REGS(0, 0, 0, 7, 7),
	GPIO_REGS(1, 0, 1, 7, 8),
	GPIO_REGS(2, 0, 2, 7, 9),
	GPIO_REGS(3, 0, 3, 7, 10),
	GPIO_REGS(4, 0, 4, 7, 11),
	GPIO_REGS(5, 0, 5, 7, 12),
	GPIO_REGS(6, 0, 6, 7, 13),
	GPIO_REGS(7, 0, 7, 7, 14),
	GPIO_REGS(8, 1, 0, 8, 0),
	GPIO_REGS(9, 1, 1, 8, 1),
	GPIO_REGS(10, 1, 2, 8, 2),
	GPIO_REGS(11, 1, 3, 8, 3),
	GPIO_REGS(12, 1, 4, 8, 4),
	GPIO_REGS(13, 1, 5, 8, 5),
	GPIO_REGS(14, 1, 6, 8, 6),
	GPIO_REGS(15, 1, 7, 8, 7),
	GPIO_REGS(16, 2, 0, 8, 8),
	GPIO_REGS(17, 2, 1, 8, 9),
	GPIO_REGS(18, 2, 2, 8, 10),
	GPIO_REGS(19, 2, 3, 8, 11),
	GPIO_REGS(20, 2, 4, 8, 12),
	GPIO_REGS(21, 2, 5, 8, 13),
	GPIO_REGS(22, 2, 6, 8, 14),
	GPIO_REGS(23, 2, 7, 9, 0),
	GPIO_REGS(24, 3, 0, 9, 1),
	GPIO_REGS(25, 3, 1, 9, 2),
	GPIO_REGS(26, 3, 2, 9, 3),
	GPIO_REGS(27, 3, 3, 9, 4),
	GPIO_REGS(28, 3, 4, 9, 5),
	GPIO_REGS(29, 3, 5, 9, 6),
	GPIO_REGS(30, 3, 6, 9, 7),
	GPIO_REGS(31, 3, 7, 9, 8),
	GPIO_REGS(32, 4, 0, 9, 9),
	GPIO_REGS(33, 4, 1, 9, 10),
	GPIO_REGS(34, 4, 2, 9, 11),
	GPIO_REGS(35, 4, 3, 9, 12),
	GPIO_REGS(36, 4, 4, 9, 13),
	GPIO_REGS(37, 4, 5, 9, 14),
	GPIO_REGS(38, 4, 6, 10, 0),
	GPIO_REGS(39, 4, 7, 10, 1),
	GPIO_REGS(40, 5, 0, 10, 2),
	GPIO_REGS(41, 5, 1, 10, 3),
	GPIO_REGS(42, 5, 2, 10, 4),
	GPIO_REGS(43, 5, 3, 10, 5),
	GPIO_REGS(44, 5, 4, 10, 6),
	GPIO_REGS(45, 5, 5, 10, 7),
	GPIO_REGS(46, 5, 6, 10, 8),
	GPIO_REGS(47, 5, 7, 10, 9),
	GPIO_REGS(48, 6, 0, 10, 10),
	GPIO_REGS(49, 6, 1, 10, 11),
	GPIO_REGS(50, 6, 2, 10, 12),
	GPIO_REGS(51, 6, 3, 10, 13),
	GPIO_REGS(52, 6, 4, 10, 14),
	GPIO_REGS(53, 6, 5, 11, 0),
	EMMC_REGS(54, 11, 1), /* EMMC_CMD */
	EMMC_REGS(55, 11, 2), /* EMMC_DS */
	EMMC_REGS(56, 11, 3), /* EMMC_CLK */
	EMMC_REGS(57, 11, 4), /* EMMC_DAT0 */
	EMMC_REGS(58, 11, 5), /* EMMC_DAT1 */
	EMMC_REGS(59, 11, 6), /* EMMC_DAT2 */
	EMMC_REGS(60, 11, 7), /* EMMC_DAT3 */
	EMMC_REGS(61, 11, 8), /* EMMC_DAT4 */
	EMMC_REGS(62, 11, 9), /* EMMC_DAT5 */
	EMMC_REGS(63, 11, 10), /* EMMC_DAT6 */
	EMMC_REGS(64, 11, 11), /* EMMC_DAT7 */
};


enum bcm2712_pfc_funcs {
	func_gpio,
	func_alt1,
	func_alt2,
	func_alt3,
	func_alt4,
	func_alt5,
	func_alt6,
	func_alt7,
	func_alt8,
	func_aon_cpu_standbyb,
	func_aon_fp_4sec_resetb,
	func_aon_gpclk,
	func_aon_pwm,
	func_arm_jtag,
	func_aud_fs_clk0,
	func_avs_pmu_bsc,
	func_bsc_m0,
	func_bsc_m1,
	func_bsc_m2,
	func_bsc_m3,
	func_clk_observe,
	func_ctl_hdmi_5v,
	func_enet0,
	func_enet0_mii,
	func_enet0_rgmii,
	func_ext_sc_clk,
	func_fl0,
	func_fl1,
	func_gpclk0,
	func_gpclk1,
	func_gpclk2,
	func_hdmi_tx0_auto_i2c,
	func_hdmi_tx0_bsc,
	func_hdmi_tx1_auto_i2c,
	func_hdmi_tx1_bsc,
	func_i2s_in,
	func_i2s_out,
	func_ir_in,
	func_mtsif,
	func_mtsif_alt,
	func_mtsif_alt1,
	func_pdm,
	func_pkt,
	func_pm_led_out,
	func_sc0,
	func_sd0,
	func_sd2,
	func_sd_card_a,
	func_sd_card_b,
	func_sd_card_c,
	func_sd_card_d,
	func_sd_card_e,
	func_sd_card_f,
	func_sd_card_g,
	func_spdif_out,
	func_spi_m,
	func_spi_s,
	func_sr_edm_sense,
	func_te0,
	func_te1,
	func_tsio,
	func_uart0,
	func_uart1,
	func_uart2,
	func_usb_pwr,
	func_usb_vbus,
	func_uui,
	func_vc_i2c0,
	func_vc_i2c3,
	func_vc_i2c4,
	func_vc_i2c5,
	func_vc_i2csl,
	func_vc_pcm,
	func_vc_pwm0,
	func_vc_pwm1,
	func_vc_spi0,
	func_vc_spi3,
	func_vc_spi4,
	func_vc_spi5,
	func_vc_uart0,
	func_vc_uart2,
	func_vc_uart3,
	func_vc_uart4,
	func__,
	func_count = func__
};

#define FUNC(f) \
	[func_##f] = "f_" #f

static const char * const bcm2712_pfc_func_names[] = {
	FUNC(gpio),
	FUNC(alt1),
	FUNC(alt2),
	FUNC(alt3),
	FUNC(alt4),
	FUNC(alt5),
	FUNC(alt6),
	FUNC(alt7),
	FUNC(alt8),
	FUNC(aon_cpu_standbyb),
	FUNC(aon_fp_4sec_resetb),
	FUNC(aon_gpclk),
	FUNC(aon_pwm),
	FUNC(arm_jtag),
	FUNC(aud_fs_clk0),
	FUNC(avs_pmu_bsc),
	FUNC(bsc_m0),
	FUNC(bsc_m1),
	FUNC(bsc_m2),
	FUNC(bsc_m3),
	FUNC(clk_observe),
	FUNC(ctl_hdmi_5v),
	FUNC(enet0),
	FUNC(enet0_mii),
	FUNC(enet0_rgmii),
	FUNC(ext_sc_clk),
	FUNC(fl0),
	FUNC(fl1),
	FUNC(gpclk0),
	FUNC(gpclk1),
	FUNC(gpclk2),
	FUNC(hdmi_tx0_auto_i2c),
	FUNC(hdmi_tx0_bsc),
	FUNC(hdmi_tx1_auto_i2c),
	FUNC(hdmi_tx1_bsc),
	FUNC(i2s_in),
	FUNC(i2s_out),
	FUNC(ir_in),
	FUNC(mtsif),
	FUNC(mtsif_alt),
	FUNC(mtsif_alt1),
	FUNC(pdm),
	FUNC(pkt),
	FUNC(pm_led_out),
	FUNC(sc0),
	FUNC(sd0),
	FUNC(sd2),
	FUNC(sd_card_a),
	FUNC(sd_card_b),
	FUNC(sd_card_c),
	FUNC(sd_card_d),
	FUNC(sd_card_e),
	FUNC(sd_card_f),
	FUNC(sd_card_g),
	FUNC(spdif_out),
	FUNC(spi_m),
	FUNC(spi_s),
	FUNC(sr_edm_sense),
	FUNC(te0),
	FUNC(te1),
	FUNC(tsio),
	FUNC(uart0),
	FUNC(uart1),
	FUNC(uart2),
	FUNC(usb_pwr),
	FUNC(usb_vbus),
	FUNC(uui),
	FUNC(vc_i2c0),
	FUNC(vc_i2c3),
	FUNC(vc_i2c4),
	FUNC(vc_i2c5),
	FUNC(vc_i2csl),
	FUNC(vc_pcm),
	FUNC(vc_pwm0),
	FUNC(vc_pwm1),
	FUNC(vc_spi0),
	FUNC(vc_spi3),
	FUNC(vc_spi4),
	FUNC(vc_spi5),
	FUNC(vc_uart0),
	FUNC(vc_uart2),
	FUNC(vc_uart3),
	FUNC(vc_uart4),
};

#define GPIO_PIN(a) PINCTRL_PIN(a, "gpio" #a)

static const struct pinctrl_pin bcm2712_c0_gpio_pins[] = {
	GPIO_PIN(0),
	GPIO_PIN(1),
	GPIO_PIN(2),
	GPIO_PIN(3),
	GPIO_PIN(4),
	GPIO_PIN(5),
	GPIO_PIN(6),
	GPIO_PIN(7),
	GPIO_PIN(8),
	GPIO_PIN(9),
	GPIO_PIN(10),
	GPIO_PIN(11),
	GPIO_PIN(12),
	GPIO_PIN(13),
	GPIO_PIN(14),
	GPIO_PIN(15),
	GPIO_PIN(16),
	GPIO_PIN(17),
	GPIO_PIN(18),
	GPIO_PIN(19),
	GPIO_PIN(20),
	GPIO_PIN(21),
	GPIO_PIN(22),
	GPIO_PIN(23),
	GPIO_PIN(24),
	GPIO_PIN(25),
	GPIO_PIN(26),
	GPIO_PIN(27),
	GPIO_PIN(28),
	GPIO_PIN(29),
	GPIO_PIN(30),
	GPIO_PIN(31),
	GPIO_PIN(32),
	GPIO_PIN(33),
	GPIO_PIN(34),
	GPIO_PIN(35),
	GPIO_PIN(36),
	GPIO_PIN(37),
	GPIO_PIN(38),
	GPIO_PIN(39),
	GPIO_PIN(40),
	GPIO_PIN(41),
	GPIO_PIN(42),
	GPIO_PIN(43),
	GPIO_PIN(44),
	GPIO_PIN(45),
	GPIO_PIN(46),
	GPIO_PIN(47),
	GPIO_PIN(48),
	GPIO_PIN(49),
	GPIO_PIN(50),
	GPIO_PIN(51),
	GPIO_PIN(52),
	GPIO_PIN(53),
	PINCTRL_PIN(54, "emmc_cmd"),
	PINCTRL_PIN(55, "emmc_ds"),
	PINCTRL_PIN(56, "emmc_clk"),
	PINCTRL_PIN(57, "emmc_dat0"),
	PINCTRL_PIN(58, "emmc_dat1"),
	PINCTRL_PIN(59, "emmc_dat2"),
	PINCTRL_PIN(60, "emmc_dat3"),
	PINCTRL_PIN(61, "emmc_dat4"),
	PINCTRL_PIN(62, "emmc_dat5"),
	PINCTRL_PIN(63, "emmc_dat6"),
	PINCTRL_PIN(64, "emmc_dat7"),
};

static const uint32_t bcm2712_c0_gpio_funcs_groups[] = {
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
};

struct bcm2712_pin_funcs {
	uint8_t funcs[BCM2712_FSEL_COUNT - 1];
};

#define PIN(i, f1, f2, f3, f4, f5, f6, f7, f8) \
	[i] = { \
		.funcs = { \
			func_##f1, \
			func_##f2, \
			func_##f3, \
			func_##f4, \
			func_##f5, \
			func_##f6, \
			func_##f7, \
			func_##f8, \
		}, \
	}

static const struct bcm2712_pin_funcs bcm2712_c0_gpio_pin_funcs[] = {
	PIN(0, bsc_m3, vc_i2c0, gpclk0, enet0, vc_pwm1, vc_spi0, ir_in, _),
	PIN(1, bsc_m3, vc_i2c0, gpclk1, enet0, vc_pwm1, sr_edm_sense, vc_spi0, vc_uart3),
	PIN(2, pdm, i2s_in, gpclk2, vc_spi4, pkt, vc_spi0, vc_uart3, _),
	PIN(3, pdm, i2s_in, vc_spi4, pkt, vc_spi0, vc_uart3, _, _),
	PIN(4, pdm, i2s_in, arm_jtag, vc_spi4, pkt, vc_spi0, vc_uart3, _),
	PIN(5, pdm, vc_i2c3, arm_jtag, sd_card_e, vc_spi4, pkt, vc_pcm, vc_i2c5),
	PIN(6, pdm, vc_i2c3, arm_jtag, sd_card_e, vc_spi4, pkt, vc_pcm, vc_i2c5),
	PIN(7, i2s_out, spdif_out, arm_jtag, sd_card_e, vc_i2c3, enet0_rgmii, vc_pcm, vc_spi4),
	PIN(8, i2s_out, aud_fs_clk0, arm_jtag, sd_card_e, vc_i2c3, enet0_mii, vc_pcm, vc_spi4),
	PIN(9, i2s_out, aud_fs_clk0, arm_jtag, sd_card_e, enet0_mii, sd_card_c, vc_spi4, _),
	PIN(10, bsc_m3, mtsif_alt1, i2s_in, i2s_out, vc_spi5, enet0_mii, sd_card_c, vc_spi4),
	PIN(11, bsc_m3, mtsif_alt1, i2s_in, i2s_out, vc_spi5, enet0_mii, sd_card_c, vc_spi4),
	PIN(12, spi_s, mtsif_alt1, i2s_in, i2s_out, vc_spi5, vc_i2csl, sd0, sd_card_d),
	PIN(13, spi_s, mtsif_alt1, i2s_out, usb_vbus, vc_spi5, vc_i2csl, sd0, sd_card_d),
	PIN(14, spi_s, vc_i2csl, enet0_rgmii, arm_jtag, vc_spi5, vc_pwm0, vc_i2c4, sd_card_d),
	PIN(15, spi_s, vc_i2csl, vc_spi3, arm_jtag, vc_pwm0, vc_i2c4, gpclk0, _),
	PIN(16, sd_card_b, i2s_out, vc_spi3, i2s_in, sd0, enet0_rgmii, gpclk1, _),
	PIN(17, sd_card_b, i2s_out, vc_spi3, i2s_in, ext_sc_clk, sd0, enet0_rgmii, gpclk2),
	PIN(18, sd_card_b, i2s_out, vc_spi3, i2s_in, sd0, enet0_rgmii, vc_pwm1, _),
	PIN(19, sd_card_b, usb_pwr, vc_spi3, pkt, spdif_out, sd0, ir_in, vc_pwm1),
	PIN(20, sd_card_b, uui, vc_uart0, arm_jtag, uart2, usb_pwr, vc_pcm, vc_uart4),
	PIN(21, usb_pwr, uui, vc_uart0, arm_jtag, uart2, sd_card_b, vc_pcm, vc_uart4),
	PIN(22, usb_pwr, enet0, vc_uart0, mtsif, uart2, usb_vbus, vc_pcm, vc_i2c5),
	PIN(23, usb_vbus, enet0, vc_uart0, mtsif, uart2, i2s_out, vc_pcm, vc_i2c5),
	PIN(24, mtsif, pkt, uart0, enet0_rgmii, enet0_rgmii, vc_i2c4, vc_uart3, _),
	PIN(25, mtsif, pkt, sc0, uart0, enet0_rgmii, enet0_rgmii, vc_i2c4, vc_uart3),
	PIN(26, mtsif, pkt, sc0, uart0, enet0_rgmii, vc_uart4, vc_spi5, _),
	PIN(27, mtsif, pkt, sc0, uart0, enet0_rgmii, vc_uart4, vc_spi5, _),
	PIN(28, mtsif, pkt, sc0, enet0_rgmii, vc_uart4, vc_spi5, _, _),
	PIN(29, mtsif, pkt, sc0, enet0_rgmii, vc_uart4, vc_spi5, _, _),
	PIN(30, mtsif, pkt, sc0, sd2, enet0_rgmii, gpclk0, vc_pwm0, _),
	PIN(31, mtsif, pkt, sc0, sd2, enet0_rgmii, vc_spi3, vc_pwm0, _),
	PIN(32, mtsif, pkt, sc0, sd2, enet0_rgmii, vc_spi3, vc_uart3, _),
	PIN(33, mtsif, pkt, sd2, enet0_rgmii, vc_spi3, vc_uart3, _, _),
	PIN(34, mtsif, pkt, ext_sc_clk, sd2, enet0_rgmii, vc_spi3, vc_i2c5, _),
	PIN(35, mtsif, pkt, sd2, enet0_rgmii, vc_spi3, vc_i2c5, _, _),
	PIN(36, sd0, mtsif, sc0, i2s_in, vc_uart3, vc_uart2, _, _),
	PIN(37, sd0, mtsif, sc0, vc_spi0, i2s_in, vc_uart3, vc_uart2, _),
	PIN(38, sd0, mtsif_alt, sc0, vc_spi0, i2s_in, vc_uart3, vc_uart2, _),
	PIN(39, sd0, mtsif_alt, sc0, vc_spi0, vc_uart3, vc_uart2, _, _),
	PIN(40, sd0, mtsif_alt, sc0, vc_spi0, bsc_m3, _, _, _),
	PIN(41, sd0, mtsif_alt, sc0, vc_spi0, bsc_m3, _, _, _),
	PIN(42, vc_spi0, mtsif_alt, vc_i2c0, sd_card_a, mtsif_alt1, arm_jtag, pdm, spi_m),
	PIN(43, vc_spi0, mtsif_alt, vc_i2c0, sd_card_a, mtsif_alt1, arm_jtag, pdm, spi_m),
	PIN(44, vc_spi0, mtsif_alt, enet0, sd_card_a, mtsif_alt1, arm_jtag, pdm, spi_m),
	PIN(45, vc_spi0, mtsif_alt, enet0, sd_card_a, mtsif_alt1, arm_jtag, pdm, spi_m),
	PIN(46, vc_spi0, mtsif_alt, sd_card_a, mtsif_alt1, arm_jtag, pdm, spi_m, _),
	PIN(47, enet0, mtsif_alt, i2s_out, mtsif_alt1, arm_jtag, _, _, _),
	PIN(48, sc0, usb_pwr, spdif_out, mtsif, _, _, _, _),
	PIN(49, sc0, usb_pwr, aud_fs_clk0, mtsif, _, _, _, _),
	PIN(50, sc0, usb_vbus, sc0, _, _, _, _, _),
	PIN(51, sc0, enet0, sc0, sr_edm_sense, _, _, _, _),
	PIN(52, sc0, enet0, vc_pwm1, _, _, _, _, _),
	PIN(53, sc0, enet0_rgmii, ext_sc_clk, _, _, _, _, _),
};

static inline uint32_t bcm2712_pfc_reg_rd(unsigned int reg)
{
	return mmio_read_32(RPI_PINCTRL_BASE + reg);
}

static inline void bcm2712_pfc_reg_wr(unsigned int reg, uint32_t val)
{
	mmio_write_32(RPI_PINCTRL_BASE + reg, val);
}

static enum bcm2712_pfc_funcs bcm2712_pfc_fsel_get(unsigned int pin)
{
	uint16_t bit = bcm2712_c0_gpio_pin_regs[pin].mux_bit;
	enum bcm2712_pfc_funcs func;
	uint32_t val;
	uint32_t fsel;

	if (!bit) {
		return func_gpio;
	}

	bit &= ~MUX_BIT_VALID;

	val = bcm2712_pfc_reg_rd(BIT_TO_REG(bit));
	fsel = (val >> BIT_TO_SHIFT(bit)) & BCM2712_FSEL_MASK;
	func = bcm2712_c0_gpio_pin_funcs[pin].funcs[fsel];
	if (func >= func_count) {
		func = (enum bcm2712_pfc_funcs)fsel;
	}

	VERBOSE("bcm2712_pinctrl_fsel_get %04x: %08x (%u => %s)\n",
		BIT_TO_REG(bit), val, pin, bcm2712_pfc_func_names[func]);

	return func;
}

static void bcm2712_pinctrl_fsel_set(unsigned int pin,
				     enum bcm2712_pfc_funcs func)
{
	uint16_t bit = bcm2712_c0_gpio_pin_regs[pin].mux_bit;
	const uint8_t *pin_funcs;
	uint32_t val;
	uint32_t fsel;
	uint32_t cur_fsel;
	int i;

	if (!bit || func >= func_count) {
		return;
	}

	bit &= ~MUX_BIT_VALID;
	fsel = BCM2712_FSEL_COUNT;

	if (func >= BCM2712_FSEL_COUNT) {
		/* Convert to an fsel number */
		pin_funcs = bcm2712_c0_gpio_pin_funcs[pin].funcs;
		for (i = 1; i < BCM2712_FSEL_COUNT; i++) {
			if (pin_funcs[i - 1] == func) {
				fsel = i;
				break;
			}
		}
	} else {
		fsel = func;
	}
	if (fsel >= BCM2712_FSEL_COUNT) {
		return;
	}

	val = bcm2712_pfc_reg_rd(BIT_TO_REG(bit));
	cur_fsel = (val >> BIT_TO_SHIFT(bit)) & BCM2712_FSEL_MASK;

	VERBOSE("read %04x: %08x (%u => %s)\n", BIT_TO_REG(bit), val, pin,
		bcm2712_pfc_func_names[cur_fsel]);

	if (cur_fsel != fsel) {
		val &= ~(BCM2712_FSEL_MASK << BIT_TO_SHIFT(bit));
		val |= fsel << BIT_TO_SHIFT(bit);

		VERBOSE("write %04x: %08x (%u <= %s)\n", BIT_TO_REG(bit), val,
			pin, bcm2712_pfc_func_names[fsel]);
		bcm2712_pfc_reg_wr(BIT_TO_REG(bit), val);
	}
}

static unsigned int bcm2712_pfc_pull_config_get(unsigned int pin)
{
	uint32_t bit = bcm2712_c0_gpio_pin_regs[pin].pad_bit;
	uint32_t val;

	if (bit == REG_BIT_INVALID) {
		return BCM2712_PULL_NONE;
	}

	val = bcm2712_pfc_reg_rd(BIT_TO_REG(bit));
	return (val >> BIT_TO_SHIFT(bit)) & BCM2712_PULL_MASK;
}

static void bcm2712_pfc_pull_config_set(unsigned int pin, unsigned int arg)
{
	uint32_t bit = bcm2712_c0_gpio_pin_regs[pin].pad_bit;
	uint32_t val;

	if (bit == REG_BIT_INVALID) {
		ERROR("can't set pulls for pin %u\n", pin);
		return;
	}

	spin_lock(&bcm2712_pfc_lock);
	val = bcm2712_pfc_reg_rd(BIT_TO_REG(bit));
	val &= ~(BCM2712_PULL_MASK << BIT_TO_SHIFT(bit));
	val |= (arg << BIT_TO_SHIFT(bit));
	bcm2712_pfc_reg_wr(BIT_TO_REG(bit), val);
	spin_unlock(&bcm2712_pfc_lock);
}

uint32_t pinctrl_get_groups_count(void)
{
	VERBOSE("%s: %lu\n", __func__,
		ARRAY_SIZE(bcm2712_c0_gpio_pins));
	return ARRAY_SIZE(bcm2712_c0_gpio_pins);
}

const char *pinctrl_get_group_name(uint32_t group_id)
{
	VERBOSE("%s: %u\n", __func__, group_id);

	return bcm2712_c0_gpio_pins[group_id].name;
}

int pinctrl_get_group_pins(uint32_t group_id, const unsigned int **pins,
			   uint32_t *num_pins)
{
	VERBOSE("%s: %u\n", __func__, group_id);
	*num_pins = 1;
	*pins = &bcm2712_c0_gpio_pins[group_id].pin;

	return 0;
}

uint32_t pinctrl_get_pins_count(void)
{
	VERBOSE("%s: %lu\n", __func__, ARRAY_SIZE(bcm2712_c0_gpio_pins));
	return ARRAY_SIZE(bcm2712_c0_gpio_pins);
}

const char *pinctrl_get_pin_name(uint32_t pin_id)
{
	assert(pin_id < ARRAY_SIZE(bcm2712_c0_gpio_pins));
	return bcm2712_c0_gpio_pins[pin_id].name;
}

int pinctrl_get_pins(const struct pinctrl_pin **pins, uint32_t *num_pins)
{
	*num_pins = ARRAY_SIZE(bcm2712_c0_gpio_pins);
	*pins = bcm2712_c0_gpio_pins;

	return 0;
}

uint32_t pinctrl_get_functions_count(void)
{
	VERBOSE("%s: %lu\n", __func__, ARRAY_SIZE(bcm2712_pfc_func_names));

	return ARRAY_SIZE(bcm2712_pfc_func_names);
}

const char *pinctrl_get_function_name(uint32_t func_id)
{
	VERBOSE("%s: %u\n", __func__, func_id);

	return bcm2712_pfc_func_names[func_id];
}

int pinctrl_get_function_groups(uint32_t func_id, const uint32_t **groups,
				uint32_t *num_groups)
{
	VERBOSE("%s: %u\n", __func__, func_id);

	*num_groups = ARRAY_SIZE(bcm2712_c0_gpio_pins);
	*groups = bcm2712_c0_gpio_funcs_groups;

	return 0;
}

int pinctrl_get_mux_group(uint32_t group_id, uint32_t *func_id)
{
	VERBOSE("%s: %u\n", __func__, group_id);

	*func_id = bcm2712_pfc_fsel_get(group_id);

	return 0;
}

int pinctrl_set_mux_group(uint32_t group_id, uint32_t func_id)
{
	VERBOSE("%s: %u -> %u\n", __func__, group_id, func_id);

	spin_lock(&bcm2712_pfc_lock);
	bcm2712_pinctrl_fsel_set(group_id, (enum bcm2712_pfc_funcs)func_id);
	spin_unlock(&bcm2712_pfc_lock);
	return 0;
}

int pinctrl_get_mux_pin(uint32_t pin_id, uint32_t *func_id)
{
	VERBOSE("%s: %u\n", __func__, pin_id);

	*func_id = bcm2712_pfc_fsel_get(pin_id);

	return 0;
}

int pinctrl_set_mux_pin(uint32_t pin_id, uint32_t func_id)
{
	VERBOSE("%s: %u -> %u\n", __func__, pin_id, func_id);

	spin_lock(&bcm2712_pfc_lock);
	bcm2712_pinctrl_fsel_set(pin_id, (enum bcm2712_pfc_funcs)func_id);
	spin_unlock(&bcm2712_pfc_lock);
	return 0;
}

static int bcm2712_pfc_pinconf_get(uint32_t pin, uint32_t *config)
{
	enum pin_config_param param = pinconf_to_config_param(*config);
	uint32_t arg;

	VERBOSE("%s: %u\n", __func__, pin);

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		arg = (bcm2712_pfc_pull_config_get(pin) == BCM2712_PULL_NONE);
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		arg = (bcm2712_pfc_pull_config_get(pin) == BCM2712_PULL_DOWN);
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		arg = (bcm2712_pfc_pull_config_get(pin) == BCM2712_PULL_UP);
		break;
	default:
		return -ENOTSUP;
	}

	*config = pinconf_to_config_packed(param, arg);

	return 0;
}

static int bcm2712_pfc_pinconf_set(uint32_t pin, uint32_t config)
{
	uint32_t param, arg;

	param = pinconf_to_config_param(config);
	arg = pinconf_to_config_argument(config);

	VERBOSE("%s param: %u arg %u\n", __func__, param, arg);

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		bcm2712_pfc_pull_config_set(pin, BCM2712_PULL_NONE);
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		bcm2712_pfc_pull_config_set(pin, BCM2712_PULL_DOWN);
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		bcm2712_pfc_pull_config_set(pin, BCM2712_PULL_UP);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int pinctrl_config_group_get(uint32_t group_id, uint32_t *config)
{
	return bcm2712_pfc_pinconf_get(group_id, config);
}

int pinctrl_config_group_set(uint32_t group_id, uint32_t config)
{
	return bcm2712_pfc_pinconf_set(group_id, config);
}

int pinctrl_config_get(uint32_t pin_id, uint32_t *config)
{
	return bcm2712_pfc_pinconf_get(pin_id, config);
}

int pinctrl_config_set(uint32_t pin_id, uint32_t config)
{
	return bcm2712_pfc_pinconf_set(pin_id, config);
}

int pinctrl_request_group(uint32_t group_id)
{
	return 0;
}

int pinctrl_free_group(uint32_t group_id)
{
	return 0;
}

int pinctrl_request_pin(uint32_t pin_id)
{
	return 0;
}

int pinctrl_free_pin(uint32_t pin_id)
{
	return 0;
}
