#include "hdraw.h"

#define ATA_IDENTIFY    0xEC
#define ATA_READ        0x20
#define ATA_WRITE       0x30

#define REG_DEV_CTRL  0x3F6
#define REG_DATA      0x1F0
#define REG_FEATURES  0x1F1
#define REG_ERROR     0x1F1
#define REG_NSECTOR   0x1F2
#define REG_LBA_LOW   0x1F3
#define REG_LBA_MID   0x1F4
#define REG_LBA_HIGH  0x1F5
#define REG_DEVICE    0x1F6
#define REG_STATUS    0x1F7
#define REG_COMMAND   0x1F7

#define	STATUS_BSY  0x80
#define	STATUS_DRDY 0x40
#define	STATUS_DFSE 0x20
#define	STATUS_DSC  0x10
#define	STATUS_DRQ  0x08
#define	STATUS_CORR 0x04
#define	STATUS_IDX  0x02
#define	STATUS_ERR  0x01

extern byte ReadPort(ushort port);
void WritePort(ushort port, byte value);

static uint IsBusy()
{
	uint ret = 0;
	uint i = 0;

	while( ret = ((i < 500) && (ReadPort(REG_STATUS) & STATUS_BSY) ) )
	{
		i++;
	}

	return ret;
}

static uint IsDevReady()
{
	return !(ReadPort(REG_STATUS) & STATUS_DRDY);
}

static uint IsDataReady()
{
	return ReadPort(REG_STATUS) & STATUS_DRQ;
}

static uint MakeDevRegVal(uint si)
{
	return 0xE0 | ((si >> 24) & 0x0f);
}

void HDRawModInit();
uint HDRawSectors();
uint HDRawWrite(uint si, byte* buf);
uint HDRawRead(uint si, byte* buf);

