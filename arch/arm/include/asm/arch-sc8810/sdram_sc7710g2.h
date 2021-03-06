#ifndef _EMC_DRV_H_
#define _EMC_DRV_H_
#include "sci_types.h"


/******************************************************************************
                           Macro define
******************************************************************************/
#define INVALIDE_VAL        0xFFFFFFFF
#define STATE_SDRAM_TYPE    0UL
#define STATE_BIT_WIDTH     1UL
#define STATE_COLUM         2UL
#define STATE_ROW           3UL
#define STATE_REINIT        4UL
#define STATE_END           5UL

#define DCFG0_AUTOREF_EN             BIT_14
#define STS3_SMEM_IDLE               BIT_17
#define STS3_DMEM_IDLE               BIT_18
#define STS3_EMC_IDLE                BIT_31
    
#define DCFG2_CNT_DONE               BIT_14
#define DCFG2_REF_CNT_RST            BIT_15
#define DCFG2_AUTO_SLEEP_MODE        BIT_22
#define DCFG2_AUTO_SLEEP_EN          BIT_23
#define DCFG2_SAMPLE_RST             BIT_24
#define DCFG2_SAMPLE_AUTO_RST_EN     BIT_25
#define DCFG0_DLL_LOCK_BIT			 BIT_14
#define DCFG0_DLL_COMPENSATION_START BIT_11
#define DCFG0_DLL_COMPENSATION_EN 	 BIT_10
    
#define	BK_MODE_1			         0  // 1 bank
#define	BK_MODE_2			         1  // 2 bank
#define	BK_MODE_4			         2  // 4 bank
#define	BK_MODE_8			         3	// 8 bank
        
#define DBURST_REG_BL_1              0
#define DBURST_REG_BL_2              1
#define DBURST_REG_BL_4              2
#define DBURST_REG_BL_8              3
    
    //define mode register domain..
#define MODE_REG_BL_1                0
#define MODE_REG_BL_2                1
#define MODE_REG_BL_4                2
#define MODE_REG_BL_8                3
    
#define MODE_REG_BT_SEQ              0
#define MODE_REG_BT_INT              1
    
#define MODE_REG_CL_1                1
#define MODE_REG_CL_2                2
#define MODE_REG_CL_3                3
    
#define MODE_REG_OPMODE              0

#define ROW_MODE_MASK           0x3
#define COL_MODE_MASK           0x7
#define DATA_WIDTH_MASK         0x1
#define AUTO_PRECHARGE_MASK     0x3
#define CS_POSITION_MASK        0x3


#define EXT_MODE_DS_FULL                0
#define EXT_MODE_DS_HALF                1
#define EXT_MODE_DS_QUARTER             2
#define EXT_MODE_DS_OCTANT              3
#define EXT_MODE_DS_THREE_QUARTERS      4



typedef enum
{
    DDR_DRV_STR_FULL = 0,   // 1/1
    DDR_DRV_STR_HALF = 1,   // 1/2
    DDR_DRV_STR_QUAR = 2,   // 1/4
    DDR_DRV_STR_OCTA = 3,   // 1/8
    DDR_DRV_STR_TR_Q = 4    // 3/4
}DDR_DRIVER_STRENGTH_T;


#define EXT_MODE_FLAG                1                 
#define EXT_MODE_PASR_ALL            0                                                                          
                                                                         
#define SDRAM_EXT_MODE_INVALID       0xffffffff  

#define	SDRAM_EXT_MODE_REG           ((EXT_MODE_FLAG<<15) | (EXT_MODE_DS_FULL<<5) | EXT_MODE_PASR_ALL)  


#define WAIT_EMC_IDLE           do{    \
                                    while(0 == (REG32(EXT_MEM_STS3)&(STS3_SMEM_IDLE)));\
                                    while(0 == (REG32(EXT_MEM_STS3)&(STS3_DMEM_IDLE)));\
                                    while(0 == (REG32(EXT_MEM_STS3)&(STS3_EMC_IDLE))); \
                                }while(0)

#define WAIT_EMC_DLL_LOCK	        do{    \
                                        while(0 == (REG32(EXT_MEM_CFG0_DLL)&(DCFG0_DLL_LOCK_BIT)));\
                                    }while(0)


                                        
#define ROW_LINE_MIN            11
#define COLUMN_LINE_MIN         8

/******************************************************************************
                            Enum define
******************************************************************************/

typedef enum SDRAM_ROW_MODE_TAG
{
    SDRAM_MIN_ROW = 0,
    ROW_MODE_11 = SDRAM_MIN_ROW,	// 11 bit row
    ROW_MODE_12,		            // 12 bit row
    ROW_MODE_13,		            // 13 bit row
    ROW_MODE_14,		            // 14 bit row
    SDRAM_MAX_ROW = ROW_MODE_14
}SDRAM_ROW_MODE_E;

typedef enum SDRAM_COLUMN_MODE_TAG
{
    SDRAM_MIN_COLUMN = 0,
    COL_MODE_8 = SDRAM_MIN_COLUMN,       //8 bit column 
    COL_MODE_9,                         //9 bit column 
	COL_MODE_10,                        //10 bit column
	COL_MODE_11,                        //11 bit column
	COL_MODE_12,                        //12 bit column
    COL_MODE_11_6G,                     // support 6G bit, column 11-bit
    ROW_MODE_15_6G,                     // support 6G bit, row 15-bit
    SDRAM_MAX_COLUMN = ROW_MODE_15_6G
}SDRAM_COLUMN_MODE_E;

typedef enum 
{
    BURST_LEN_1_WORD = 0,
    BURST_LEN_2_WORD,
    BURST_LEN_4_WORD,
    BURST_LEN_8_WORD,
    BURST_LEN_16_WORD,
    BURST_LEN_MAX = BURST_LEN_16_WORD
}SDRAM_BURST_LEN_E;


typedef enum SDRAM_CAS_LATENCY_TAG
{
    CAS_LATENCY_1 = 1,
    CAS_LATENCY_2,
    CAS_LATENCY_3,
    CAS_LATENCY_MAX = CAS_LATENCY_3
}SDRAM_CAS_LATENCY_E;

typedef enum SDRAM_CHIP_FEATURE_CL_TAG
{
    SDRAM_FEATURE_CL_2 = (1<<0),
    SDRAM_FEATURE_CL_3 = (1<<1),
    SDRAM_FEATURE_CL_MAX = SDRAM_FEATURE_CL_3
}
SDRAM_CHIP_FEATURE_CL_E;

typedef enum SDRAM_CHIP_FEATURE_BL_TAG
{
    SDRAM_FEATURE_BL_1 = (1<<0),
    SDRAM_FEATURE_BL_2 = (1<<1),
    SDRAM_FEATURE_BL_4 = (1<<2),
    SDRAM_FEATURE_BL_8 = (1<<3),
    SDRAM_FEATURE_BL_16 = (1<<4),
    SDRAM_FEATURE_BL_MAX = SDRAM_FEATURE_BL_16
}
SDRAM_CHIP_FEATURE_BL_E;

typedef enum SDRAM_CAP_TYPE_TAG
{
    CAP_ZERO        = 0,
    CAP_64M_BIT     =0x00800000,
    CAP_128M_BIT    =0x01000000,
    CAP_256M_BIT    =0x02000000,
    CAP_512M_BIT    =0x04000000,
    CAP_1G_BIT      =0x08000000,
    CAP_2G_BIT      =0x10000000,
    CAP_4G_BIT      =0x20000000,
    CAP_6G_BIT      =0x30000000,
    CAP_MAX         = CAP_6G_BIT
}
SDRAM_CAP_TYPE_E;

typedef enum
{
    DATA_WIDTH_16 = 0,
    DATA_WIDTH_32
}DMEM_DATA_WIDTH_E;


typedef enum
{
	SDR_SDRAM = 0,
	DDR_SDRAM
}DMEM_TYPE_E;

typedef enum{
    EMC_CLK_26MHZ    = 26000000,
    EMC_CLK_67MHZ    = 67000000,
    EMC_CLK_133MHZ    = 133333333,
    EMC_CLK_200MHZ   = 200000000,
    EMC_CLK_266MHZ   = 266666666,
    EMC_CLK_333MHZ   = 333333333,
    EMC_CLK_370MHZ   = 370000000,
    EMC_CLK_400MHZ   = 400000000,
    EMC_CLK_MAX = EMC_CLK_400MHZ
}EMC_CLK_TYPE_E;

typedef enum{
    CHIP_CLK_26MHZ    = 26000000,
    CHIP_CLK_800MHZ   = 800000000,
    CHIP_CLK_850MHZ   = 850000000,
    CHIP_CLK_900MHZ   = 900000000,
    CHIP_CLK_1000MHZ   = 1000000000,
    CHIP_CLK_1100MHZ   = 1100000000,
    CHIP_CLK_1200MHZ   = 1200000000,
    CHIP_CLK_1300MHZ   = 1300000000,
    CHIP_CLK_1400MHZ   = 1400000000,
    CHIP_CLK_1500MHZ   = 1500000000,
    CHIP_CLK_MAX = CHIP_CLK_1500MHZ
}CHIP_CLK_TYPE_E;


typedef enum{
    MEM_1K_BYTE  = 1024,
    MEM_1M_BYTE  = 1024*1024,
    MEM_1G_BYTE  = 1024*1024*1024
}MEM_SIZE_E;


typedef enum MEM_RW_TYPE_TAG
{
    MEM_READ,
    MEM_WRITE
}
MEM_RW_TYPE_E;

typedef enum MEM_ACCESS_TYPE_TAG
{
    MEM_ACCESS_TYPE_BYTE    = 1,
    MEM_ACCESS_TYPE_HWORD   = 2,
    MEM_ACCESS_TYPE_WORD    = 4
}
MEM_ACCESS_TYPE_E;

typedef enum MEM_ENDIAN_TYPE_TAG
{
    MEM_BIG_ENDIAN,
    MEM_LITTLE_ENDIAN
}
MEM_ENDIAN_TYPE_E;

typedef enum EMC_ENDIAN_SWITCH_TAG
{
    EMC_ENDIAN_SWITCH_NONE = 3,
    EMC_ENDIAN_SWITCH_BYTE = 0,
    EMC_ENDIAN_SWITCH_HALF = 1,
    EMC_ENDIAN_SWITCH_WORD = 2,
}
EMC_ENDIAN_SWITCH_E;

typedef enum EMC_DVC_ENDIAN_TAG
{
    EMC_DVC_ENDIAN_DEFAULT = 0,
    EMC_DVC_ENDIAN_LITTLE = 0,
    EMC_DVC_ENDIAN_BIG = 1
}
EMC_DVC_ENAIDN_E;

typedef enum EMC_AUTO_GATE_TAG
{
    EMC_AUTO_GATE_DEFAULT = 0,
    EMC_AUTO_GATE_DIS = 0,
    EMC_AUTO_GATE_EN = 1
}
EMC_AUTO_GATE_E;

typedef enum EMC_AUTO_SLEEP_TAG
{
    EMC_AUTO_SLEEP_DEFAULT = 0,
    EMC_AUTO_SLEEP_DIS = 0,
    EMC_AUTO_SLEEP_EN = 1
}
EMC_AUTO_SLEEP_E;

typedef enum EMC_CMD_QUEUE_TAG
{
    EMC_2DB = 0,		// 2 stage device burst
    EMC_2DB_1CB,		// 2-stage device burst and 1-stage channel burst
    EMC_2DB_2CB		// 2-stage device burst and 2-stage channel burst
}
EMC_CMD_QUEUE_E;

typedef enum EMC_CS_MODE_TAG
{
    EMC_CS_MODE_DEFAULT = 0,
    EMC_CS0_ENLARGE = 1,
    EMC_CS1_ENLARGE = 2
}
EMC_CS_MODE_E;

typedef enum EMC_CS_MAP_TAG
{
    EMC_ONE_CS_MAP_DEFAULT      = 5,
    EMC_ONE_CS_MAP_32MBIT      = 0,
    EMC_ONE_CS_MAP_64MBIT      = 1,
    EMC_ONE_CS_MAP_128MBIT     = 2,
    EMC_ONE_CS_MAP_256MBIT     = 3,
    EMC_ONE_CS_MAP_512MBIT     = 4,
    EMC_ONE_CS_MAP_1GBIT       = 5,
    EMC_ONE_CS_MAP_2GBIT       = 6,
    EMC_ONE_CS_MAP_4GBIT       = 7,
    EMC_MAP_MAX                 = EMC_ONE_CS_MAP_4GBIT
}
EMC_CS_MAP_E;

typedef enum EMC_CS_NUM_TAG
{
    EMC_CS0 = 0,
    EMC_CS1
}
EMC_CS_NUM_E;

typedef enum EMC_BURST_MODE_TAG
{
    BURST_WRAP = 0,
    BURST_INCR
}
EMC_BURST_MODE_E;

typedef enum EMC_BURST_INVERT_TAG
{
    HBURST_TO_SINGLE = 0,
    HBURST_TO_BURST
}
EMC_BURST_INVERT_E;

typedef enum EMC_CHL_NUM_TAG
{
    EMC_CHL_MIN = 0,
    EMC_AXI_MIN = EMC_CHL_MIN,
    EMC_AXI_ARM    = EMC_AXI_MIN,
    EMC_AXI_GPU,
    EMC_AXI_DISPC,
	EMC_AXI_MAX     = EMC_AXI_DISPC,
    EMC_AHB_MIN,
    EMC_AHB_CP_MTX = EMC_AHB_MIN,
    EMC_AHB_MST_MTX,
    EMC_AHB_LCDC,
    EMC_AHB_DCAM,
    EMC_AHB_VSP,
	EMC_AHB_MAX     = EMC_AHB_VSP,
    EMC_CHL_MAX = EMC_AHB_MAX
}
EMC_CHL_NUM_E;


typedef enum EMC_CLK_SYNC_TAG
{
    EMC_CLK_ASYNC = 0,
    EMC_CLK_SYNC
}
EMC_CLK_SYNC_E;

typedef enum EMC_REF_CS_TAG
{
    EMC_CS_AREF_OBO = 0, //CSs auto-refresh one by one
    EMC_CS_AREF_ALL	//CSs auto-refresh at same time
}
EMC_CS_REF_E;

typedef enum EMC_CKE_SEL_TAG
{
    EMC_CKE_SEL_DEFAULT = 0,
    EMC_CKE_CS0 = 0,
    EMC_CKE_CS1 = 1,
    EMC_CKE_ALL_CS = 2
}
EMC_CKE_SEL_E;

typedef enum EMC_DQS_GATE_LOOP_TAG
{
    EMC_DQS_GATE_DEFAULT = 0,
    EMC_DQS_GATE_DL = 0,
    EMC_DQS_GATE_DL_LB = 1,
    EMC_DQS_GATE_LB = 2
}
EMC_DQS_GATE_LOOP_E;

typedef enum EMC_DQS_GATE_MODE_TAG
{
    EMC_DQS_GATE_MODE_DEFAULT = 0,
    EMC_DQS_GATE_MODE0 = 0,
    EMC_DQS_GATE_MODE1 = 1
}
EMC_DQS_GATE_MODE_E;

typedef enum EMC_PHYL1_TIMING_NUM_TAG
{
#ifdef SDR_SDRAM_SUPPORT
    EMC_PHYL1_TIMING_SDRAM_LATENCY2 = 0,
    EMC_PHYL1_TIMING_SDRAM_LATENCY3,
#endif
    EMC_PHYL1_TIMING_DDRAM_LATENCY2,
    EMC_PHYL1_TIMING_DDRAM_LATENCY3,
    EMC_PHYL1_TIMING_MATRIX_MAX
}EMC_PHYL1_TIMING_NUM_E;

typedef enum EMC_PHYL2_TIMING_NUM_TAG
{
    EMC_PHYL2_TIMING_DLL_OFF = 0,
    EMC_PHYL2_TIMING_DLL_ON,
    EMC_PHYL2_TIMING_MATRIX_MAX
}EMC_PHYL2_TIMING_NUM_E;

typedef enum SC8810_CLK_NUM_TAG
{
	ARM460_AHB230_EMC = 0,
	SC7702_CLK_MAX
}SC8810_CLK_E;


typedef enum
{
	SEQ = 0, 		//set memory sequenally
	ROW_BY_ROW,		//set memory row by row
	BANK_BY_BANK,	//set memory bank by bank
	COL_BY_COL		//set memory column by column
}MEM_ACC_TYPE_E;

typedef enum
{
    ZERO_TO_F = 0,
	F_TO_ZERO,
	FIVE_TO_A,
	A_TO_FIVE,
	FZERO_TO_ZEROF,
	ZEROF_TO_FZERO,
	BIGGER,
	LITTER
}BURST_DATA_TYPE_E;

typedef enum
{
    MCU_CLK_MPLL_SOURCE,
    MCU_CLK_TDPLL_DIV2_SOURCE,
    MCU_CLK_TDPLL_DIV3_SOURCE,
    MCU_CLK_XTL_SOURCE,
    MCU_CLK_NONE
}MCU_CLK_SOURCE_E;

typedef enum
{
    EMC_CLK_MPLL_DIV2_SOURCE,
    EMC_CLK_DPLL_SOURCE,
    EMC_CLK_TDPLL_DIV3_SOURCE,
    EMC_CLK_XTL_SOURCE,
    EMC_CLK_NONE
}EMC_CLK_SOURCE_E;

typedef enum EMC_CHANNEL_PRIORITY_TAG
{
    EMC_CHL_LOWEST_PRI = 0,
    EMC_CHL_PRI_0 = 0,
    EMC_CHL_PRI_1,
    EMC_CHL_PRI_2,
    EMC_CHL_PRI_3,
    EMC_CHL_HIGHEST_PRI = EMC_CHL_PRI_3,
    EMC_CHL_NONE
}EMC_CHL_PRI_E;


/******************************************************************************
                            Structure define
******************************************************************************/
typedef struct
{
	//uint8 row_ref_max;		//ROW_REFRESH_TIME,Refresh interval time , ns, tREF-max = 7800 ns
    uint16 trefi_max;
	uint8 row_pre_min;		//ROW_PRECHARGE_TIME , ns, tRP-min = 27 ns.
	uint8 rcd_min;     	// T_RCD,ACTIVE to READ or WRITE delay  , ns, tRCD-min = 27 ns
	uint32 wr_min;      	// T_WR  ,WRITE recovery time  , ns, tWR-min = 15 ns.
	uint8 rfc_min;     	//T_RFC, AUTO REFRESH command period , ns, tRFC-min = 80 ns.
	uint32 xsr_min;     	//T_XSR  , ns, tXSR-min = 120 ns.
	uint8 ras_min;     	//T_RAS_MIN , row active time, ns, tRAS-min = 50ns
    uint32 rrd_min;
	uint8 mrd_min;     	//T_MRD , 2 cycles, tMRD-min = 2 cycles.
	uint8 wtr_min;
}SDRAM_TIMING_PARA_T, *SDRAM_TIMING_PARA_T_PTR;

typedef struct SDRAM_CFG_INFO_TAG 
{
    SDRAM_ROW_MODE_E        row_mode;
    SDRAM_COLUMN_MODE_E     col_mode;
    DMEM_DATA_WIDTH_E       data_width;
    SDRAM_BURST_LEN_E       burst_length;
    SDRAM_CAS_LATENCY_E     cas_latency;
    uint32                  ext_mode_val;
    DMEM_TYPE_E             sdram_type;
    EMC_CS_MAP_E            cs_position;
} SDRAM_CFG_INFO_T, *SDRAM_CFG_INFO_T_PTR;


typedef struct SDRAM_MODE_TAG 
{
    SDRAM_CAP_TYPE_E        capacity;
    EMC_CS_MAP_E            cs_position;
    SDRAM_ROW_MODE_E        row_mode;
    SDRAM_COLUMN_MODE_E     col_mode;
    DMEM_DATA_WIDTH_E       data_width;
//    void *                  reserved;
} SDRAM_MODE_T, * SDRAM_MODE_PTR;


typedef struct EMC_PHY_L1_TIMING_TAG
{
	uint8 data_pad_ie_delay;
	uint8 data_pad_oe_delay;
	uint8 dqs_gate_pst_delay;
	uint8 dqs_gate_pre_delay;
	uint8 dqs_ie_delay;
	uint8 dqs_oe_delay;
}EMC_PHY_L1_TIMING_T,*EMC_PHY_L1_TIMING_T_PTR;

typedef struct EMC_PHY_L2_TIMING_TAG
{
	uint32 clkwr_dl_0;
	uint32 clkwr_dl_1;
	uint32 clkwr_dl_2;
	uint32 clkwr_dl_3;	
	uint32 dqs_gate_pre_dl_0;
	uint32 dqs_gate_pre_dl_1;
	uint32 dqs_gate_pre_dl_2;
	uint32 dqs_gate_pre_dl_3;
	uint32 dqs_gate_pst_dl_0;
	uint32 dqs_gate_pst_dl_1;
	uint32 dqs_gate_pst_dl_2;
	uint32 dqs_gate_pst_dl_3;
	uint32 dqs_in_pos_dl_0;
	uint32 dqs_in_pos_dl_1;
	uint32 dqs_in_pos_dl_2;
	uint32 dqs_in_pos_dl_3;
	uint32 dqs_in_neg_dl_0;
	uint32 dqs_in_neg_dl_1;
	uint32 dqs_in_neg_dl_2;
	uint32 dqs_in_neg_dl_3;
}EMC_PHY_L2_TIMING_T,*EMC_PHY_L2_TIMING_T_PTR;


typedef struct EMC_DRM_PARAM_TAG
{
	char                    chip_name[100];
	SDRAM_TIMING_PARA_T     time_param;
	SDRAM_CFG_INFO_T        cfg_info;
}
EMC_DRM_PARAM_T, *EMC_DRM_PARAM_T_PTR;


typedef struct SDRAM_CHIP_FEATURE_TAG
{
    uint8 cas;     // cas latency supported
    uint8 bl;      // burst length supported
    SDRAM_CAP_TYPE_E cap;
}
SDRAM_CHIP_FEATURE_T, *SDRAM_CHIP_FEATURE_T_PTR;


typedef struct
{
    CHIP_CLK_TYPE_E arm_clk;
    EMC_CLK_TYPE_E emc_clk;

    DDR_DRIVER_STRENGTH_T ddr_drv;  // DDR SDRAM driver strength in mode register

    uint8 dqs_drv;          //data_qs pin driver strength
    uint8 dat_drv;          //data pin driver strength
    uint8 ctl_drv;          //ctrl pin driver strength
    uint8 clk_drv;          //clock pin driver strength

    uint8 clk_wr;   // dll clk wr balance

    uint8 nand_id[5];
}EMC_PARAM_T, *EMC_PARAM_PTR;

typedef struct
{
    EMC_CHL_NUM_E emc_chl_num;
    EMC_CHL_PRI_E axi_chl_wr_pri;
    EMC_CHL_PRI_E axi_chl_rd_pri;
    EMC_CHL_PRI_E ahb_chl_pri;
}EMC_CHL_INFO_T, *EMC_CHL_INFO_PTR;



/*******************************************************************************
                          Parameter declare
*******************************************************************************/
    
extern uint32 DRAM_CAP;
//extern uint32  SDRAM_BASE;


/*******************************************************************************
                          Function declare
*******************************************************************************/


extern void sdram_init(void);

#endif
