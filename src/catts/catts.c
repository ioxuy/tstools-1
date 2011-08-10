/*
 * vim: set tabstop=8 shiftwidth=8:
 * name: catts.c
 * funx: generate text line with bin ts file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcmp, etc */
#include <stdint.h> /* for uintN_t, etc */

#include "libts/common.h"
#include "libts/error.h"
#include "libts/if.h"

enum FILE_TYPE
{
        FILE_MTS,
        FILE_TSRS,
        FILE_TS,
        FILE_UNKNOWN
};

static FILE *fd_i = NULL;
static char file_i[FILENAME_MAX] = "";
static int npline = 188; /* data number per line */
static int type = FILE_TS;
static int aim_start = 0; /* first byte */
static int aim_stop = 0; /* last byte */
static ts_pkt_t PKT;
static ts_pkt_t *pkt = &PKT;

static int deal_with_parameter(int argc, char *argv[]);
static int show_help();
static int show_version();
static int judge_type();
static int mts_time(uint64_t *mts, uint8_t *bin);

int main(int argc, char *argv[])
{
        unsigned char bbuf[LINE_LENGTH_MAX / 3 + 10]; /* bin data buffer */
        char tbuf[LINE_LENGTH_MAX + 10]; /* txt data buffer */

        if(0 != deal_with_parameter(argc, argv))
        {
                return -1;
        }

        fd_i = fopen(file_i, "rb");
        if(NULL == fd_i)
        {
                fprintf(stderr, "%s ", file_i);
                DBG(ERR_FOPEN_FAILED, "\n");
                return -ERR_FOPEN_FAILED;
        }

        judge_type();
        while(1 == fread(bbuf, npline, 1, fd_i))
        {
                pkt_init(pkt);

                switch(type)
                {
                        case FILE_TS:
                                pkt->addr = &(pkt->ADDR);
                                pkt->ts = (bbuf + 0);
                                break;
                        case FILE_MTS:
                                pkt->addr = &(pkt->ADDR);
                                pkt->ts = (bbuf + 4);
                                pkt->mts = &(pkt->MTS);
                                mts_time(pkt->mts, bbuf);
                                break;
                        default: /* FILE_TSRS */
                                pkt->addr = &(pkt->ADDR);
                                pkt->ts = (bbuf + 0);
                                pkt->rs = (bbuf + 188);
                                break;
                }
                b2t(tbuf, pkt);
                puts(tbuf);
                pkt->ADDR += npline;
        }

        fclose(fd_i);

        return 0;
}

static int deal_with_parameter(int argc, char *argv[])
{
        int i;

        if(1 == argc)
        {
                /* no parameter */
                fprintf(stderr, "No binary file to process...\n\n");
                show_help();
                return -1;
        }

        for(i = 1; i < argc; i++)
        {
                if('-' == argv[i][0])
                {
                        if(0 == strcmp(argv[i], "-start"))
                        {
                                int start;

                                i++;
                                if(i >= argc)
                                {
                                        fprintf(stderr, "no parameter for '-start'!\n");
                                        exit(EXIT_FAILURE);
                                }
                                sscanf(argv[i], "%i" , &start);
                                aim_start = start;
                        }
                        else if(0 == strcmp(argv[i], "-stop"))
                        {
                                int stop;

                                i++;
                                if(i >= argc)
                                {
                                        fprintf(stderr, "no parameter for '-stop'!\n");
                                        exit(EXIT_FAILURE);
                                }
                                sscanf(argv[i], "%i" , &stop);
                                aim_stop = stop;
                        }
                        else if(0 == strcmp(argv[i], "-h") ||
                                0 == strcmp(argv[i], "--help")
                        )
                        {
                                show_help();
                                return -1;
                        }
                        else if(0 == strcmp(argv[i], "-v") ||
                                0 == strcmp(argv[i], "--version")
                        )
                        {
                                show_version();
                                return -1;
                        }
                        else
                        {
                                fprintf(stderr, "Wrong parameter: %s\n", argv[i]);
                                DBG(ERR_BAD_ARG, "\n");
                                return -ERR_BAD_ARG;
                        }
                }
                else
                {
                        strcpy(file_i, argv[i]);
                }
        }

        return 0;
}

static int show_help()
{
        puts("'catts' read binary file, translate 0xXX to 'XY ' format, then send to stdout.");
        puts("");
        puts("Usage: catts [OPTION] file [OPTION]");
        puts("");
        puts("Options:");
        puts("");
#if 0
        puts(" -start <a>               cat from a%(file length), default: 0, first byte");
        puts(" -stop <b>                cat to b%(file length), default: 0, last byte");
#endif
        puts(" -h, --help               display this information");
        puts(" -v, --version            display my version");
        puts("");
        puts("Examples:");
        puts("  catts xxx.ts");
        puts("");
        puts("Report bugs to <zhoucheng@tsinghua.org.cn>.");
        return 0;
}

static int show_version()
{
        puts("catts 1.0.0");
        puts("");
        puts("Copyright (C) 2009,2010,2011 ZHOU Cheng.");
        puts("This is free software; contact author for additional information.");
        puts("There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR");
        puts("A PARTICULAR PURPOSE.");
        puts("");
        puts("Written by ZHOU Cheng.");
        return 0;
}

#define SYNC_TIME       3 /* SYNC_TIME syncs means TS sync */
#define ASYNC_BYTE      4096 /* head ASYNC_BYTE bytes async means BIN file */
/* for TS data: use "state machine" to determine sync position and packet size */
static int judge_type()
{
        uint8_t dat;
        int sync_cnt;
        int state = FILE_UNKNOWN;

        pkt->ADDR = 0;
        type = FILE_UNKNOWN;
        while(FILE_UNKNOWN == type)
        {
                switch(state)
                {
                        case FILE_UNKNOWN:
                                fseek(fd_i, pkt->ADDR, SEEK_SET);
                                if(1 != fread(&dat, 1, 1, fd_i))
                                {
                                        return -1;
                                }
                                if(0x47 == dat)
                                {
                                        sync_cnt = 1;
                                        state = FILE_TS;
                                }
                                else
                                {
                                        pkt->ADDR++;
                                        if(pkt->ADDR > ASYNC_BYTE)
                                        {
                                                pkt->ADDR = 0;
                                                type = FILE_UNKNOWN;
                                        }
                                }
                                break;
                        case FILE_TS:
                                fseek(fd_i, +187, SEEK_CUR);
                                if(1 != fread(&dat, 1, 1, fd_i))
                                {
                                        return -1;
                                }
                                if(0x47 == dat)
                                {
                                        sync_cnt++;
                                        if(sync_cnt >= SYNC_TIME)
                                        {
                                                npline = 188;
                                                type = FILE_TS;
                                        }
                                }
                                else
                                {
                                        fseek(fd_i, pkt->ADDR + 1, SEEK_SET);
                                        sync_cnt = 1;
                                        state = FILE_MTS;
                                }
                                break;
                        case FILE_MTS:
                                fseek(fd_i, +191, SEEK_CUR);
                                if(1 != fread(&dat, 1, 1, fd_i))
                                {
                                        return -1;
                                }
                                if(0x47 == dat)
                                {
                                        sync_cnt++;
                                        if(sync_cnt >= SYNC_TIME)
                                        {
                                                if(pkt->ADDR < 4)
                                                {
                                                        fseek(fd_i, pkt->ADDR + 1, SEEK_SET);
                                                        sync_cnt = 1;
                                                        state = FILE_TSRS;
                                                }
                                                else
                                                {
                                                        pkt->ADDR -= 4;
                                                        npline = 192;
                                                        type = FILE_MTS;
                                                }
                                        }
                                }
                                else
                                {
                                        fseek(fd_i, pkt->ADDR + 1, SEEK_SET);
                                        sync_cnt = 1;
                                        state = FILE_TSRS;
                                }
                                break;
                        case FILE_TSRS:
                                fseek(fd_i, +203, SEEK_CUR);
                                if(1 != fread(&dat, 1, 1, fd_i))
                                {
                                        return -1;
                                }
                                if(0x47 == dat)
                                {
                                        sync_cnt++;
                                        if(sync_cnt >= SYNC_TIME)
                                        {
                                                npline = 204;
                                                type = FILE_TSRS;
                                        }
                                }
                                else
                                {
                                        pkt->ADDR++;
                                        state = FILE_UNKNOWN;
                                }
                                break;
                        default:
                                DBG(ERR_BAD_CASE, "%d\n", state);
                                return -1;
                }
        }
        fseek(fd_i, pkt->ADDR, SEEK_SET);
        if(pkt->ADDR != 0)
        {
                fprintf(stderr, "%lld-byte passed\n", pkt->ADDR);
        }
        return 0;
}

static int mts_time(uint64_t *mts, uint8_t *bin)
{
        int i;

        for(*mts = 0, i = 0; i < 4; i++)
        {
                *mts <<= 8;
                *mts |= *bin++;
        }

        return 0;
}