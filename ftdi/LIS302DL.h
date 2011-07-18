#define REG_WhoAmI		0x0F
#define REG_CtrlReg1		0x20
#define REG_CtrlReg2		0x21
#define REG_CtrlReg3		0x22
#define REG_HPFilterReset	0x23
#define REG_StatusReg		0x27
#define REG_OutX		0x29
#define REG_OutY		0x2B
#define REG_OutZ		0x2D
#define REG_FfWuCfg1		0x30
#define REG_FfWuSrc1		0x31
#define REG_FfWuThs1		0x32
#define REG_FfWuDuration1	0x33
#define REG_FfWuCfg2		0x34
#define REG_FfWuSrc2		0x35
#define REG_FfWuThs2		0x36
#define REG_FfWuDuration2	0x37

#define BV_DR	7
#define BV_PD	6
#define BV_FS	5
#define BV_STP	4
#define BV_STM	3
#define BV_Zen	2
#define BV_Yen	1
#define BV_Xen	0

typedef	unsigned int	u08;

struct strCtrlReg1 {
        u08     Xen:1;		// LSB
        u08     Yen:1;
        u08     Zen:1;
        u08     STM:1;
        u08     STP:1;
        u08     FS:1;
        u08     PD:1;
        u08     DR:1;		// MSB
};

#define BV_SIM		7
#define BV_BOOT		6
//--			5
#define BV_FDS		4
#define BV_HPFF_WU2	3
#define BV_HPFF_WU1	2
#define BV_HPcoeff2	1
#define BV_HPcoeff1	0

struct strCtrlReg2 {
        u08     HPCoeff:2;	// LSB
        u08     HPFF_WU1:1;
        u08     HPFF_WU2:1;
        u08     FDS:1;
        u08     :1;
        u08     BOOT:1;
        u08     SIM:1;		// MSB
};

#define BV_IHL		7
#define BV_PP_OD	6
#define BV_I2CFG2	5
#define BV_I2CFG1	4
#define BV_I2CFG0	3
#define BV_I1CFG2	2
#define BV_I1CFG1	1
#define BV_I1CFG0	0

#define ICFG_GND	0x00
#define ICFG_WU1	0x01
#define ICFG_WU2	0x02
#define ICFG_WU12	0x03
#define ICFG_DR		0x04
#define ICFG_INVAL	0x07
struct strCtrlReg3 {
        u08     I1CFG:3;	// LSB
        u08     I2CFG:3;
        u08     PP_OD:1;
        u08     IHL:1;		// MSB
};

#define BV_ZYXOR	7
#define BV_ZOR		6
#define BV_YOR		5
#define BV_XOR		4
#define BV_ZYXDA	3
#define BV_ZDA		2
#define BV_YDA		1
#define BV_XDA		0

struct strStatusReg {
        u08     XDA:1;		// LSB
        u08     YDA:1;
        u08     ZDA:1;
        u08     ZXYDA:1;
        u08     XOR:1;
        u08     YOR:1;
        u08     ZOR:1;
        u08     ZYXOR:1;	// MSB
};

