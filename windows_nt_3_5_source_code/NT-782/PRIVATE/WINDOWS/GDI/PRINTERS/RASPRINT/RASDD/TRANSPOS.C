/**************************** Function Header ******************************
 * transpos.c
 *      Functions associated with transposing bitmaps.  Several flavours
 *      are available,  to be used as appropriate in some special cases.
 *      The special cases are generally faster than the general method.
 *
 * HISTORY:
 *  10:46 on Wed 27 Feb 1991    -by-    Lindsay Harris   [lindsayh]
 *      Added table creation at load time - big endian/little endian
 *
 *  14:24 on Wed 23 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it - initially for the 8 x 8 case.
 *
 * Copyright (C) 1991 - 1993 Micrososft Corporation
 *
 ***************************************************************************/

#include        <stddef.h>
#include        <windows.h>
#include        <libproto.h>
#include        <winddi.h>
#include        "pdev.h"
#include        "win30def.h"
#include        "udmindrv.h"
#include        "udpfm.h"
#include        "uddevice.h"
#include        "udrender.h"

/*
 *   The transpose table:  maps one byte into two longs,  such that the
 * 8 bits of the byte turn into 64 bits: each bit of the original is
 * turned into one byte of output.
 *   THUS:
 * Input byte:   hgfedcba
 *   transposes into output bytes:
 *      0000000a  0000000b  0000000c  0000000d
 *      0000000e  0000000f  0000000g  0000000h
 *
 *   The table is allocated at DrvEnableSurface time,  thus ensuring that
 *  we do not allocate memory that we are not going to use.
 */

#define TABLE_SIZE      (256 * 2 * sizeof( DWORD ))

/*
 *   We also need a similar table for colour separation.  This one
 *  consists of 256 DWORDs,  and is used to split the RGB(K) format
 *  input byte into an output DWORD with the two R bits in one byte,
 *  the two G bits in the next byte etc.  Used for single pin colour
 *  printers,  like the HP PaintJet.
 *   The table is generated according to the following rule:
 *
 *  INPUT BYTE:  KRGBkrgb
 *
 *  OUTPUT DWORD:  000000Kk 000000Rr 000000Gg 000000Bb
 */

#define SEP_TABLE_SIZE  (256 * sizeof( DWORD ))



/************************** Function Header *******************************
 * vInitTrans
 *      Initialise the transpose tables.  This is done to make the tables
 *      independent of whether the processor is big endian or little endian,
 *      since the data is generated by the processor that is going to
 *      use it!  There are still some minor questions of byte ordering,
 *      but nothing too major to resolve.
 *
 * RETURNS:
 *      TRUE/FALSE;  FALSE for lack of storage for table.
 *
 * HISTORY:
 *  Friday December 3 1993      -by-    Norman Hendley   [normanh]
 *      Changed graphics mode check to RES_DM_GDI from nPins
 *
 *  10:52 on Wed 27 Feb 1991    -by-    Lindsay Harris   [lindsayh]
 *      Borrowed from program which generated original table.
 *
 **************************************************************************/

BOOL
bInitTrans( pPDev )
PDEV  *pPDev;
{

    /*
     *   Function to generate the transposition table.  There is nothing
     * difficult about generating the table.  The only trick is the use
     * of the union.  This allows us to setup a DWORD table with the
     * byte ordering of the hardware on which we are running.  This is
     * achieved by writing the data into the BYTE entry,  then using
     * the same memory as a DWORD to be put away into memory.  The reason
     * for using DWORDS is to get maximum benefit from memory references
     * in the inner loop of the transpose functions.
     *   Storage space is allocated on the heap,  and the address is
     * stored in the PDEV.
     *   Note that the 8 bits per pel case is special, as we are shuffling
     * bytes around, and thus do not need any tables.  For this case,
     * return TRUE without allocating any storage.
     *
     *   Returns TRUE for success,  FALSE meaning storage unavailable.
     */

    register  DWORD   *pdw;
    register  int   iShift,  j;

    int    i;

#define pUDPDev ((UD_PDEV *)(pPDev->pUDPDev))

    union
    {
        BYTE   b[ 8 ];          /* Exactly 64 bits */
        DWORD  dw[ 2 ];         /* Also exactly 64 bits */
    } u;


    if( pUDPDev->sBitsPixel == 8 )
    {
        pPDev->pdwTrans = NULL;

        return   TRUE;              /* Byte operations - no table needed */
    }

    if( !(pPDev->pdwTrans = (DWORD *)HeapAlloc( pPDev->hheap, 0, TABLE_SIZE )) )
        return  FALSE;


    pdw = pPDev->pdwTrans;              /* Speedier access */


    /*
     *   Colour requires different tables,  as the pixel data consists of
     * 4 bits which need to move in a single group.
     */

    if( pUDPDev->Resolution.fDump & RES_DM_COLOR )
    {
        /*
         *   First generate the landscape to portrait transpose data.
         *  The only complication is maintaining 4 bit nibbles as a single
         *  entity.
         */

        for( iShift = 0; iShift < 256; iShift++ )
        {
            /*
             *   The low nibble goes into the highest byte address,  the
             *  next nibble goes 4 bytes lower in memory.
             */
            u.dw[ 0 ] = 0;
            u.dw[ 1 ] = 0;

            u.b[ 3 ] = (BYTE)((iShift >> 4) & 0x0f);
            u.b[ 7 ] = (BYTE)(iShift & 0x0f);

            /*   Store the result  */

            *pdw++ = u.dw[ 0 ];
            *pdw++ = u.dw[ 1 ];
        }

        /*
         *   There is an additional transpose operation required for single
         *  pin colour printers.  The HP Paintjet typifies this class.
         *  This operation is required to separate the RGB pixels (2 of each
         *  colour per byte) into bytes that may be sent to the printer,
         *  such that all R bytes are sent in one go,  followed by G etc.
         *  For multiple pin printers,  this falls out of the standard
         *  transpose operations.
         */



        if( pUDPDev->Resolution.fDump & RES_DM_GDI )
        {
            pPDev->pdwColrSep = (DWORD *)HeapAlloc( pPDev->hheap, 0, SEP_TABLE_SIZE );
            if( pPDev->pdwColrSep == NULL )
            {
                HeapFree( pPDev->hheap, 0, (LPSTR)pPDev->pdwTrans );
                pPDev->pdwTrans = 0;

                return   FALSE;
            }

            pdw = pPDev->pdwColrSep;    /* Speedier access */

            /*
             *   The explanation above for SEP_TABLE_SIZE explains what is
             *  taking place in the following loops.
             */

            for( i = 0; i <= 0xff; i++ )
            {
                u.dw[ 0 ] = 0;
                iShift = i & 0x77;              /* Only use 3 bits per pel */

                if( pUDPDev->fColorFormat & DC_EXTRACT_BLK )
                {
                    if( pUDPDev->fColorFormat & DC_PRIMARY_RGB )
                    {
                    /*
                     *    Whenever we have a 0 nibble,  replace it with 8.
                     *  This does the colour separation for us!  The
                     *  separation happens when the transpose happens.
                     */

                        if( (iShift & 0x07) == 0 )
                            iShift |= 0x08;

                        if( (iShift & 0x70) == 0 )
                            iShift |= 0x80;
                    }
                    else
                    {
                        /*  CMY - same idea,  different conditions!  */
                        if( (iShift & 0x07) == 0x07 )
                            iShift = (iShift & ~0x07) | 0x08;

                        if( (iShift & 0x70) == 0x70 )
                            iShift = (iShift & ~0x70) | 0x80;
                    }
                }

                /*   The two bits Bb  */
                u.b[ 3 ] = (BYTE)(((iShift >> 3) & 0x02) | (iShift & 0x1));
                iShift >>= 1;


                /*   The two bits Gg  */
                u.b[ 2 ] = (BYTE)(((iShift >> 3) & 0x02) | (iShift & 0x1));
                iShift >>= 1;


                /*   The two bits Rr  */
                u.b[ 1 ] = (BYTE)(((iShift >> 3) & 0x02) | (iShift & 0x1));
                iShift >>= 1;


                /*   The two bits Kk  */
                u.b[ 0 ] = (BYTE)(((iShift >> 3) & 0x02) | (iShift & 0x1));

                *pdw++ = u.dw[ 0 ];             /* Safe for posterity */
            }
        }
        else
        {
            /*
             *   The dot matrix case.  Here we will call the relevant
             * transpose function,  but use the modified table below.  This
             * table will do the colour separation,  and will result in the
             * transpose operation splitting up the data for each head pass.
             */

            pPDev->pdwColrSep = (DWORD *)HeapAlloc( pPDev->hheap, 0, TABLE_SIZE );
            if( pPDev->pdwColrSep == NULL )
            {
                HeapFree( pPDev->hheap, 0, (LPSTR)pPDev->pdwTrans );
                pPDev->pdwTrans = 0;

                return   FALSE;
            }

            pdw = pPDev->pdwColrSep;    /* Speedier access */

            for( i = 0; i <= 0xff; i++ )
            {
                /*  Each bit of i goes into one byte of the output  */
                u.dw[ 0 ] = 0;
                u.dw[ 1 ] = 0;

                iShift = i & 0x77;              /* Only 3 bits per pel */

                if( pUDPDev->fColorFormat & DC_EXTRACT_BLK )
                {
                    if( pUDPDev->fColorFormat & DC_PRIMARY_RGB )
                    {
                    /*
                     *    Whenever we have a 0 nibble,  replace it with 8.
                     *  This does the colour separation for us!  The
                     *  separation happens when the transpose happens.
                     */

                        if( (iShift & 0x07) == 0 )
                            iShift |= 0x08;

                        if( (iShift & 0x70) == 0 )
                            iShift |= 0x80;
                    }
                    else
                    {
                        /*  CMY - same idea,  different conditions!  */
                        if( (iShift & 0x07) == 0x07 )
                            iShift = (iShift & ~0x07) | 0x08;

                        if( (iShift & 0x70) == 0x70 )
                            iShift = (iShift & ~0x70) | 0x80;
                    }
                }

                for( j = 8; --j >= 0; )
                {
                    u.b[ j ] = (BYTE)(iShift & 0x1);
                    iShift >>= 1;
                }

                /*   Store the result  */

                *pdw++ = u.dw[ 0 ];
                *pdw++ = u.dw[ 1 ];
            }
        }
    }
    else
    {
        /*
         *   Monochrome case - simple transpositions.
         */

        for( i = 0; i <= 0xff; i++ )
        {
            /*  Each bit of i goes into one byte of the output  */
            iShift = i;
            u.dw[ 0 ] = 0;
            u.dw[ 1 ] = 0;

            for( j = 8; --j >= 0; )
            {
                u.b[ j ] = (BYTE)(iShift & 0x1);
                iShift >>= 1;
            }

            /*   Store the result  */

            *pdw++ = u.dw[ 0 ];
            *pdw++ = u.dw[ 1 ];
        }
    }

    return  TRUE;
}
#undef  pUDPDev


/************************** Function Header *******************************
 * vTrans8x8
 *      Function to transpose the input array into the output array,
 *      where the input data is to be considered 8 rows of bitmap data,
 *      and the output area is dword aligned.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  14:27 on Wed 23 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 *************************************************************************/

void
vTrans8x8( pbIn,  pRData )
BYTE    *pbIn;          /* Source */
RENDER  *pRData;        /* Rendering info */
{
    /*
     *    The technique is quite simple,  though not necessarily obvious.
     *  Take an 8 scan line by 8 bits block of data,  and transform it
     *  into 8 bytes with bits in the scan line order,  rather than
     *  along the scan line as supplied.
     *    To do this as quickly as possible, each byte to be converted
     *  is used as an index into a lookup table;  each table entry is
     *  64 bits long (a pair of longs above).  These 64 bits are ORed
     *  with the running total of 64 bits (the two variables, dw0, dw1);
     *  shift the running total one bit left.  Repeat this operation
     *  for the corresponding byte in the next scan line - this is
     *  the new table lookup index.  Repeat for all 8 bytes in the 8
     *  scan lines being processed.  Store the 64 bit temporary results
     *  in the output dword array.  Move to the next byte in the
     *  scan line,  and repeat the loop for this column.
     */

    register  DWORD  dw0,  dw1;         /* Inner loop temporaries */
    register  BYTE  *pbTemp;
    register  DWORD *pdw;

    register  int    cbLine;            /* Bytes per line in scan data */
    register  int    i;                 /* Loop variable. */


    int      iWide;                     /* Pixels across the bitmap */
    DWORD   *pdwOut;                    /* Destination */
    DWORD   *pdwTrans;                  /* Local copy of output buffer */


    /*
     *   Some initialisation:  byte count,  area limits, etc.
     */


    cbLine = pRData->cbTLine;
    pdwOut = pRData->pvTransBuf;
    pdwTrans = pRData->Trans.pdwTransTab;

    if( pRData->iTransHigh != 8 )
    {
        /*  This can happen at the end of a page. */

        vTrans8N( pbIn,  pRData );

        return;
    }


    /*
     *   Scan across the lines in groups of 8 bits.  In the case that the
     *  input is not a multiple of 8,  we will produce a few extra
     *  bytes at the end;  the caller should allow for this when allocating
     *  storage for pdwOut.  The consequence is that the last few
     *  bytes will contain garbage; presumably the caller will not
     *  process them further.
     */

    for( iWide = pRData->iTransWide; iWide > 0; iWide -= BBITS )
    {
        dw0 = 0;
        dw1 = 0;

        /*
         *   Loop DOWN the scanlines at the starting byte location,
         * generating the transposed data as we go.
         */

        for( i = BBITS, pbTemp = pbIn++; --i >= 0; pbTemp += cbLine )
        {
            dw0 <<= 1;
            dw1 <<= 1;
            pdw = pdwTrans + (*pbTemp << 1);
            dw0 |= *pdw;
            dw1 |= *(pdw + 1);
        }

        /*   Store the two temporary values in the output buffer. */
        *pdwOut = dw0;
        *(pdwOut + 1) = dw1;
        pdwOut += 2;

    }

    return;
}


/************************** Function Header *******************************
 * vTrans8N
 *      Function to transpose the input array into the output array,
 *      where the input data is to be considered N rows of bitmap data,
 *      and the output area is byte aligned.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  16:34 on Mon 28 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 *************************************************************************/

void
vTrans8N( pbIn,  pRData )
BYTE    *pbIn;          /* Source */
RENDER  *pRData;        /* Overall rendering info */
{
    /*
     *    The technique is quite simple,  though not necessarily obvious.
     *  Take an 8 scan line by 8 bits block of data,  and transform it
     *  into 8 bytes with bits in the scan line order,  rather than
     *  along the scan line as supplied.
     *    To do this as quickly as possible, each byte to be converted
     *  is used as an index into a lookup table;  each table entry is
     *  64 bits long (a pair of longs above).  These 64 bits are ORed
     *  with the running total of 64 bits (the two variables, dw0, dw1);
     *  shift the running total one bit left.  Repeat this operation
     *  for the corresponding byte in the next scan line - this is
     *  the new table lookup index.  Repeat for all 8 bytes in the 8
     *  scan lines being processed.  Store the 64 bit temporary results
     *  in the output dword array.  Move to the next byte in the
     *  scan line,  and repeat the loop for this column.
     *    This function is based on the special 8 X 8 case (vTrans8x8).
     *  The significant differences are that the transposed data needs
     *  to be written byte at a time (instead of DWORD at a time),
     *  and that there are N scan lines to convert in each loop.
     */

    register  DWORD  dw0,  dw1;         /* Inner loop temporaries */
    register  BYTE  *pbTemp;
    register  DWORD *pdw;

    register  int    cbLine;            /* Bytes per line in scan data */
    register  int    i;                 /* Loop variable. */
    register  int    iBand;             /* For moving down the scan lines */

    int      iSkip;                     /* Output interleave factor */
    int      iWide;                     /* Pixels across the bitmap */

    BYTE    *pbOut;                     /* Destination, local copy */
    BYTE    *pbBase;                    /* Start addr of 8 scan line group */
    BYTE    *pbOutTmp;                  /* For output loop */

    DWORD   *pdwTrans;                  /* Speedier access */



    /*
     *   Set up the local variables from the RENDER structure passed in.
     */

    cbLine = pRData->cbTLine;
    iSkip = pRData->iTransSkip;
    pbOut = pRData->pvTransBuf;                 /* Reserved for us! */
    pdwTrans = pRData->Trans.pdwTransTab;

    /*
     *     To ease MMU thrashing,  we scan ACROSS the bitmap in 8 line
     *  groups.  This results in closer memory references,  and so less
     *  page faults and so faster execution.  Hence,  the outer most loop
     *  loops DOWN the scanlines.  The next inner loop scans across groups
     *  of 8 scan lines at a time,  while the inner most loop transposes
     *  one byte by 8 scan lines of bitmap image.
     *     Note that processing the data this way causes a slight increase
     *  in scattered memory addresses when writing the output data.
     *  There is no way to avoid one or the other memory references being
     *  scattered;  however,  the output area is smaller than the input
     *  input,  so scattering here will be less severe to the MMU.
     */

    for( iBand = pRData->iTransHigh; iBand >= BBITS; iBand -= BBITS )
    {

        /*
         *    Have selected the next group of 8 scan lines to process,
         *  so scan from left to right,  transposing data in 8 x 8 bit
         *  groups.  This is the size that can be done very quickly with
         *  a 32 bit environment.
         */

        pbBase = pbIn;
        pbIn += BBITS * cbLine;         /* Next address */

        pbOutTmp = pbOut;
        ++pbOut;                /* Onto the next byte sequence */

        for( iWide = pRData->iTransWide; iWide > 0; iWide -= BBITS )
        {
            /*
             *    Process the bitmap byte at a time moving across, and
             *  8 scan lines high.  This corresponds to transposing an
             *  8 x 8 bit array.  We can do that quickly.
             */
            dw0 = 0;
            dw1 = 0;
            pbTemp = pbBase++;

            for( i = BBITS; --i >= 0; pbTemp += cbLine )
            {
                /*   The INNER loop - the bit swapping operations */
                dw0 <<= 1;
                dw1 <<= 1;
                pdw = pdwTrans + (*pbTemp << 1);
                dw0 |= *pdw;
                dw1 |= *(pdw + 1);

            }

/*   !!!LindsayH:   Note that the following code is big endian/little endian
 *   sensitive,  and currently works on the 80386 (which ever way that is).
 *   There are two alternatives to cure this problem:  first is to have
 *   another function,  with the order of byte extraction reversed; second
 *   is to offset the value in pbTemp,  and change the sign of iSkip.
 *   There are disadvantages to both.
 *  FOR NOW,  this is not a problem,  and will be left as an exercise
 *  for the student.
 */
            /*   Store the two temporary values in the output buffer. */
            pbTemp = pbOutTmp;
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            dw0 >>= BBITS;              /* One byte's worth */
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            dw0 >>= BBITS;
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            dw0 >>= BBITS;
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            *pbTemp = (BYTE)dw1;

            pbTemp += iSkip;
            dw1 >>= BBITS;
            *pbTemp = (BYTE)dw1;

            pbTemp += iSkip;
            dw1 >>= BBITS;
            *pbTemp = (BYTE)dw1;

            pbTemp += iSkip;
            dw1 >>= BBITS;
            *pbTemp = (BYTE)dw1;

            pbOutTmp += BBITS * iSkip;  /* Next chunk of output data */
        }

    }

    /*
     *    There may be some scan lines remaining.  If so,  iBand will
     *  be > 0,  and that indicates the number of output scan lines
     *  remaining.
     */

    if( iBand > 0 )
    {

        /*
         *   This is basically the same as the stripped down version
         *  in the outer loop above.  Note that the output data is still
         *  byte aligned,  IT IS PRESUMED THAT THE 'MISSING' LINES ARE
         *  ZERO FILLED.  This may not be what is desired - it is for
         *  transposing bits to output to a dot matrix printer where
         *  the page length is not a multiple of the number of pins.
         *  I don't know if that can ever happen.
         */

        pbBase = pbIn;
        pbOutTmp = pbOut;

        for( iWide = pRData->iTransWide; iWide > 0; iWide -= BBITS )
        {
            /*
             *    Process the bitmap byte at a time moving across, and
             *  8 scan lines high.  This corresponds to transposing an
             *  8 x 8 bit array.  We can do that quickly.
             */
            dw0 = 0;
            dw1 = 0;
            pbTemp = pbBase++;

            /*
             *    The inner loop now only transposes as many scan lines
             *  as the bitmap actually contains - we must not run off
             *  the end of memory.
             */

            for( i = iBand; --i >= 0; pbTemp += cbLine )
            {
                /*   The INNER loop - the bit swapping operations */
                dw0 <<= 1;
                dw1 <<= 1;
                pdw = pdwTrans + (*pbTemp << 1);
                dw0 |= *pdw;
                dw1 |= *(pdw + 1);

            }

            /*   Zero fill the missing bits  */
            dw0 <<= BBITS - iBand;
            dw1 <<= BBITS - iBand;

            /*   Store the two temporary values in the output buffer. */
            pbTemp = pbOutTmp;
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            dw0 >>= BBITS;              /* One byte's worth */
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            dw0 >>= BBITS;
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            dw0 >>= BBITS;
            *pbTemp = (BYTE)dw0;

            pbTemp += iSkip;
            *pbTemp = (BYTE)dw1;

            pbTemp += iSkip;
            dw1 >>= BBITS;
            *pbTemp = (BYTE)dw1;

            pbTemp += iSkip;
            dw1 >>= BBITS;
            *pbTemp = (BYTE)dw1;

            pbTemp += iSkip;
            dw1 >>= BBITS;
            *pbTemp = (BYTE)dw1;

            pbOutTmp += BBITS * iSkip;  /* Next chunk of output data */
        }

    }

    return;
}


/*
 *   Define the number of pels transposed per loop iteration.  In the case
 * of a colour bitmap, this is 2,  since there are 4 bits per pel, thus
 * 2 per byte.
 */

#define PELS_PER_LOOP   (BBITS / 4)


/************************** Function Header *******************************
 * vTrans8N4BPP
 *      Function to transpose the input array into the output array,
 *      where the input data is to be considered N rows of bitmap data,
 *      and the output area is byte aligned.
 *      This version works on 4 bits per pel bitmaps (colour for us).
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  15:20 on Tue 30 Jul 1991    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation, based on vTrans8N.
 *
 *************************************************************************/

void
vTrans8N4BPP( pbIn,  pRData )
BYTE    *pbIn;          /* Source */
RENDER  *pRData;        /* Overall rendering info */
{
    /*
     *    The technique is quite simple,  though not necessarily obvious.
     *  Take an 8 scan line by 8 bits block of data,  and transform it
     *  into 8 bytes with bits in the scan line order,  rather than
     *  along the scan line as supplied.
     *    To do this as quickly as possible, each byte to be converted
     *  is used as an index into a lookup table;  each table entry is
     *  64 bits long (a pair of longs above).  These 64 bits are ORed
     *  with the running total of 64 bits (the two variables, dw0, dw1);
     *  shift the running total one bit left.  Repeat this operation
     *  for the corresponding byte in the next scan line - this is
     *  the new table lookup index.  Repeat for all 8 bytes in the 8
     *  scan lines being processed.  Store the 64 bit temporary results
     *  in the output dword array.  Move to the next byte in the
     *  scan line,  and repeat the loop for this column.
     *    This function is based on the special 8 X 8 case (vTrans8x8).
     *  The significant differences are that the transposed data needs
     *  to be written byte at a time (instead of DWORD at a time),
     *  and that there are N scan lines to convert in each loop.
     */

    register  DWORD  dw0,  dw1;         /* Inner loop temporaries */
    register  BYTE  *pbTemp;
    register  DWORD *pdw;

    register  int    cbLine;            /* Bytes per line in scan data */
    register  int    i;                 /* Loop variable. */
    register  int    iBand;             /* For moving down the scan lines */

    int      iSkip;                     /* Output interleave factor */
    int      iWide;                     /* Pixels across the bitmap */

    DWORD   *pdwOut;                    /* Destination, local copy */
    BYTE    *pbBase;                    /* Start addr of 8 scan line group */
    DWORD   *pdwOutTmp;                 /* For output loop */

    DWORD   *pdwTrans;                  /* Speedier access */



    /*
     *   Set up the local variables from the RENDER structure passed in.
     *  See the above function for explanation of iSkip.
     */

    cbLine = pRData->cbTLine;
    iSkip = pRData->iTransSkip / DWBYTES;
    pdwOut = pRData->pvTransBuf;                        /* Reserved for us! */
    pdwTrans = pRData->Trans.pdwTransTab;

    /*
     *     To ease MMU thrashing,  we scan ACROSS the bitmap in 8 line
     *  groups.  This results in closer memory references,  and so less
     *  page faults and faster execution.  Hence,  the outer most loop
     *  loops DOWN the scanlines.  Then next inner loop scans across groups
     *  of 8 scan lines at a time,  while the inner most loop transposes
     *  one byte by 8 scan lines of bitmap image.
     *     Note that processing the data this way causes a slight increase
     *  in scattered memory addresses when writing the output data.
     *  There is no way to avoid one or the other memory references being
     *  scattered;  however,  the output area is smaller than the input
     *  input,  so scattering here will be less severe on the MMU.
     */


    for( iBand = pRData->iTransHigh; iBand >= BBITS; iBand -= BBITS )
    {

        /*
         *    Have selected the next group of 8 scan lines to process,
         *  so scan from left to right,  transposing data in 8 x 8 bit
         *  groups.  This is the size that can be done very quickly with
         *  a 32 bit environment.
         */

        pbBase = pbIn;
        pbIn += BBITS * cbLine;         /* Next address */

        pdwOutTmp = pdwOut;
        ++pdwOut;               /* Onto the next byte sequence */

        for( iWide = pRData->iTransWide; iWide > 0; iWide -= BBITS )
        {
            /*
             *    Process the bitmap byte at a time moving across, and
             *  8 scan lines high.  This corresponds to transposing an
             *  8 x 8 pixel array.  We can do that quickly.
             */
            dw0 = 0;
            dw1 = 0;
            pbTemp = pbBase++;

            for( i = BBITS; --i >= 0; pbTemp += cbLine )
            {
                /*   The INNER loop - the bit swapping operations */
                dw0 >>= 8;
                dw1 >>= 8;
                pdw = pdwTrans + (*pbTemp << 1);
                dw0 |= *pdw << 4;
                dw1 |= *(pdw + 1) << 4;

                pbTemp += cbLine;
                --i;

                pdw = pdwTrans + (*pbTemp << 1);
                dw0 |= *pdw;
                dw1 |= *(pdw + 1);

            }

            /*   Store the two temporary values in the output buffer. */

            *pdwOutTmp = dw0;
            *(pdwOutTmp + iSkip) = dw1;

            pdwOutTmp += PELS_PER_LOOP * iSkip; /* Next chunk of output data */
        }

    }

    /*
     *    There may be some scan lines remaining.  If so,  iBand will
     *  be > 0,  and that indicates the number of output scan lines
     *  remaining.
     */

    if( iBand > 0 )
    {

        /*
         *   This is basically the same as the stripped down version
         *  in the outer loop above.  Note that the output data is still
         *  byte aligned,  IT IS PRESUMED THAT THE 'MISSING' LINES ARE
         *  ZERO FILLED.  This may not be what is desired - it is for
         *  transposing bits to output to a dot matrix printer where
         *  the page length is not a multiple of the number of pins.
         *  I don't know if that can ever happen.
         */

        pbBase = pbIn;
        pdwOutTmp = pdwOut;

        for( iWide = pRData->iTransWide; iWide > 0; iWide -= BBITS )
        {
            /*
             *    Process the bitmap byte at a time moving across, and
             *  8 scan lines high.  This corresponds to transposing an
             *  8 x 8 bit array.  We can do that quickly.
             */
            dw0 = 0;
            dw1 = 0;
            pbTemp = pbBase++;

            /*
             *    The inner loop now only transposes as many scan lines
             *  as the bitmap actually contains - we must not run off
             *  the end of memory.
             */

            for( i = iBand; --i >= 0; pbTemp += cbLine )
            {
                /*   The INNER loop - the bit swapping operations. */

                pdw = pdwTrans + (*pbTemp << 1);

                if( (i ^ iBand) & 0x1 )
                {
                    /*  Every even time through the loop */
                    dw0 >>= 8;
                    dw1 >>= 8;

                    dw0 |= *pdw << 4;
                    dw1 |= *(pdw + 1) << 4;
                }
                else
                {
                    /*  Odd times through the loop */
                    dw0 |= *pdw;
                    dw1 |= *(pdw + 1);
                }

            }

            /*   Zero fill the missing bits  */
            dw0 >>= 8 * ((BBITS - iBand) / 2);
            dw1 >>= 8 * ((BBITS - iBand) / 2);

            /*   Store the two temporary values in the output buffer. */

            *pdwOutTmp = dw0;
            *(pdwOutTmp + iSkip) = dw1;

            pdwOutTmp += 2 * iSkip;     /* Next chunk of output data */
        }

    }

    return;
}


/***************************** Function Header ******************************
 * vTransColSep()
 *      Function to transpose the colour bits in a 4 Bits Per Pel colour
 *      bitmap into an array of bytes,  where the bytes are ordered in
 *      the same way as the original bits.  An example of this is provided
 *      in the explanation for the SEP_TABLE_SIZE value at the top of this file.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  13:48 on Mon 10 Jun 1991    -by-    Lindsay Harris   [lindsayh]
 *      Time ZERO
 *
 ***************************************************************************/

void
vTransColSep( pbIn, pRData )
register  BYTE    *pbIn;        /* Source */
RENDER  *pRData;                /* Overall rendering info */
{
    /*
     *    Operation is quite simple - pass along the input array byte
     *  at a time,  and use each 4 byte group to generate a DWORD of
     *  output - placed in pdwOut.  The previously generated translation
     *  table is especially formulated to do this job!
     *
     *    NOTE:  pdwOut and pbIn MAY POINT TO THE SAME ADDRESS!  THERE IS
     *  NO OVERLAP IN OPERATIONS TO CAUSE CONFUSION.
     */

    register  DWORD   dwTemp;
    register  DWORD  *pdwSep;

    int      iI;
    int      iBlock;
    DWORD   *pdwOut;            /* Destination - DWORD aligned */


    iBlock = pRData->cDWLine * pRData->iNumScans;

    pdwSep = pRData->pdwColrSep;                /* Colour separation table */
    pdwOut = pRData->pvTransBuf;                /* Where the data goes */


    /*   Loop through the line in 4 byte groups */
    for( iI = iBlock; --iI >= 0; )
    {

        dwTemp = *(pdwSep + *pbIn++);

        dwTemp <<= 2;
        dwTemp |= *(pdwSep + *pbIn++);

        dwTemp <<= 2;
        dwTemp |= *(pdwSep + *pbIn++);

        *pdwOut++ = (dwTemp << 2) | *(pdwSep + *pbIn++);
    }

    return;

}


/************************** Function Header ********************************
 * vTrans8BPP
 *      The transpose function for 8 bits per pel bitmaps.  This is rather
 *      easy, as all we do is shuffle bytes!
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  17:32 on Tue 03 Nov 1992    -by-    Lindsay Harris   [lindsayh]
 *      Initial version.
 *
 ****************************************************************************/

void
vTrans8BPP( pbIn,  pRData )
BYTE    *pbIn;          /* Source */
RENDER  *pRData;        /* Overall rendering info */
{


    /*
     *    Scan along the input bitmap,  writing the data to the output
     *  in column order.  This results in reduced MMU thrashing, as
     *  the output addresses are all limited to a much smaller range
     *  than the incoming addresses.
     */

    register  BYTE   *pbBase;             /* Scan along input bitmap */
    register  BYTE   *pbOut;              /* The output scan column pointer */

    int     iBand;                 /* Count down scan lines */
    int     iSkip;                 /* Offset between output bytes */
    int     iWide;                 /* Loop across the input scan line */
    int     cbLine;                /* Bytes per input scan line */

    BYTE   *pbOutBase;             /* Start of column of output data */


    /*
     *   Set up the local copies (for faster access) of data passed in.
     */

    cbLine = pRData->cbTLine;
    iSkip = pRData->iTransSkip;
    pbOutBase = pRData->pvTransBuf;       /* Base output buffer address */


    for( iBand = pRData->iTransHigh; iBand > 0; --iBand )
    {
        /*
         *    This loop processes scan lines in the input bitmap. As
         *  we progress across the scan line, the output data is written
         *  in column order.
         */

        pbBase = pbIn;
        pbIn += cbLine;            /* Next scan line, DWORD aligned */

        pbOut = pbOutBase;
        ++pbOutBase;               /* One column across output area */

        for( iWide = pRData->iTransWide; iWide > 0; iWide -= BBITS )
        {
            /*
             *   This loop traverses the input scan line, taking bytes
             *  and writing them to the output area in column order.
             */

            *pbOut = *pbBase++;
            pbOut += iSkip;
        }
    }

    return;
}
