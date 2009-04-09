/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mike@dragonflylogic.com
*/

/* $Id: dmtxreedsol.c 667 2009-02-10 19:48:01Z mblaughton $ */

/**
 * @file dmtxreedsol.c
 * @brief Reed Solomon error correction
 */

#ifndef GfSum
#define GfSum(a,b) (a ^ b)
#endif

/**
 * @brief  XXX
 * @param  message
 * @param  sizeIdx
 * @return void
 */
static void
GenReedSolEcc(DmtxMessage *message, int sizeIdx)
{
   int i, j, val;
   int step, block;
   int blockErrorWords;
   int symbolDataWords;
   int blockDataWords;
   int symbolErrorWords;
   int symbolTotalWords;
   unsigned char g[69], b[68], *bPtr;
   unsigned char *codewords = message->code;

   symbolDataWords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);
   symbolErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, sizeIdx);
   symbolTotalWords = symbolDataWords + symbolErrorWords;
   blockErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribBlockErrorWords, sizeIdx);
   step = dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, sizeIdx);

   assert(blockErrorWords == symbolErrorWords / step);

   memset(g, 0x01, sizeof(g));

   /* Generate ECC polynomial */
   for(i = 1; i <= blockErrorWords; i++) {
      for(j = i - 1; j >= 0; j--) {
         g[j] = GfDoublify(g[j], i);     /* g[j] *= 2**i */
         if(j > 0)
            g[j] = GfSum(g[j], g[j-1]);  /* g[j] += g[j-1] */
      }
   }

   /* Populate error codeword array */
   for(block = 0; block < step; block++) {

      memset(b, 0x00, sizeof(b));
      for(i = block; i < symbolDataWords; i += step) {
         val = GfSum(b[blockErrorWords-1], codewords[i]);
         for(j = blockErrorWords - 1; j > 0; j--) {
            b[j] = GfSum(b[j-1], GfProduct(g[j], val));
         }
         b[0] = GfProduct(g[0], val);
      }

      blockDataWords = dmtxGetBlockDataSize(sizeIdx, block);
      bPtr = b + blockErrorWords;

      for(i = block + (step * blockDataWords); i < symbolTotalWords; i += step)
         codewords[i] = *(--bPtr);

      assert(b == bPtr);
   }
}

/**
 * @brief  XXX
 * @param  code
 * @param  sizeIdx
 * @param  fix
 * @return DmtxPass | DmtxFail
 */
static int
DecodeCheckErrors(unsigned char *code, int sizeIdx, int fix)
{
   int i, j;
   int interleavedBlocks;
   int blockErrorWords;
   int blockTotalWords;
   int blockMaxCorrectable;
   struct rs *rs;
   int fixedErr, fixedErrSum;
   unsigned char data[255];

   interleavedBlocks = dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, sizeIdx);
   blockErrorWords = dmtxGetSymbolAttribute(DmtxSymAttribBlockErrorWords, sizeIdx);
   blockMaxCorrectable = dmtxGetSymbolAttribute(DmtxSymAttribBlockMaxCorrectable, sizeIdx);

   fixedErrSum = 0;
   for(i = 0; i < interleavedBlocks; i++) {

      blockTotalWords = blockErrorWords + dmtxGetBlockDataSize(sizeIdx, i);

      rs = init_rs_char(blockErrorWords, 255 - blockTotalWords);
      if(rs == NULL)
         return DmtxFail;

      for(j = 0; j < blockTotalWords; j++)
         data[j] = code[j*interleavedBlocks+i];

      fixedErr = decode_rs_char(rs, data, NULL, 0, fix);

      if(fixedErr < 0 || fixedErr > blockMaxCorrectable) {
         free_rs_char(&rs);
         return DmtxFail;
      }

      fixedErrSum += fixedErr;

      for(j = 0; j < blockTotalWords; j++)
         code[j*interleavedBlocks+i] = data[j];

      free_rs_char(&rs);
   }

   if(fix != DmtxUndefined && fix >= 0 && fix < fixedErrSum)
      return DmtxFail;

   return DmtxPass;
}

/**
 * @brief  Galois Field Arithmetic: a times b
 * @param  a
 * @param  b
 * @return Galois Field Product
 */
static int
GfProduct(int a, int b)
{
   if(a == 0 || b == 0)
      return 0;
   else
      return aLogVal[(logVal[a] + logVal[b]) % 255];
}

/**
 * @brief  XXX
 * @param  a
 * @param  b
 * @return Result
 */
static int
GfDoublify(int a, int b)
{
   if(a == 0) /* XXX this is right, right? */
      return 0;
   else if(b == 0)
      return a; /* XXX this is right, right? */
   else
      return aLogVal[(logVal[a] + b) % 255];
}
