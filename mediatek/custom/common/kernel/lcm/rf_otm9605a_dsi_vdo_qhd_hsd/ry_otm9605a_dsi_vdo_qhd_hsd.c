#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif

#include "cust_gpio_usage.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (540)
#define FRAME_HEIGHT (960)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_ID_OTM9605A									0x9605

//#define LCM_DEBUG
#if defined(BUILD_LK)
	#if defined(BUILD_LK)
	#define LCM_LOG(fmt, args...)    printf(fmt, ##args)
	#else
	#define LCM_LOG(fmt, args...)    printk(fmt, ##args)	
	#endif
#else
#define LCM_LOG(fmt, args...)	 printk(fmt, ##args)	
#endif

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define SET_GPIO_OUT(n, v)  (lcm_util.set_gpio_out((n), (v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	//OTM9605A_LG5.0_MIPI初始化如下：
	 {0x00,1,{0x00}},
	 {0xFF,3,{0x96,0x05,0x01}},

	 {0x00,1,{0x80}},
	 {0xFF,2,{0x96,0x05}},

	 {0x00,1,{0x92}}, // mipi 2 lane
	 {0xFF,2,{0x10,0x02}},

	 {0x00,1,{0xB4}}, 
	 {0xC0,1,{0x10}},//inversion  50

	 {0x00,1,{0x80}},
	 {0xC1,2,{0x36,0x66}}, //70Hz  77

	 {0x00,1,{0x89}},
	 {0xC0,1,{0x01}},// TCON OSC turbo mode

	 {0x00,1,{0xA0}},
	 {0xC1,1,{0x00}},//02   ESD 

	 // DC voltage for LGD 4.5"
	 {0x00,1,{0x80}},
	 {0xC5,4,{0x08,0x00,0xA0,0x11}},

	 {0x00,1,{0x90}},
	 {0xC5,3,{0xD6,0x57,0x01}}, //VGH=14V, VGL=-11V

	 {0x00,1,{0xB0}},
	 {0xC5,2,{0x05,0xac}}, //05 28

	 {0x00,1,{0x00}}, //GVDD=5.2V/NGVDD=-5.2V
	 {0xD8,2,{0xa7,0xa7}},//85  85

	 {0x00,1,{0x00}}, //Vcom setting
	 {0xD9,1,{0x5c}},//47  64  5a  56//*****53

	 {0x00,1,{0x80}},//************ 
	 {0xC4,1,{0x9C}},

	 {0x00,1,{0x87}},//**************** 
	 {0xC4,1,{0x40}},

	 //Inrush Current Test
	 {0x00,1,{0xA6}},
	 {0xC1,1,{0x01}},

	 {0x00,1,{0xA2}}, // pl_width, pch_dri_pch_nop
	 {0xC0,3,{0x0C,0x05,0x02}},

	 //GOA mapping
	 {0x00,1,{0x80}}, //GOA mapping
	 {0xCB,10,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	 {0x00,1,{0x90}}, //GOA mapping
	 {0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},    

	 {0x00,1,{0xA0}},
	 {0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},    

	 {0x00,1,{0xB0}}, 
	 {0xCB,10,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	 {0x00,1,{0xC0}},
	 {0xCB,15,{0x04,0x04,0x04,0x04,0x08,0x04,0x08,0x04,0x08,0x04,0x08,0x04,0x04,0x04,0x08}},   

	 {0x00,1,{0xD0}}, 
	 {0xCB,15,{0x08,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x04,0x08,0x04,0x08,0x04,0x08,0x04}},    

	 {0x00,1,{0xE0}}, 
	 {0xCB,10,{0x08,0x04,0x04,0x04,0x08,0x08,0x00,0x00,0x00,0x00}},

	 {0x00,1,{0xF0}}, 
	 {0xCB,10,{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},

	 {0x00,1,{0x80}},
	 {0xCC,10,{0x26,0x25,0x21,0x22,0x00,0x0F,0x00,0x0D,0x00,0x0B}},

	 {0x00,1,{0x90}},
	 {0xCC,15,{0x00,0x09,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x26,0x25,0x21,0x22,0x00}},    

	 {0x00,1,{0xA0}},
	 {0xCC,15,{0x10,0x00,0x0E,0x00,0x0C,0x00,0x0A,0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00}},

	 {0x00,1,{0x80}},//GOA VST Setting
	 {0xCE,12,{0x8B,0x03,0x06,0x8A,0x03,0x06,0x89,0x03,0x06,0x88,0x03,0x06}},           

	 {0x00,1,{0x90}}, //GOA VEND and Group Setting
	 {0xCE,14,{0xF0,0x00,0x00,0xF0,0x00,0x00,0xF0,0x00,0x00,0xF0,0x00,0x00,0x00,0x00}},       

	 {0x00,1,{0xA0}}, //GOA CLK1 and GOA CLK2 Setting
	 {0xCE,14,{0x38,0x03,0x03,0xC0,0x00,0x06,0x00,0x38,0x02,0x03,0xC1,0x00,0x06,0x00}},      

	 {0x00,1,{0xB0}}, //GOA CLK3 and GOA CLK4 Setting/Address Shift
	 {0xCE,14,{0x38,0x01,0x03,0xC2,0x00,0x06,0x00,0x38,0x00,0x03,0xC3,0x00,0x06,0x00}},       

	 {0x00,1,{0xC0}}, //GOA CLKB1 and GOA CLKB2 Setting
	 {0xCE,14,{0x38,0x07,0x03,0xBC,0x00,0x06,0x00,0x38,0x06,0x03,0xBD,0x00,0x06,0x00}},      

	 {0x00,1,{0xD0}}, //GOA CLKB3 and GOA CLKB4 Setting
	 {0xCE,14,{0x38,0x05,0x03,0xBE,0x00,0x06,0x00,0x38,0x04,0x03,0xBF,0x00,0x06,0x00}},       

	 {0x00,1,{0x80}}, //GOA CLKC1 and GOA CLKC2 Setting
	 {0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},      

	 {0x00,1,{0x90}}, //GOA CLKC3 and GOA CLKC4 Setting
	 {0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},       

	 {0x00,1,{0xA0}}, //GOA CLKD1 and GOA CLKD2 Setting
	 {0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},       

	 {0x00,1,{0xB0}}, //GOA CLKD3 and GOA CLKD4 Setting
	 {0xCF,14,{0xF0,0x00,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,0x00,0x10,0x00,0x00,0x00}},       

	 {0x00,1,{0xC0}}, //GOA ECLK Setting and GOA Other Options1 and GOA Signal Toggle Option Setting
	 {0xCF,10,{0x02,0x02,0x20,0x20,0x00,0x00,0x01,0x02,0x00,0x02}},

	 {0x00,1,{0x00}},
	 {0xE1,16,{0x00,0x0B,0x11,0x10,0x07,0x0E,0x09,0x07,0x05,0x06,0x12,0x08,0x0f,0x0d,0x08,0x03}},//G2.2 POS    

	 {0x00,1,{0x00}},
	 {0xE2,16,{0x00,0x0B,0x11,0x10,0x07,0x0E,0x09,0x07,0x05,0x06,0x12,0x08,0x0f,0x0d,0x03,0x03}},//G2.2 POS     

	 {0x00,1,{0xb1}}, 
	 {0xC5,1,{0x28}},//VDD18 

	 {0x00,1,{0xB2}},
	 {0xF5,4,{0x15,0x00,0x15,0x00}},//VRGH disable 

	 {0x00,1,{0xC0}},
	 {0xC5,1,{0x00}},//thermo disable 

	 {0x00,1,{0x80}},//Source output levels during porch and non-display area 
	 {0xC4,1,{0x9C}}, 

	 {0x00,1,{0x00}}, 
	 {0xff,3,{0xff,0xff,0xff}},

	 {0x11,1,{0x00}},  
	 {REGFLAG_DELAY,20,{}},

	 {0x29,1,{0x00}},//Display ON 
	 {REGFLAG_DELAY,20,{}}, 

{REGFLAG_END_OF_TABLE, 0x00, {}}
};



static struct LCM_setting_table lcm_compare_id_setting[] = {
    // zht 
	{0x00,1,{0x00}},
	{0xFF,3,{0x96,0x05,0x01}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

//static int vcom = 0x50;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned int cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
	  /* 
	   case 0xD9:
	        table[i].para_list[0]=0x00;
		table[i].para_list[1]=vcom;
		dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		vcom+=1;
		break;	
	   */
            case REGFLAG_DELAY:
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE:
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
    }	
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));
	
	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;


	params->dsi.mode   = SYNC_EVENT_VDO_MODE;
	
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding 	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;
		
	params->dsi.packet_size=256;
	params->dsi.intermediat_buffer_num = 2;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
		
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
	params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 16;
	params->dsi.vertical_frontporch					= 15;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 10;
	params->dsi.horizontal_backporch				= 64;
	params->dsi.horizontal_frontporch				= 64;
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;
	
	// Bit rate calculation
  	//lizc improve clock for c919 haixu 20141009
	params->dsi.pll_div1=1; 	// div1=0,1,2,3;div1_real=1,2,4,4
	params->dsi.pll_div2=1;		// div2=0,1,2,3;div2_real=1,2,4,4
	params->dsi.fbk_sel=1;		 // fbk_sel=0,1,2,3;fbk_sel_real=1,2,4,4
	params->dsi.fbk_div =32;		// fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
}


static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(2); 
	SET_RESET_PIN(0);
	MDELAY(20); //10
	
	SET_RESET_PIN(1);
	MDELAY(120); 

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
	
	data_array[0]=0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);

	SET_RESET_PIN(1);
	MDELAY(2);
	SET_RESET_PIN(0);
    MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_esd_recover()
{
    lcm_init();
    return TRUE;
}

static void lcm_setpwm(unsigned int divider)
{
	// TBD
}

static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[5];
	char id_high=0;
	char id_low=0;
	int id=0;

	SET_RESET_PIN(1);
	MDELAY(2);
	SET_RESET_PIN(0);
	MDELAY(30);
	SET_RESET_PIN(1);
	MDELAY(200);

    push_table(lcm_compare_id_setting, sizeof(lcm_compare_id_setting) / sizeof(struct LCM_setting_table), 1);  //zht 


	array[0] = 0x00053700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xa1, buffer, 5);

	id_high = buffer[2];
	id_low = buffer[3];
	id = (id_high<<8) | id_low;

	#if defined(BUILD_LK)
		printf("OTM9605A uboot %s \n", __func__);
		printf("%s id = 0x%08x \n", __func__, id);
	#else
		printk("OTM9605A kernel %s \n", __func__);
		printk("%s id = 0x%08x \n", __func__, id);
	#endif

	return (LCM_ID_OTM9605A == id)?1:0;
}

LCM_DRIVER otm9605a_qhd_dsi_vdo_lg50_drv = 
{
    .name			= "otm9605a_qhd_dsi_vdo_lg50",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,	
};
