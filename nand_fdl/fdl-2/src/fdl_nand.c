#include "fdl_nand.h"
#include "asm/arch/sci_types.h"
#if     defined CONFIG_NAND_SC8830 
#include <asm/arch/sprd_nfc_reg_v3.h>
#elif   defined CONFIG_NAND_TIGER
#include <asm/arch/sprd_nfc_reg_v2.h>
#elif   defined CONFIG_NAND_SC8810
#include <asm/arch/regs_nfc.h>
#else
#include "asm/arch/nand_controller.h"
#endif
#include <linux/mtd/mtd.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <jffs2/jffs2.h>
#include <parsemtdparts.h>
#include <asm/arch/cmd_def.h>
#include <asm/arch/secure_boot.h>
#include <asm/errno.h>
#include <config.h>
#include <common.h>
#include <asm/io.h>
#define CONFIG_SYS_NAND_READ_DELAY \
        { volatile int dummy; int i; for (i=0; i<10000; i++) dummy = i; }

#define CONFIG_SYS_NAND_BAD_BLOCK_POS   0
#define CONFIG_SYS_NAND_5_ADDR_CYCLE    1
static int pageinblock = 0;
struct mtd_info *_local_mtd = 0;

/*#define FDL2_DEBUG 1
#ifdef FDL2_DEBUG
#else
#define printf(arg...) do{}while(0)
#endif*/

typedef struct {
		unsigned char colParity;
		unsigned lineParity;
		unsigned lineParityPrime;
} yaffs_ECCOther;
typedef struct {
		unsigned sequenceNumber;
		unsigned objectId;
		unsigned chunkId;
		unsigned byteCount;
} yaffs_PackedTags2TagsPart;

typedef struct {
		yaffs_PackedTags2TagsPart t;
		yaffs_ECCOther ecc;
} yaffs_PackedTags2;

#define CONFIG_SYS_SPL_ECC_POS		8
#define MAX_SPL_SIZE    0x4000
#define FDL_NAND_BUF_LEN	(8192 + 512)

static unsigned int cur_write_pos;
static unsigned int is_system_write;
static unsigned char *backupblk = NULL;
static unsigned long backupblk_len = 0;
static unsigned long backupblk_flag = 0;
void set_current_write_pos(unsigned int pos)
{
	cur_write_pos = pos;
}
int nand_flash_init(void)
{
    return (nand_init());
}

int get_end_write_pos(void)
{
    return cur_write_pos;
}


int nand_format(void)
{
#ifdef FDL2_DEBUG
	printf("function: %s\n", __FUNCTION__);
#endif
	struct mtd_info *nand;
	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
	  return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];
	
#ifdef FDL2_DEBUG
	printf("function: %s nand_curr_device is %d\n", __FUNCTION__, nand_curr_device);
#endif
	unsigned int off = nand->erasesize; //TO SKIP THE FIRST BLOCK
	unsigned int size = nand->size - off; //to erase the left blocks

	nand_erase_options_t opts;
	memset(&opts, 0, sizeof(opts));
	opts.offset = off;
	opts.length = size;
	opts.jffs2 = 0;
	opts.scrub = 0;
	
#ifdef FDL2_DEBUG
	printf("function: %s erase off 0x%x length: 0x%x jffs2: 0x%x scurb: 0x%x\n",__FUNCTION__, opts.offset, opts.length, opts.jffs2, opts.scrub);
#endif
	return nand_erase_opts(nand, &opts);
}

int nand_erase_allflash(void)
{
	struct mtd_info *nand;
	int blocks;

	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
	  	return NAND_SYSTEM_ERROR;

	nand = &nand_info[nand_curr_device];
#if 0
	nand_erase_options_t opts;
	memset(&opts, 0, sizeof(opts));
	if(secureboot_enabled()){
		opts.offset = 0xc000;
		opts.length = nand->size - 0xc000;
	}else{
		opts.offset = 0;
		opts.length = nand->size;
	}
	opts.jffs2 = 0;
	opts.scrub = 1;
	opts.quiet = 0;
	printf("offset : 0x%016Lx size : 0x%016Lx\n", (unsigned long long)opts.offset, (unsigned long long)opts.length);

	return nand_erase_opts(nand, &opts);
#else
	blocks = nand->size / nand->erasesize;
	nand_scan_patition(blocks, nand->erasesize, nand->writesize);

	return NAND_SUCCESS;
#endif
}

int nand_erase_partition(unsigned int addr, unsigned int size, int skip_bad)
{
	struct mtd_partition cur_partition;
	int erase_blk, ret = 0;
	struct mtd_info *nand;

	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
	  	return NAND_SYSTEM_ERROR;

	nand = &nand_info[nand_curr_device];

	cur_partition.offset = addr;
	cur_partition.size = size;

	if (cur_partition.size == 0xffffffff){
		cur_partition.size = nand->size;
	}

	for (erase_blk = 0; erase_blk < (cur_partition.size / nand->erasesize); erase_blk ++) {
		printf("erasing block : %d    %d % \r", (cur_partition.offset / nand->erasesize + erase_blk), (erase_blk * 100 ) / (cur_partition.size / nand->erasesize));

		if (((cur_partition.offset + erase_blk * nand->erasesize) != 0x0) && skip_bad && nand_block_isbad(nand, cur_partition.offset + erase_blk * nand->erasesize)) {
			//printf("\nerase block : %d is bad\n", (cur_partition.offset / nand->erasesize + erase_blk));
			printf("skip bad block:	0x%x\n", cur_partition.offset + erase_blk * nand->erasesize);
		} else {
			//ret = nand_erase_fdl(cur_partition.offset + erase_blk * nand->erasesize, nand->erasesize);
			ret = nand_scan_block(cur_partition.offset /nand->erasesize+erase_blk,nand->erasesize,nand->writesize);
			if (ret != 0) {
				/* erase failed */
				ret = nand->block_markbad(nand, cur_partition.offset + erase_blk * nand->erasesize);
				if (ret != 0) {
					/* mark failed */
					printf("mark bad block:	0x%x	error!\n", cur_partition.offset + erase_blk * nand->erasesize);
				   	return NAND_SYSTEM_ERROR + 1;
				} else {
					/* mark success */
					printf("mark bad block:	0x%x	sucess!\n", cur_partition.offset + erase_blk * nand->erasesize);
				}
			}	
		}
	}
	
	return NAND_SUCCESS;
}

int move2goodblk(struct mtd_info *nand, int yaffs_flag)
{
	int pageno, size, pageall, ret;
	unsigned char buffer[FDL_NAND_BUF_LEN];

write2nextblk:
	printf("old write address : 0x%08x\n", cur_write_pos);
	cur_write_pos = (cur_write_pos + nand->erasesize) & (~(nand->erasesize - 1));
	printf("new write address : 0x%08x\n", cur_write_pos);
	//find a good block to write 
	while(!(cur_write_pos & (nand->erasesize-1))) {
		if (nand_block_isbad(nand, cur_write_pos&(~(nand->erasesize - 1)))) {
			printf("%s skip bad block 0x%x\n", __FUNCTION__, cur_write_pos&(~(nand->erasesize - 1)));
			cur_write_pos = (cur_write_pos + nand->erasesize)&(~(nand->erasesize - 1));
		} else
			break;
	}
	
	if (!yaffs_flag) {
		size = nand->writesize;
		pageall = backupblk_len / nand->writesize;
		for (pageno = 0; pageno < pageall; pageno ++) {
			memset(buffer, 0xff, FDL_NAND_BUF_LEN);
			memcpy(buffer, backupblk + pageno * size, size);
			ret = nand_write_skip_bad(nand, cur_write_pos, &size, buffer);
			if (0 == ret) {
				//////////////////////
				struct mtd_oob_ops ops;
				ops.mode = MTD_OOB_AUTO;
				ops.len = nand->writesize;
				ops.datbuf = (uint8_t *)buffer;
				ops.oobbuf = (uint8_t *)buffer + nand->writesize; 
				ops.ooblen = nand->oobsize; 
				ops.ooboffs = 0;
				memset(buffer, 0x0, FDL_NAND_BUF_LEN);
				ret = nand_do_read_ops(nand,(unsigned long long)(cur_write_pos),&ops);
				if (ret < 0) {
					printf("read error, mark bad block : 0x%08x\n", cur_write_pos);
					nand->block_markbad(nand, cur_write_pos&~(nand->erasesize - 1));
					printf("find next good block to write again\n");
					goto write2nextblk;
				}
				//////////////////////
				cur_write_pos += nand->writesize;
			} else {
				printf("write error, mark bad block : 0x%08x\n", cur_write_pos);
				nand->block_markbad(nand, cur_write_pos&~(nand->erasesize - 1));
				printf("find next good block to write again\n");
				goto write2nextblk;
			}
		} //for (pageno = 0; pageno < pageall; pageno ++)
	} else {//if (!yaffs_flag)
		struct mtd_oob_ops ops;
		size = nand->writesize + nand->oobsize;
		pageall = backupblk_len / size;
		for (pageno = 0; pageno < pageall; pageno ++) {
			memset(buffer, 0xff, size);
			memcpy(buffer, backupblk + pageno * size, size);
			ops.mode = MTD_OOB_AUTO;
			ops.len = nand->writesize;
			ops.datbuf = (uint8_t *)buffer;
			ops.oobbuf = (uint8_t *)buffer + nand->writesize;
			ops.ooblen = sizeof(yaffs_PackedTags2);
			ops.ooboffs = 0;
			ret = nand_do_write_ops(nand, (unsigned long long)cur_write_pos, &ops);
			if (0 == ret) {
				//////////////////////
				ops.mode = MTD_OOB_AUTO;
				ops.len = nand->writesize;
				ops.datbuf = (uint8_t *)buffer;
				ops.oobbuf = (uint8_t *)buffer + nand->writesize; 
				ops.ooblen = nand->oobsize; 
				ops.ooboffs = 0;
				memset(buffer, 0x0, FDL_NAND_BUF_LEN);
				ret = nand_do_read_ops(nand,(unsigned long long)(cur_write_pos),&ops);
				if (ret < 0) {
					printf("read error, mark bad block : 0x%08x\n", cur_write_pos);
					nand->block_markbad(nand, cur_write_pos&~(nand->erasesize - 1));
					printf("find next good block to write again\n");
					goto write2nextblk;
				}
				//////////////////////
				cur_write_pos += nand->writesize;
			} else {
				printf("write error, mark bad block : 0x%08x\n", cur_write_pos);
				nand->block_markbad(nand, cur_write_pos&~(nand->erasesize - 1));
				printf("find next good block to write again\n");
				goto write2nextblk;
			}
		} //for (pageno = 0; pageno < pageall; pageno ++)
	}

	return 0;
}

int nand_erase_fdl(unsigned int addr, unsigned int size)
{
#ifdef FDL2_DEBUG
	printf("function: %s\n", __FUNCTION__);
#endif
	struct mtd_info *nand;
	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
	  	return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];

	/*if(addr < nand->erasesize) //to skip the first block erase
	  return NAND_INVALID_ADDR;*/
	if(size != nand->erasesize)
	  	return NAND_INVALID_SIZE;
	
	nand_erase_options_t opts;
	memset(&opts, 0, sizeof(opts));
	opts.offset = addr;
	opts.length = size;
	opts.jffs2 = 0;
	opts.quiet = 1;
	if (addr == 0x0)
		opts.scrub = 1;
	/*else if ((nand->erasesize == MAX_SPL_SIZE) && (addr < (3 * nand->erasesize))) {
		//small page nand, block 0, 1 and 2 is reserved for spl
		opts.scrub = 1;
	}*/

#ifdef FDL2_DEBUG
	printf("function: %s erase off 0x%x length: 0x%x jffs2: 0x%x scurb: 0x%x\n",__FUNCTION__, opts.offset, opts.length, opts.jffs2, opts.scrub);
#endif
	return nand_erase_opts(nand, &opts);
}

unsigned long log2phy_table(struct real_mtd_partition *phypart)
{
	struct mtd_info *nand;
	unsigned long ret;
	
	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
		return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];
	if (phypart->offset == 0xffffffff)
		return NAND_INVALID_ADDR;
	//printf("nand->size = 0x%016Lx\n", (unsigned long long)nand->size);
	real_parse_cmdline_partitions(phypart, (unsigned long long)nand->size);
	if (phypart->offset == 0xffffffff)
		return NAND_INVALID_ADDR;
	if ((phypart->offset) & (nand->erasesize - 1))
	  	return NAND_INVALID_ADDR;

	return NAND_SUCCESS;
}

int get_nand_pageoob(NAND_PAGE_OOB_STATUS *nand_page_oob_info)
{
	struct mtd_info *nand;

	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
		return NAND_SYSTEM_ERROR;

	nand = &nand_info[nand_curr_device];
	memset(nand_page_oob_info, 0, sizeof(NAND_PAGE_OOB_STATUS));
	nand_page_oob_info->erasesize = nand->erasesize;
	nand_page_oob_info->writesize = nand->writesize;
	nand_page_oob_info->oobsize = nand->oobsize;

	return NAND_SUCCESS;
}

int nand_start_write(struct real_mtd_partition *phypart, unsigned int size, NAND_PAGE_OOB_STATUS *nand_page_oob_info, unsigned long noerasepartition)
{
	struct mtd_partition cur_partition;
	int erase_blk, ret = 0;

	struct mtd_info *nand;
	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
	  return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];
	//printf("nand->size = 0x%016Lx\n", (unsigned long long)nand->size);
	//printf("addr = 0x%08x  size = 0x%08x  flag = %d  part_size = 0x%08x\n", phypart->offset, size, phypart->yaffs, phypart->size);
	memset(nand_page_oob_info, 0, sizeof(NAND_PAGE_OOB_STATUS));
	nand_page_oob_info->erasesize = nand->erasesize;
	nand_page_oob_info->writesize = nand->writesize;
	nand_page_oob_info->oobsize = nand->oobsize;

	is_system_write = phypart->yaffs;

#ifdef FDL2_DEBUG
	printf("function %s, addr 0x%x, size 0x%x\n", __FUNCTION__, addr, size);
#endif

	backupblk_len = 0;
	backupblk_flag = 0;	
	if (backupblk == NULL)
		backupblk = (unsigned char *)malloc((nand->writesize + nand->oobsize) * (nand->erasesize / nand->writesize));

	if (backupblk == NULL) {
		printf("backup block malloc is wrong : %d\n", nand->erasesize);
		return NAND_SYSTEM_ERROR;
	}
	memset(backupblk, 0xff, (nand->writesize + nand->oobsize) * (nand->erasesize / nand->writesize));

	cur_write_pos = phypart->offset;

	cur_partition.offset = phypart->offset;
	cur_partition.size = phypart->size;

	if ((noerasepartition >= 0x90000022) && (noerasepartition <= 0x90000026))/* nvram */
		return NAND_SUCCESS;

	for (erase_blk = 0; erase_blk < (cur_partition.size / nand->erasesize); erase_blk ++) {
		printf("erasing block : %d    %d % \r", (cur_partition.offset / nand->erasesize + erase_blk), (erase_blk * 100 ) / (cur_partition.size / nand->erasesize));
		if (((cur_partition.offset + erase_blk * nand->erasesize) != 0x0) && nand_block_isbad(nand, cur_partition.offset + erase_blk * nand->erasesize)) {
			printf("\nerase block : %d is bad\n", (cur_partition.offset / nand->erasesize + erase_blk));
		} else {
			ret = nand_erase_fdl(cur_partition.offset + erase_blk * nand->erasesize, nand->erasesize);
			if (ret != 0) {
				/* erase failed */
				ret = nand->block_markbad(nand, cur_partition.offset + erase_blk * nand->erasesize);
				if (ret != 0) {
					/* mark failed */
					printf("mark 0x%x bad error\n", cur_partition.offset + erase_blk * nand->erasesize);
				   	return NAND_SYSTEM_ERROR + 1;
				} else {
					/* mark success */
					printf("skip bad block 0x%x\n", cur_partition.offset + erase_blk * nand->erasesize);
				}
			}

		}
	}
	
	return NAND_SUCCESS;
}

int nand_do_write_ops(struct mtd_info *mtd, loff_t to,struct mtd_oob_ops *ops);
int nand_do_write_oob(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops);
int nand_do_read_ops(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops);
#ifdef CONFIG_NAND_SC8810

#ifdef CONFIG_NAND_SC7710G2
struct bootloader_header
{
	uint32_t version; //version, fot tiger this member must be 0
	uint32_t magic_num; //0xaa55a5a5
	uint32_t sct_size; //
	uint32_t hash_len;//word length, only used when secure boot enable
	uint32_t acycle; // 3, 4, 5
	uint32_t bus_width; //0 ,1
	uint32_t spare_size; //spare part sise for one sector
	uint32_t ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	uint32_t ecc_pos; // ecc postion at spare part
	uint32_t sct_per_page; //sector per page
	uint32_t info_pos;
	uint32_t info_size;
	uint32_t ecc_value[27];
};

/*
 * spare info data is don't used at the romcode, so the fdl only set the s_info size to 1, and the data value 0xff
 */
void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
	struct bootloader_header *header;
	struct nand_chip *chip = nand->priv;
	struct sc8810_ecc_param param;
	u8 ecc[44];
	header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
	memset(header, 0, sizeof(struct bootloader_header));
	memset(ecc, 0xff, sizeof(ecc));

	header->version = 0x1;
	header->sct_size = chip->ecc.size;
	header->hash_len = 0x400;
	if (chip->options & NAND_BUSWIDTH_16){
		header->bus_width = 1;
	}
	if(nand->writesize > 512) {
		if (chip->chipsize > (128 << 20)){
			header->acycle = 5;
		}
		else{
			header->acycle = 4;
		}
	}
	else{
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20)){
			header->acycle = 3;
		}
		else{
			header->acycle = 3;
		}
	}

	header->acycle -= 3;

	header->magic_num = 0xaa55a5a5;
	header->spare_size = (nand->oobsize/chip->ecc.steps);
	header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
	header->ecc_pos = ecc_pos;
	header->sct_size = (nand->writesize/chip->ecc.steps);

	header->sct_per_page = chip->ecc.steps;

	param.mode = 24;
	param.ecc_num = 1;
	param.sp_size = sizeof(ecc);
	param.ecc_pos = 0;
	param.m_size = chip->ecc.size;
	param.p_mbuf = (u8 *)bl_data;
	param.p_sbuf = ecc;

	sc8810_ecc_encode(&param);
	memcpy(header->ecc_value, ecc, sizeof(ecc));

}

#else

struct bootloader_header
{
	u32 check_sum;
	u32 page_type; //page type 0-512,1-1K,2-2k,3-4k,4-8K
	u32 acycle; // 3, 4, 5
	u32 bus_width; //0 ,1
	u32 advance; // 0, 1
	u32 magic_num; //0xaa55a5a5
	u32 spare_size; //spare part sise for one sector
	u32 ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	u32 ecc_pos; // ecc postion at spare part
	u32 sct_size; //sector size;
	u32 sct_per_page; //sector per page
	u32 ecc_value[11];
};
u32 get_nand_page_type(u32 pg_sz)
{
	u32 pg_type = 0;
	switch(pg_sz)
	{
	case 512:
		pg_type = 0;
		break;
	case 1024:
		pg_type = 1;
		break;
	case 2048:
		pg_type = 2;
		break;
	case 4096:
		pg_type = 3;
		break;
	case 892:
		pg_type = 4;
		break;		
	default:
		while(1);
		break;								
	}
	return pg_type;
}
extern unsigned short CheckSum(const unsigned int *src, int len);
void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
	struct bootloader_header *header;
	struct nand_chip *chip = nand->priv;
	struct sc8810_ecc_param param;
	u8 ecc[44];
	header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
	memset(header, 0, sizeof(struct bootloader_header));
	memset(ecc, 0, sizeof(ecc));
#if 1
	header->page_type = get_nand_page_type(nand->writesize);
	if (chip->options & NAND_BUSWIDTH_16)	{
		header->bus_width = 1;
	}
	if(nand->writesize > 512)	{
		header->advance = 1;
		/* One more address cycle for devices > 128MiB */
		if (chip->chipsize > (128 << 20))		{
			header->acycle = 5;
		}
		else	 {
			header->acycle = 4;
		}
	}
	else{
		header->advance = 0;
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20)) {
			header->acycle = 3;
		}
		else	{
			header->acycle = 3;
		}
	}
	header->magic_num = 0xaa55a5a5;
	header->spare_size = (nand->oobsize/chip->ecc.steps);
	header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
	header->ecc_pos = ecc_pos;
	header->sct_size = (nand->writesize/chip->ecc.steps);
	header->sct_per_page = chip->ecc.steps;
	header->check_sum = CheckSum((unsigned int *)(bl_data + BOOTLOADER_HEADER_OFFSET + 4), (NAND_PAGE_LEN - BOOTLOADER_HEADER_OFFSET - 4));

	param.mode = 24;
	param.ecc_num = 1;
	param.sp_size = sizeof(ecc);
	param.ecc_pos = 0;
	param.m_size = chip->ecc.size;
	param.p_mbuf = (u8 *)bl_data;
	param.p_sbuf = ecc;
	sc8810_ecc_encode(&param);
	memcpy(header->ecc_value, ecc, sizeof(ecc));
#endif	
}

#endif

int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
{
	int eccsteps;
	u32 eccsize;
	struct nand_chip *chip = mtd->priv;
	int eccbytes = chip->ecc.bytes;
	u32 i;
	u32 page;
	u32 spare_per_sct;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	eccsteps = chip->ecc.steps;
	eccsize = chip->ecc.size;
	spare_per_sct = mtd->oobsize / eccsteps;
	memset(chip->buffers->ecccode, 0xff, mtd->oobsize);
	
	page = (int)(pg >> chip->page_shift);
	
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);
	
	for (i = 0; i < eccsteps; i ++, buf += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, buf, eccsize);
		chip->ecc.calculate(mtd, buf, &ecc_calc[0]);
		memcpy(chip->buffers->ecccode + i * spare_per_sct + ecc_pos, &ecc_calc[0], eccbytes);
	}
	chip->write_buf(mtd, chip->buffers->ecccode, mtd->oobsize);
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	chip->waitfunc(mtd, chip);
	
	return NAND_SUCCESS;
}
int nand_write_spl(u8 *buf, struct mtd_info *mtd)
{
	u32 i;
	u32 pg_start;
	u32 pg_end;
	u32 pg;
	u8 * data;
	int ret = NAND_SUCCESS;

	//struct nand_chip *chip = mtd->priv;
	set_header_info(buf, mtd, CONFIG_SYS_SPL_ECC_POS);
	/* erase spl block in nand_start_write(), so erase spl block code is deleted */

	/* write spl to flash*/
	for (i = 0; i < 3; i++) {
		pg_start = i * MAX_SPL_SIZE;
		pg_end = (i + 1) * MAX_SPL_SIZE;
		data = buf;
		for(pg  = pg_start; pg < pg_end; pg += mtd->writesize) {
			ret = nand_write_spl_page(data, mtd, pg, CONFIG_SYS_SPL_ECC_POS);
			data += mtd->writesize;
		}
	}
	
	return ret;	
}
#endif

#if defined CONFIG_NAND_SC8830 || defined CONFIG_NAND_TIGER
#ifdef CONFIG_NAND_SC8830
struct bootloader_header
{
	uint32_t version; //version, fot tiger this member must be 0
	uint32_t magic_num; //0xaa55a5a5	
	uint32_t check_sum;
	uint32_t hash_len; //word length, only used when secure boot enable
	uint32_t sct_size; //
	uint32_t acycle; // 3, 4, 5
	uint32_t bus_width; //0 ,1
	uint32_t spare_size; //spare part sise for one sector
	uint32_t ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	uint32_t ecc_pos; // ecc postion at spare part
	uint32_t sct_per_page; //sector per page
	uint32_t info_pos;
	uint32_t info_size;
	uint32_t ecc_value[27];
}
#elif defined CONFIG_NAND_TIGER
struct bootloader_header
{
	uint32_t version; //version, fot tiger this member must be 0
	uint32_t check_sum;
	uint32_t sct_size; //
	uint32_t acycle; // 3, 4, 5
	uint32_t bus_width; //0 ,1
	uint32_t spare_size; //spare part sise for one sector
	uint32_t ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	uint32_t ecc_pos; // ecc postion at spare part
	uint32_t sct_per_page; //sector per page
	uint32_t info_pos;
	uint32_t info_size;
	uint32_t hash_len; //word length, only used when secure boot enable
	uint32_t magic_num; //0xaa55a5a5	
	uint32_t ecc_value[11];
};
#endif
extern unsigned short CheckSum(const unsigned int *src, int len);
/*
 * spare info data is don't used at the romcode, so the fdl only set the s_info size to 1, and the data value 0xff
 */
void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
	struct bootloader_header *header;
	struct nand_chip *chip = nand->priv;
	struct sprd_ecc_param param;
#ifdef CONFIG_NAND_SC8830
	u8 ecc[108];
#else
	u8 ecc[44];
#endif
	header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
	memset(header, 0, sizeof(struct bootloader_header));
	memset(ecc, 0xff, sizeof(ecc));
#ifdef CONFIG_NAND_SC8830
	header->version = 1;
#else
	header->version = 0;
#endif
	header->sct_size = chip->ecc.size;
	if (chip->options & NAND_BUSWIDTH_16)	{
		header->bus_width = 1;
	}
	if(nand->writesize > 512) {
		if (chip->chipsize > (128 << 20))		{
			header->acycle = 5;
		}
		else	 {
			header->acycle = 4;
		}
	}
	else{
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20)) {
			header->acycle = 4;
		}
		else	{
			header->acycle = 3;
		}
	}
	header->magic_num = 0xaa55a5a5;
	header->spare_size = (nand->oobsize/chip->ecc.steps);
	header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
	/*ecc is at the last postion at spare part*/
	header->ecc_pos = header->spare_size - (chip->ecc.bytes);
	header->sct_per_page = chip->ecc.steps;
	
	header->sct_per_page = chip->ecc.steps;
	header->info_pos = header->ecc_pos - 1;
	header->info_size = 1;
	
#ifdef CONFIG_NAND_SC8830
	param.mode = 60;
	param.ecc_pos = 0;
	param.sinfo_size = 0;
#else
	header->check_sum = CheckSum((unsigned int *)(bl_data + BOOTLOADER_HEADER_OFFSET + 4), (NAND_PAGE_LEN - BOOTLOADER_HEADER_OFFSET - 4));
	param.mode = 24;
	param.ecc_pos = 1;
	param.sinfo_size = 1;
#endif
	param.ecc_num = 1;
	param.sp_size = sizeof(ecc);
	param.m_size = chip->ecc.size;
	param.p_mbuf = (u8 *)bl_data;
	param.p_sbuf = ecc;
	param.sinfo_pos = 0;
	sprd_ecc_encode(&param);
	memcpy(header->ecc_value, ecc, sizeof(ecc));
}
int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
{
	struct nand_chip *chip = mtd->priv;
	int status;
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, pg);
	chip->ecc.write_page(mtd, chip, buf);
	chip->ecc.write_page(mtd, chip, buf);
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	chip->waitfunc(mtd, chip);
	if (status & NAND_STATUS_FAIL) {
		return -1;
	}
	if (status & NAND_STATUS_FAIL) {
		return -1;
	}
	return NAND_SUCCESS;
}
int nand_write_spl(u8 *buf, struct mtd_info *mtd)
{
	u32 i;
	u32 pg_start;
	u32 pg_end;
	u32 pg;
	u8 * data;
	int ret = NAND_SUCCESS;

	//struct nand_chip *chip = mtd->priv;
	set_header_info(buf, mtd, CONFIG_SYS_SPL_ECC_POS);
	/* erase spl block in nand_start_write(), so erase spl block code is deleted */

	/* write spl to flash*/
	for (i = 0; i < 3; i++) {
		pg_start = i * MAX_SPL_SIZE / mtd->writesize;
		pg_end = (i + 1) * MAX_SPL_SIZE / mtd->writesize;
		data = buf;
		for(pg  = pg_start; pg < pg_end; pg += 1) {
			ret = nand_write_spl_page(data, mtd, pg, CONFIG_SYS_SPL_ECC_POS);
			data += mtd->writesize;
		}
	}
	
	return ret;	
}
#endif

#ifdef CONFIG_NAND_SC7710G2
void get_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
    struct bootloader_header *header;
    struct nand_chip *chip = nand->priv;
    struct sc8810_ecc_param param;

    u8 ecc[44];
    header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
    memset(header, 0, sizeof(struct bootloader_header));
    memset(ecc,0xff, sizeof(ecc));
    header->version = 0x1;
    header->sct_size = chip->ecc.size;
    header->hash_len = 0x400;
    //header->page_type = get_nand_page_type(nand->writesize);
    if (chip->options & NAND_BUSWIDTH_16)       {
        header->bus_width = 1;
    }
    if(nand->writesize > 512)   {
     /*  header->advance = 1;
         One more address cycle for devices > 128MiB */
        if (chip->chipsize > (128 << 20))               {
            header->acycle = 5;
        }
        else     {
            header->acycle = 4;
        }
    }
    else{
       /* header->advance = 0;
         One more address cycle for devices > 32MiB */
        if (chip->chipsize > (32 << 20)) {
            header->acycle = 3;
        }
        else    {
            header->acycle = 3;
        }
    }

    header->acycle -= 3;
    header->magic_num = 0xaa55a5a5;
    header->spare_size = (nand->oobsize/chip->ecc.steps);
    header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
    header->ecc_pos = ecc_pos;
    header->sct_size = (nand->writesize/chip->ecc.steps);
    header->sct_per_page = chip->ecc.steps;
    //header->check_sum = CheckSum((unsigned int *)(bl_data + BOOTLOADER_HEADER_OFFSET + 4), (NAND_PAGE_LEN - BOOTLOADER_HEADER_OFFSET - 4));

    param.mode = 24;
    param.ecc_num = 1;
    param.sp_size = sizeof(ecc);
    param.ecc_pos = 0;
    param.m_size = chip->ecc.size;
    param.p_mbuf = (u8 *)bl_data;
    param.p_sbuf = ecc;

    sc8810_ecc_decode(&param);
    memcpy(header->ecc_value, ecc, sizeof(ecc));
}

static int nand_is_bad_block(struct mtd_info *mtd, int block)
{
#if 0
    struct nand_chip *this = mtd->priv;

    nand_command(mtd, block, 0, CONFIG_SYS_NAND_BAD_BLOCK_POS, NAND_CMD_READOOB);

    /*
     * * Read one byte
     * */
    if (readb(this->IO_ADDR_R) != 0xff)
        return 1;
#endif
    return 0;
}

static int nand_read_spl_page(struct mtd_info *mtd, int block, int page, u32 *dst)
{
    struct nand_chip *this = mtd->priv;
    u_char *ecc_calc;
    u_char *ecc_code;
    u_char *oob_data;
    int i;
    int eccsize = this->ecc.size;
    int eccbytes = this->ecc.bytes;
    int eccsteps = mtd->writesize / eccsize;
    int ecctotal = eccbytes * eccsteps;

    uint8_t *p = dst;
    int stat;
    printf("eccsize=0x%x ; eccbytes=0x%x ; eccsteps=0x%x ; ecctotal=0x%x\r\n",eccsize,eccbytes,eccsteps,ecctotal);

    this->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);

    ecc_calc = (u_char *)(CONFIG_SYS_SDRAM_BASE + 0x10000);
    ecc_code = ecc_calc + 0x100;
    oob_data = ecc_calc + 0x200;

    for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
        this->ecc.hwctl(mtd, NAND_ECC_READ);
        this->read_buf(mtd, p, eccsize);
        this->ecc.calculate(mtd, p, &ecc_calc[i]);
    }

    this->read_buf(mtd, oob_data, mtd->oobsize);
    return 0;
}

static int nand_load_spl(struct mtd_info *mtd, unsigned int offs,
        unsigned int spl_size, uchar *dst)
{
    unsigned int block, lastblock;
    static unsigned int page = 0;
    static unsigned int total_page = 0;

    block = offs / mtd->erasesize;
    total_page += spl_size / mtd->writesize;
    lastblock = (offs + spl_size - 1) / mtd->erasesize;
    page = (offs % mtd->erasesize) / mtd->writesize;
    pageinblock = (mtd->erasesize) / mtd->writesize;
    /*if(total_page == pageinblock)
     * {
     * page = 0;
     * total_page = 0;
     * }*/
    printf("block=%d ; lastblock=%d ; page=%d ; pageinblock=%d\r\n",block,lastblock,page,pageinblock);
    while (block <= lastblock) {
        if (!nand_is_bad_block(mtd, block)) {
            /*
             * * Skip bad blocks
             * */
            printf("<<<<<<<<<<<<<<<<<<<<<<<<<<start read>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
            while (page < total_page) {
                printf("page=%d\r\n",page);
                nand_read_spl_page(mtd, block, page, dst);
                dst += mtd->writesize;
                page++;
            }

            page = 0;
        } else {
            lastblock++;
        }

        block++;
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<start end>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
    }

    return 0;
}
#endif

int nand_write_fdl(unsigned int size, unsigned char *buf)
{
#ifdef FDL2_DEBUG
	printf("function: %s   %d\n", __FUNCTION__, __LINE__);
#endif
	struct mtd_info *nand;
	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
		return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];
	int ret=0;
	int pos;
	unsigned char buffer[FDL_NAND_BUF_LEN];

#if (defined(CONFIG_NAND_SC8810) || defined(CONFIG_NAND_TIGER) || defined(CONFIG_NAND_SC8830))//only for sc8810 to write spl
	if(cur_write_pos < 0xc000) {
		if(secureboot_enabled()) //if secure boot enabled, don't update spl
			return NAND_SUCCESS;
		return nand_write_spl(buf, nand);
	}
#endif
	//find a good block to write 
	while(!(cur_write_pos & (nand->erasesize-1))) {
#ifdef FDL2_DEBUG
		printf("function: %s to check bad block, check address 0x%x\n", __FUNCTION__,cur_write_pos&(~(nand->erasesize - 1)));
#endif
		if (nand_block_isbad(nand, cur_write_pos&(~(nand->erasesize - 1)))) {
#ifdef FDL2_DEBUG
			printf("function: %s block is bad\n",__FUNCTION__);
#endif
			printf("%s skip bad block 0x%x\n", __FUNCTION__, cur_write_pos&(~(nand->erasesize - 1)));
			cur_write_pos = (cur_write_pos + nand->erasesize)&(~(nand->erasesize - 1));
		} else {
#ifdef FDL2_DEBUG
			printf("function: %s block is good\n", __FUNCTION__);
#endif
			break;
		}
	}
	if(!is_system_write){
#ifdef FDL2_DEBUG
		printf("function: %s cur_write_pos: 0x%x size: 0x%x\n", __FUNCTION__, cur_write_pos, size);
#endif
		//printf("cur_write_pos: 0x%x size: 0x%x\n", cur_write_pos, size);
		if(size < nand->writesize) {
			/* set 0xff from buf[size -- writesize] for fixnv and runtimenv */
			for (pos = size; pos < nand->writesize; pos ++)
				buf[pos] = 0xff;
		  	size = nand->writesize;	
		} else if(size > nand->writesize)	
		  		return NAND_INVALID_SIZE;
#ifdef FDL2_DEBUG
		printf("function: %s erase done, now to write it\n", __FUNCTION__);
#endif
		/* backup */
		memcpy(backupblk + backupblk_len, buf, size);
		backupblk_len += size;
		if (backupblk_len >= nand->erasesize)
			backupblk_flag = 1; /* full */
		else
			backupblk_flag = 0;
		
		ret = nand_write_skip_bad(nand, cur_write_pos, &size, buf);

#if 0
		/* fcj test */
		if (cur_write_pos == (0x00bc0000 - 0x800))
			ret = -1;
#endif

		if (0 == ret) {
			cur_write_pos += nand->writesize;
		} else {
			printf("\nwrite error, mark bad block : 0x%08x\n", cur_write_pos);
			nand->block_markbad(nand, cur_write_pos&~(nand->erasesize - 1));
			printf("find new good partition to move and write data again\n");
			move2goodblk(nand,is_system_write);
			printf("move and write end. new write pos : 0x%08x\n", cur_write_pos);
			ret = 0;
			/* all success not failed */
			//return NAND_SYSTEM_ERROR;
		}
	}else{ // system write 
#ifdef FDL2_DEBUG
		printf("function: %s system write cur_write_pos: 0x%x size: 0x%x\n", __FUNCTION__, cur_write_pos, size);
#endif
		if(size != (nand->writesize + nand->oobsize))
		  	return NAND_INVALID_SIZE;

		struct nand_chip *chip = nand->priv;

		chip->ops.mode = MTD_OOB_AUTO;
		chip->ops.len = nand->writesize;
		chip->ops.datbuf = (uint8_t *)buf;
		chip->ops.oobbuf = (uint8_t *)buf + nand->writesize;
		chip->ops.ooblen = sizeof(yaffs_PackedTags2);
		chip->ops.ooboffs = 0;

#ifdef FDL2_DEBUG
		printf("function: %s system write start to write\n", __FUNCTION__);
#endif

		/* backup */
		memcpy(backupblk + backupblk_len, buf, size);
		backupblk_len += size;
		if (backupblk_len >= (nand->writesize + nand->oobsize) * (nand->erasesize / nand->writesize))
			backupblk_flag = 1; /* full */
		else
			backupblk_flag = 0;
		
		ret = nand_do_write_ops(nand, (unsigned long long)cur_write_pos, &(chip->ops));
#if 0
		/* fcj test */
		if ((cur_write_pos == (0x02bc0000 - 0x800)) || (cur_write_pos == (0x0bfc0000 - 0x800)))
			ret = -1;
#endif
		if (0 == ret) {
			cur_write_pos += nand->writesize;
		} else {
			printf("\nwrite error, mark bad block : 0x%08x\n", cur_write_pos);
			nand->block_markbad(nand, cur_write_pos&~(nand->erasesize - 1));
			printf("find new good partition to move and write data again\n");
			move2goodblk(nand, is_system_write);
			printf("move and write end. new write pos : 0x%08x\n", cur_write_pos);
			ret = 0;
			/* all success not failed */
			//return NAND_SYSTEM_ERROR;
		}
	}

	if(ret == 0) {
		struct mtd_oob_ops ops;
		ops.mode = MTD_OOB_AUTO;
		ops.len = nand->writesize;
		ops.datbuf = (uint8_t *)buffer;
		ops.oobbuf = (uint8_t *)buffer + nand->writesize; 
		ops.ooblen = nand->oobsize; 
		ops.ooboffs = 0;
		memset(buffer, 0x0, FDL_NAND_BUF_LEN);
		ret = nand_do_read_ops(nand,(unsigned long long)(cur_write_pos-nand->writesize),&ops);
		if (ret < 0) {
			cur_write_pos -= nand->writesize;
			printf("\nread error, mark bad block : 0x%08x\n", cur_write_pos);
			nand->block_markbad(nand, cur_write_pos & ~(nand->erasesize - 1));
			printf("find new good partition to move and write data again, then read data\n");
			move2goodblk(nand, is_system_write);
			printf("move and write and read end. new write pos : 0x%08x\n", cur_write_pos);
			/* can not cur_write_pos + nand->writesize */
			//return NAND_SYSTEM_ERROR;
		}

		if (backupblk_flag) {
			memset(backupblk, 0xff, (nand->writesize + nand->oobsize) * (nand->erasesize / nand->writesize));
			backupblk_len = 0;
			backupblk_flag = 0;
		}

		return NAND_SUCCESS;
	} else {
		/* can not run here */
		return NAND_SYSTEM_ERROR;
		}
}

int nand_end_write(void)
{
	struct mtd_info *nand;

	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
		return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];

	cur_write_pos = NULL;
	memset(backupblk, 0xff, (nand->writesize + nand->oobsize) * (nand->erasesize / nand->writesize));
	backupblk_len = 0;
	backupblk_flag = 0;
	return NAND_SUCCESS;
}

int nand_read_fdl(struct real_mtd_partition *phypart, unsigned int off, unsigned int size, unsigned char *buf)
{
	struct mtd_info *nand;
	int ret=0;
	int pos;
	unsigned char buffer[FDL_NAND_BUF_LEN];
	static unsigned long addr;
        static unsigned long skip_addr = 0;
	static unsigned int cur_read_pos = 0;
        int offset;

        if(off == 0){
                addr = phypart->offset;
		cur_read_pos = phypart->offset;
		skip_addr = 0x0;
		}

	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
		return NAND_SYSTEM_ERROR;
	nand = &nand_info[nand_curr_device];

	printf("addr = 0x%08x  size = %d  off = %d ; cur_read_pos= 0x%x\r\n", addr, size, off,cur_read_pos);
	while (!(cur_read_pos & (nand->erasesize - 1))) {

		printf("function: %s to check bad block, check address 0x%x\n", __FUNCTION__,cur_read_pos&(~(nand->erasesize - 1)));
		if (nand_block_isbad(nand, cur_read_pos&(~(nand->erasesize - 1)))) {
			printf("skip bad block 0x%x\r\n", cur_read_pos&(~(nand->erasesize - 1)));

			cur_read_pos = (cur_read_pos + nand->erasesize)&(~(nand->erasesize - 1));
                        addr = cur_read_pos;

		} else {
			printf("good block 0x%x\n", addr & (~(nand->erasesize - 1)));
			break;
		}
	}
	//cur_read_pos += nand->writesize;
        printf("read flash addr :0x%x, off=0x%x ; cur_read_pos=0x%x \r\n", (addr+off), off,cur_read_pos);
	if((strcmp(phypart->name, "system") == 0) || (strcmp(phypart->name, "userdata") == 0)){

		if(size != (nand->writesize + nand->oobsize))
		  	return NAND_INVALID_SIZE;
		struct nand_chip *chip = nand->priv;

		chip->ops.mode = MTD_OOB_AUTO;
		chip->ops.len = nand->writesize;
		chip->ops.datbuf = (uint8_t *)buf;
		chip->ops.oobbuf = (uint8_t *)(buf + nand->writesize);
		chip->ops.ooblen = sizeof(yaffs_PackedTags2);
		chip->ops.ooboffs = 0;
		//memset(buffer, 0xff, FDL_NAND_BUF_LEN);
		memset(buf, 0xff, size);
		ret = nand_do_read_ops(nand,(unsigned long long)(addr + off),&chip->ops);
		if(ret < 0)
		{
			printf("\nread error, mark bad block : 0x%08x\n", addr);
			nand->block_markbad(nand, addr & ~(nand->erasesize - 1));
			return NAND_SYSTEM_ERROR;
		}
	}else{
		struct mtd_oob_ops ops;
		ops.mode = MTD_OOB_AUTO;
		ops.len = nand->writesize;
		ops.datbuf = (uint8_t *)buf;
		ops.oobbuf = (uint8_t *)buffer;
		ops.ooblen = nand->oobsize;
		//ops.ooboffs = 0;
		memset(buffer, 0xff, FDL_NAND_BUF_LEN);
		memset(buf, 0xff, size);
		if((strcmp(phypart->name, "spl") == 0))
			ret = nand_load_spl(nand, (unsigned int)(addr + off),size, buf);
		else
			ret = nand_do_read_ops(nand,(unsigned long long)(addr + off), &ops);
		if (ret<0) {
			printf("\nread error, mark bad block : 0x%08x\n", addr);
			nand->block_markbad(nand, addr & ~(nand->erasesize - 1));
			return NAND_SYSTEM_ERROR;
		}

	}
	cur_read_pos += nand->writesize;
	return NAND_SUCCESS;
}

int nand_read_NBL(void *buf)
{
	printf("function: %s\n", __FUNCTION__);
	return NAND_SUCCESS;
}

void nand_set_write_pos(unsigned int pos){
	cur_write_pos = pos;
}

unsigned int nand_check_write_pos(unsigned int  pos){
	return (cur_write_pos <= pos);
}
