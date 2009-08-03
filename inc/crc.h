/* vim: set tabstop=8 shiftwidth=8: */
//=============================================================================
// Name: crc.h
// Purpose: Calculate 16-bit or 32-bit CRC for several 8-bit data
// To build: gcc -std-c99 -c crc.c
//=============================================================================

#ifndef _CRC_H
#define _CRC_H

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Variables definition:
//=============================================================================
enum
{
        MODE_CRC32,
        MODE_CRC16,
        MODE_CRC8
};

/*============================================================================
 * Public Function Declaration
 ===========================================================================*/
void crc_init();
uint16_t crc16(void *buf, size_t size);
uint32_t crc32(void *buf, size_t size);
uint32_t CRC  (void *buf, size_t size, int mode);

#ifdef __cplusplus
}
#endif

#endif // _CRC_H

/*****************************************************************************
 * End
 ****************************************************************************/
