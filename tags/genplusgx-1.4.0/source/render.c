/***************************************************************************************
 *  Genesis Plus
 *  Video Display Processor (Rendering)
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Eke-Eke (2007,2008,2009), additional code & fixes for the GCN/Wii port
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************************/

#include "shared.h"
#include "md_ntsc.h"
#include "sms_ntsc.h"

/*** NTSC Filters ***/
extern md_ntsc_t *md_ntsc;
extern sms_ntsc_t *sms_ntsc;

/* Look-up pixel table information */
#define LUT_MAX     (5)
#define LUT_SIZE    (0x10000)

/* Clip structure */
typedef struct
{
  uint8 left;
  uint8 right;
  uint8 enable;
}clip_t;

#undef ALIGN_LONG
#ifdef ALIGN_LONG
/* Or change the names if you depend on these from elsewhere.. */
#undef READ_LONG
#undef WRITE_LONG

static __inline__ uint32 READ_LONG(void *address)
{
  if ((uint32)address & 3)
  {
#ifdef LSB_FIRST  /* little endian version */
    return ( *((uint8 *)address) +
        (*((uint8 *)address+1) << 8)  +
        (*((uint8 *)address+2) << 16) +
        (*((uint8 *)address+3) << 24) );
#else       /* big endian version */
    return ( *((uint8 *)address+3) +
        (*((uint8 *)address+2) << 8)  +
        (*((uint8 *)address+1) << 16) +
        (*((uint8 *)address)   << 24) );
#endif  /* LSB_FIRST */
  }
  else return *(uint32 *)address;
}

static __inline__ void WRITE_LONG(void *address, uint32 data)
{
  if ((uint32)address & 3)
  {
#ifdef LSB_FIRST
      *((uint8 *)address) =  data;
      *((uint8 *)address+1) = (data >> 8);
      *((uint8 *)address+2) = (data >> 16);
      *((uint8 *)address+3) = (data >> 24);
#else
      *((uint8 *)address+3) =  data;
      *((uint8 *)address+2) = (data >> 8);
      *((uint8 *)address+1) = (data >> 16);
      *((uint8 *)address)   = (data >> 24);
#endif /* LSB_FIRST */
    return;
  }
  else *(uint32 *)address = data;
}

#endif  /* ALIGN_LONG */


/*   
   Two Pattern Attributes are written in VRAM as two 16bits WORD:

   P = priority bit
   C = color palette (2 bits)
   V = Vertical Flip bit
   H = Horizontal Flip bit
   N = Pattern Number (11 bits)

   MSB PCCVHNNN NNNNNNNN LSB  PCCVHNNN NNNNNNNN LSB
      PATTERN1        PATTERN2

   Pattern attributes are read from VRAM as 32bits WORD like this:

   LIT_ENDIAN: ATTR is  MSB PCCVHNNN NNNNNNNN PCCVHNNN NNNNNNNN LSB
                PATTERN2       PATTERN1

   BIG_ENDIAN: ATTR is  MSB PCCVHNNN NNNNNNNN PCCVHNNN NNNNNNNN LSB
                PATTERN1       PATTERN2


   Each Line Buffer written byte describe one pixel data like this:
   
    msb SPppcccc lsb
  
    with:
      S = sprite data indicator (not written here)
      P = priority bit  (from Pattern Attribute)
      p = color palette (from Pattern Attribute)
      c = color data (from Pattern Cache)
    

   A column is 2 patterns wide
   A pattern is 8 pixels wide = 8 bytes = two 32 bits write
*/

/* Draw a single 8-pixel column */
/*
   pattern cache is addressed like this: 00000VHN NNNNNNNN NNYYYXXX
  with :  Y = pattern row (1-8 lines)
      X = pattern column (1-8 pixels)
      V = Vertical Flip bit
      H = Horizontal Flip bit
      N = Pattern Number (1-2048)
*/
#ifdef ALIGN_LONG
#ifdef LSB_FIRST
#define DRAW_COLUMN(ATTR, LINE) \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF) << 6 | (LINE)]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  ATTR >>= 16; \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF) << 6 | (LINE)]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++;
#else
#define DRAW_COLUMN(ATTR, LINE) \
  atex = atex_table[(ATTR >> 29) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF0000) >> 10 | (LINE)]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF) << 6 | (LINE)]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++;
#endif
#else /* NOT ALIGNED */
#ifdef LSB_FIRST
#define DRAW_COLUMN(ATTR, LINE) \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF) << 6 | (LINE)]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex); \
  ATTR >>= 16; \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF) << 6 | (LINE)]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex);
#else
#define DRAW_COLUMN(ATTR, LINE) \
  atex = atex_table[(ATTR >> 29) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF0000) >> 10 | (LINE)]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex); \
  atex = atex_table[(ATTR >> 13) & 7]; \
  src = (uint32 *)&bg_pattern_cache[(ATTR & 0x1FFF) << 6 | (LINE)]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex);
#endif
#endif /* ALIGN_LONG */


/* Draw a single 16-pixel column */
/*
   pattern cache is addressed like this: 00000VHN NNNNNNNN NYYYYXXX
  with :  Y = pattern row (1-16 lines)
      X = pattern column (1-8 pixels)
         V = Vertical Flip bit
      H = Horizontal Flip bit
      N = Pattern Number (1-1024)

   one pattern line is 8 pixels = 8 bytes = 2 * 32 bits 
*/
#ifdef ALIGN_LONG
#ifdef LSB_FIRST 
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  atex = atex_table[(ATTR >> 13) & 7]; \
  offs = (ATTR & 0x03FF) << 7 | (ATTR & 0x1800) << 6 | (LINE); \
  if(ATTR & 0x1000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  ATTR >>= 16; \
  atex = atex_table[(ATTR >> 13) & 7]; \
  offs = (ATTR & 0x03FF) << 7 | (ATTR & 0x1800) << 6 | (LINE); \
  if(ATTR & 0x1000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; 
#else
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  atex = atex_table[(ATTR >> 29) & 7]; \
  offs = (ATTR & 0x03FF0000) >> 9 | (ATTR & 0x18000000) >> 10 | (LINE); \
  if(ATTR & 0x10000000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  atex = atex_table[(ATTR >> 13) & 7]; \
  offs = (ATTR & 0x03FF) << 7 | (ATTR & 0x1800) << 6 | (LINE); \
  if(ATTR & 0x1000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; \
  WRITE_LONG(dst, READ_LONG(src) | atex); \
  dst++; \
  src++; 
#endif
#else /* NOT ALIGNED */
#ifdef LSB_FIRST 
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  atex = atex_table[(ATTR >> 13) & 7]; \
  offs = (ATTR & 0x03FF) << 7 | (ATTR & 0x1800) << 6 | (LINE); \
  if(ATTR & 0x1000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex); \
  ATTR >>= 16; \
  atex = atex_table[(ATTR >> 13) & 7]; \
  offs = (ATTR & 0x03FF) << 7 | (ATTR & 0x1800) << 6 | (LINE); \
  if(ATTR & 0x1000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex);
#else
#define DRAW_COLUMN_IM2(ATTR, LINE) \
  atex = atex_table[(ATTR >> 29) & 7]; \
  offs = (ATTR & 0x03FF0000) >> 9 | (ATTR & 0x18000000) >> 10 | (LINE); \
  if(ATTR & 0x10000000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex); \
  atex = atex_table[(ATTR >> 13) & 7]; \
  offs = (ATTR & 0x03FF) << 7 | (ATTR & 0x1800) << 6 | (LINE); \
  if(ATTR & 0x1000) offs ^= 0x40; \
  src = (uint32 *)&bg_pattern_cache[offs]; \
  *dst++ = (*src++ | atex); \
  *dst++ = (*src++ | atex);
#endif
#endif /* ALIGN_LONG */

/*
  gcc complains about this:
    *lb++ = table[(*lb << 8) |(*src++ | palette)]; 
  .. claiming the result on lb is undefined.
  So we manually advance lb and use constant offsets into the line buffer.

  Added sprite collision detection
  (check if non-transparent sprite data has been previously drawn)
*/
#define DRAW_SPRITE_TILE \
  for(i=0; i<8; i++) \
  { \
    if (((lb[i] & 0x8F) > 0x80) && src[i]) status |= 0x20; \
    lb[i] = table[(lb[i] << 8) |(src[i] | palette)]; \
  }


/************************************************/
/* Pixel creation macros                        */
/* each R,G,B color channel is 4-bits with a    */
/* total of 15 different intensity levels.      */
/*                                              */
/* Color intensity depends on the S/TE mode:    */
/*                                              */
/*    normal   : xxx0     (0-14)                */
/*    shadow   : 0xxx     (0-7)                 */
/*    highlight: 1xxx - 1 (7-14)                */
/*                                              */
/* with xxx0 = original 4-bits CRAM value       */
/*                                              */
/************************************************/
#ifndef NGC

/* 8:8:8 RGB */
#define MAKE_PIXEL_32(r,g,b) ((r) << 20 | (g) << 12 | (b) << 4)
/* 5:5:5 RGB */
#define MAKE_PIXEL_15(r,g,b) ((r) << 11 | (g) << 6 | (b) << 1)
/* 3:3:2 RGB */
#define MAKE_PIXEL_8(r,g,b)  ((r) <<  5 | (g) << 2 | (b) >> 1)

#endif

/* 5:6:5 RGB */
/* 4-bit color intensity is dithered to match 5-bit or 6-bit equivalent */
#define MAKE_PIXEL_16(r,g,b) ((r) << 12 | ((r) >> 3) << 11 | (g) << 7 | ((g) >> 2) << 5 | (b) << 1 | (b) >> 3)

/* Clip data */
static clip_t clip[2];

/* Attribute expansion table */
static const uint32 atex_table[] = {
  0x00000000, 0x10101010, 0x20202020, 0x30303030,
  0x40404040, 0x50505050, 0x60606060, 0x70707070
};

/* Sprite name look-up table */
static uint8 name_lut[0x400];

typedef struct
{
  uint16 ypos;
  uint16 xpos;
  uint16 attr;
  uint8 size;
  uint8 index; // unused
} object;

static object object_info[2][20];

/* Pixel look-up tables and table base address */
static uint8 *lut[5];
static uint8 *lut_base = NULL;

#ifndef NGC
/* 8-bit pixel remapping data */
static uint8 pixel_8[0x100];
static uint8 pixel_8_lut[3][0x200];

/* 15-bit pixel remapping data */
static uint16 pixel_15[0x100];
static uint16 pixel_15_lut[3][0x200];

/* 32-bit pixel remapping data */
static uint32 pixel_32[0x100];
static uint32 pixel_32_lut[3][0x200];
#endif

/* 16-bit pixel remapping data */
static uint16 pixel_16[0x100];
static uint16 pixel_16_lut[3][0x200];

/* Line buffers */
static uint8 tmp_buf[0x200];  /* Temporary buffer */
static uint8 bg_buf[0x200];   /* Merged background buffer */
static uint8 nta_buf[0x200];  /* Plane A / Window line buffer */
static uint8 ntb_buf[0x200];  /* Plane B line buffer */
static uint8 obj_buf[0x200];  /* Object layer line buffer */

/* Sprite line buffer data */
uint8 object_count[2];
uint8 object_which;

/*--------------------------------------------------------------------------*/
/* Look-up table functions (handles priority between layers pixels)         */
/*--------------------------------------------------------------------------*/

static void palette_init(void)
{
  int i;

  for (i = 0; i < 0x200; i += 1)
  {
    int r, g, b;

    r = (i >> 6) & 7;
    g = (i >> 3) & 7;
    b = (i >> 0) & 7;

#ifndef NGC
    pixel_8_lut[0][i] = MAKE_PIXEL_8(r>>1,g>>1,b>>1);
    pixel_8_lut[1][i] = MAKE_PIXEL_8(r,g,b);
    pixel_8_lut[2][i] = MAKE_PIXEL_8((r+7)>>1,(g+7)>>1,(b+7)>>1);

    pixel_15_lut[0][i] = MAKE_PIXEL_15(r,g,b);
    pixel_15_lut[1][i] = MAKE_PIXEL_15(r<<1,g<<1,b<<1);
    pixel_15_lut[2][i] = MAKE_PIXEL_15(r+7,g+7,b+7);

    pixel_32_lut[0][i] = MAKE_PIXEL_32(r,g,b);
    pixel_32_lut[1][i] = MAKE_PIXEL_32(r<<1,g<<1,b<<1);
    pixel_32_lut[2][i] = MAKE_PIXEL_32(r+7,g+7,b+7);
#endif

    pixel_16_lut[0][i] = MAKE_PIXEL_16(r,g,b);
    pixel_16_lut[1][i] = MAKE_PIXEL_16(r<<1,g<<1,b<<1);
    pixel_16_lut[2][i] = MAKE_PIXEL_16(r+7,g+7,b+7);
  }
}

static void make_name_lut(void)
{
  int col, row;
  int vcol, vrow;
  int width, height;
  int flipx, flipy;
  int i, name;

  memset (name_lut, 0, sizeof (name_lut));

  for (i = 0; i < 0x400; i += 1)
  {
    vcol = col = i & 3;
    vrow = row = (i >> 2) & 3;
    height = (i >> 4) & 3;
    width = (i >> 6) & 3;
    flipx = (i >> 8) & 1;
    flipy = (i >> 9) & 1;

    if(flipx) vcol = (width - col);
    if(flipy) vrow = (height - row);

    name = vrow + (vcol * (height + 1));

    if ((row > height) || col > width) name = -1;

    name_lut[i] = name;
  }
}

/* Input (bx):  d5-d0=color, d6=priority, d7=unused */
/* Input (ax):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=priority, d7=unused */
static uint32 make_lut_bg(uint32 bx, uint32 ax)
{
  int bf, bp, b;
  int af, ap, a;
  int x = 0;
  int c;

  bf = (bx & 0x7F);
  bp = (bx >> 6) & 1;
  b  = (bx & 0x0F);
  
  af = (ax & 0x7F);   
  ap = (ax >> 6) & 1;
  a  = (ax & 0x0F);

  c = (ap ? (a ? af : (b ? bf : x)) : \
    (bp ? (b ? bf : (a ? af : x)) : \
    (   (a ? af : (b ? bf : x)) )));

  /* Strip palette bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0xC0;

  return (c);
}


/* Input (bx):  d5-d0=color, d6=priority, d7=sprite pixel marker */
/* Input (sx):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=zero, d7=sprite pixel marker */
static uint32 make_lut_obj(uint32 bx, uint32 sx)
{
  int bf, bp, bs, b;
  int sf, sp, s;
  int c;

  bf = (bx & 0x3F);
  bs = (bx >> 7) & 1;
  bp = (bx >> 6) & 1;
  b  = (bx & 0x0F);
  
  sf = (sx & 0x3F);
  sp = (sx >> 6) & 1;
  s  = (sx & 0x0F);

  if(s == 0) return bx;

  if(bs)
  {
    c = bf; /* previous sprite has higher priority */
  }
  else
  {
    c = (sp ? (s ? sf : bf)  : \
      (bp ? (b ? bf : (s ? sf : bf)) : \
          (s ? sf : bf) ));
  }

  /* Strip palette bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0xC0;

  return (c | 0x80);
}


/* Input (bx):  d5-d0=color, d6=priority, d7=unused */
/* Input (sx):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=priority, d7=intensity select (half/normal) */
static uint32 make_lut_bg_ste(uint32 bx, uint32 ax)
{
  int bf, bp, b;
  int af, ap, a;
  int gi;
  int x = 0;
  int c;

  bf = (bx & 0x7F);
  bp = (bx >> 6) & 1;
  b  = (bx & 0x0F);
  
  af = (ax & 0x7F);   
  ap = (ax >> 6) & 1;
  a  = (ax & 0x0F);

  gi = (ap | bp) ? 0x80 : 0x00;

  c = (ap ? (a ? af : (b ? bf : x)) :
     (bp ? (b ? bf : (a ? af : x)) : ((a ? af : (b ? bf : x)))));

  c |= gi;

  /* Strip palette bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0xC0;

  return (c);
}


/* Input (bx):  d5-d0=color, d6=priority, d7=sprite pixel marker */
/* Input (sx):  d5-d0=color, d6=priority, d7=unused */
/* Output:    d5-d0=color, d6=priority, d7=sprite pixel marker */
static uint32 make_lut_obj_ste(uint32 bx, uint32 sx)
{
  int bf, bs;
  int sf;
  int c;

  bf = (bx & 0x7F);   
  bs = (bx >> 7) & 1; 
  sf = (sx & 0x7F);

  if((sx & 0x0F) == 0) return bx;

  c = (bs) ? bf : sf;

  /* Strip palette bits from transparent pixels */
  if((c & 0x0F) == 0x00) c &= 0xC0;

  return (c | 0x80);
}


/* Input (bx):  d5-d0=color, d6=priority, d7=intensity (half/normal) */
/* Input (sx):  d5-d0=color, d6=priority, d7=sprite marker */
/* Output:    d5-d0=color, d6=intensity (half/normal), d7=(double/invalid) */
static uint32 make_lut_bgobj_ste(uint32 bx, uint32 sx)
{
  int c;

  int bf = (bx & 0x3F);
  int bp = (bx >> 6) & 1;
  int bi = (bx & 0x80) ? 0x40 : 0x00;
  int b  = (bx & 0x0F);

  int sf = (sx & 0x3F);
  int sp = (sx >> 6) & 1;
  int si = (sx & 0x40);
  int s  = (sx & 0x0F);

  if(bi & 0x40) si |= 0x40;

  if(sp)
  {
    if(s)
    {      
      if((sf & 0x3E) == 0x3E)
      {
        if(sf & 1)
        {
          c = (bf | 0x00);
        }
        else
        {
          c = (bx & 0x80) ? (bf | 0x80) : (bf | 0x40);
        }
      }
      else
      {
        if(sf == 0x0E || sf == 0x1E || sf == 0x2E)
        {
          c = (sf | 0x40);
        }
        else
        {
          c = (sf | si);
        }
      }
    }
    else
    {
      c = (bf | bi);
    }
  }
  else
  {
    if(bp)
    {
      if(b)
      {
        c = (bf | bi);
      }
      else
      {
        if(s)
        {
          if((sf & 0x3E) == 0x3E)
          {
            if(sf & 1)
            {
              c = (bf | 0x00);
            }
            else
            {
              c = (bx & 0x80) ? (bf | 0x80) : (bf | 0x40);
            }
          }
          else
          {
            if(sf == 0x0E || sf == 0x1E || sf == 0x2E)
            {
              c = (sf | 0x40);
            }
            else
            {
              c = (sf | si);
            }
          }
        }
        else
        {
          c = (bf | bi);
        }
      }
    }
    else
    {
      if(s)
      {
        if((sf & 0x3E) == 0x3E)
        {
          if(sf & 1)
          {
            c = (bf | 0x00);
          }
          else
          {
            c = (bx & 0x80) ? (bf | 0x80) : (bf | 0x40);
          }
        }
        else
        {
          if(sf == 0x0E || sf == 0x1E || sf == 0x2E)
          {
            c = (sf | 0x40);
          }
          else
          {
            c = (sf | si);
          }
        }
      }
      else
      {          
        c = (bf | bi);
      }
    }
  }

  if((c & 0x0f) == 0x00) c &= 0xC0;

  return (c);
}

/*--------------------------------------------------------------------------*/
/* Color update functions                                                   */
/*--------------------------------------------------------------------------*/
#ifndef NGC

static void color_update_8(int index, uint16 data)
{
  /* VDP Palette Selection bit */
  /* color value is limited to 00X00X00X */
  if (!(reg[0] & 4)) data &= 0x49;

  if(reg[12] & 8)
  {
    pixel_8[0x00 | index] = pixel_8_lut[0][data];
    pixel_8[0x40 | index] = pixel_8_lut[1][data];
    pixel_8[0x80 | index] = pixel_8_lut[2][data];
  }
  else
  {
    uint8 temp = pixel_8_lut[1][data];
    pixel_8[0x00 | index] = temp;
    pixel_8[0x40 | index] = temp;
    pixel_8[0x80 | index] = temp;
  }
}

static void color_update_15(int index, uint16 data)
{
  /* VDP Palette Selection bit */
  /* color value is limited to 00X00X00X */
  if (!(reg[0] & 4)) data &= 0x49;

  if(reg[12] & 8)
  {
    pixel_15[0x00 | index] = pixel_15_lut[0][data];
    pixel_15[0x40 | index] = pixel_15_lut[1][data];
    pixel_15[0x80 | index] = pixel_15_lut[2][data];
  }
  else
  {
    uint16 temp = pixel_15_lut[1][data];
    pixel_15[0x00 | index] = temp;
    pixel_15[0x40 | index] = temp;
    pixel_15[0x80 | index] = temp;
  }
}

static void color_update_32(int index, uint16 data)
{
  /* VDP Palette Selection bit */
  /* color value is limited to 00X00X00X */
  if (!(reg[0] & 4)) data &= 0x49;

  if(reg[12] & 8)
  {
    pixel_32[0x00 | index] = pixel_32_lut[0][data];
    pixel_32[0x40 | index] = pixel_32_lut[1][data];
    pixel_32[0x80 | index] = pixel_32_lut[2][data];
  }
  else
  {
    uint32 temp = pixel_32_lut[1][data];
    pixel_32[0x00 | index] = temp;
    pixel_32[0x40 | index] = temp;
    pixel_32[0x80 | index] = temp;
  }
}

#endif

static void color_update_16(int index, uint16 data)
{
  /* VDP Palette Selection bit */
  /* color value is limited to 00X00X00X */
  if (!(reg[0] & 4)) data &= 0x49;

  if(reg[12] & 8)
  {
    pixel_16[0x00 | index] = pixel_16_lut[0][data];
    pixel_16[0x40 | index] = pixel_16_lut[1][data];
    pixel_16[0x80 | index] = pixel_16_lut[2][data];
  }
  else
  {
    uint16 temp = pixel_16_lut[1][data];
    pixel_16[0x00 | index] = temp;
    pixel_16[0x40 | index] = temp;
    pixel_16[0x80 | index] = temp;
  }
}

/*--------------------------------------------------------------------------*/
/* Remap functions                                                          */
/*--------------------------------------------------------------------------*/

#ifndef NGC

static inline void remap_8(uint8 *src, uint8 *dst, uint8 *table, int length)
{
  do
  {
    *dst++ = table[*src++];
  }
  while (--length);
}

static inline void remap_16(uint8 *src, uint16 *dst, uint16 *table, int length)
{
  do
  {
    *dst++ = table[*src++];
  }
  while (--length);
}

static inline void remap_32(uint8 *src, uint32 *dst, uint32 *table, int length)
{
  do
  {
    *dst++ = table[*src++];
  }
  while (--length);
}

#else

static inline void remap_texture(uint8 *src, uint16 *dst, uint16 *table, int tiles)
{
  do
  {
    /* one tile is 4 pixels wide */
    *dst++ = table[*src++];
    *dst++ = table[*src++];
    *dst++ = table[*src++];
    *dst++ = table[*src++];
    dst += 12;
  }
  while (--tiles);
}
#endif


static inline void merge(uint8 *srca, uint8 *srcb, uint8 *dst, uint8 *table, int width)
{
  do
  {
    *dst++ = table[(*srcb++ << 8) | (*srca++)];
  }
  while (--width);
}

/*--------------------------------------------------------------------------*/
/* Helper functions (cache update, hscroll, window clip)                    */
/*--------------------------------------------------------------------------*/

static void update_bg_pattern_cache(int index)
{
  int i;
  uint8 x, y, c;
  uint16 name;
  uint8 *dst;
  uint32 bp;
#ifdef LSB_FIRST
  uint8 shift_table[8] = {12, 8, 4, 0, 28, 24, 20, 16};
#else
  uint8 shift_table[8] = {28, 24, 20, 16, 12, 8, 4, 0};
#endif        

  for(i = 0; i < index; i++)
  {
    name = bg_name_list[i];
    bg_name_list[i] = 0;
    
    for(y = 0; y < 8; y ++)
    {
      if(bg_name_dirty[name] & (1 << y))
      {
        dst = &bg_pattern_cache[name << 6];
        bp = *(uint32 *)&vram[(name << 5) | (y << 2)];

        for(x = 0; x < 8; x ++)
        {
          c = (bp >> shift_table[x]) & 0x0F;
          dst[0x00000 | (y << 3) | (x)] = (c);      /* hf=0, vf=0: normal */
          dst[0x20000 | (y << 3) | (x ^ 7)] = (c);    /* hf=1, vf=0: horizontal flipped */
          dst[0x40000 | ((y ^ 7) << 3) | (x)] = (c);    /* hf=0, vf=1: vertical flipped */
          dst[0x60000 | ((y ^ 7) << 3) | (x ^ 7)] = (c);  /* hf=1, vf=1: horizontal & vertical flipped */
        }
      }
    }
    bg_name_dirty[name] = 0;
  }
}

static inline uint32 get_hscroll(int line)
{
  switch(reg[11] & 3)
  {
    case 0: /* Full-screen */
      return *(uint32 *)&vram[hscb];

    case 1: /* First 8 lines */
      return *(uint32 *)&vram[hscb + ((line & 7) << 2)];

    case 2: /* Every 8 lines */
      return *(uint32 *)&vram[hscb + ((line & ~7) << 2)];

    default: /* Every line */
      return *(uint32 *)&vram[hscb + (line << 2)];
  }
}


/*--------------------------------------------------------------------------*/
/* Layers render functions                                                  */
/*--------------------------------------------------------------------------*/
static void render_bg(int line, int width)
{
  uint32 column, atex, atbuf, *src, *dst;

  /* common data */
  uint32 xscroll      = get_hscroll(line);
  uint32 yscroll      = *(uint32 *)&vsram[0];
  uint32 pf_col_mask  = playfield_col_mask;
  uint32 pf_row_mask  = playfield_row_mask;
  uint32 pf_shift     = playfield_shift;

  /* Plane B */
  uint8 *buf    = ntb_buf;
  uint32 start  = 0;
  uint32 end    = width >> 4;

#ifdef LSB_FIRST
  uint32 shift  = (xscroll >> 16) & 0x0F;
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  uint32 v_line = (line + ((yscroll >> 16) & 0x3FF)) & pf_row_mask;
#else
  uint32 shift  = (xscroll & 0x0F);
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  uint32 v_line = (line + (yscroll & 0x3FF)) & pf_row_mask;
#endif

  uint32 *nt    = (uint32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
  v_line        = (v_line & 7) << 3;

  if(shift)
  {
    dst   = (uint32 *)&buf[0x10 + shift];
    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }
  else
  {
    dst = (uint32 *)&buf[0x20];
  }

  for(column = 0; column < end; column ++, index ++)
  {
    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }

  /* Window and Plane A */
  buf = nta_buf;

  uint32 a  = (reg[18] & 0x1F) << 3;
  uint32 w  = (reg[18] >> 7) & 1;

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* set for Plane A */
    start = clip[0].left;
    end   = clip[0].right;

#ifdef LSB_FIRST
    shift   = (xscroll & 0x0F);
    index   = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line  = (line + (yscroll & 0x3FF)) & pf_row_mask;
#else
    shift   = (xscroll >> 16) & 0x0F;
    index   = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line  = (line + ((yscroll >> 16) & 0x3FF)) & pf_row_mask;
#endif

    nt      = (uint32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line  = (v_line & 7) << 3;

    if(shift)
    {
      dst = (uint32 *)&buf[0x10 + shift + (start<<4)];

      /* Window bug */
      if (start) atbuf = nt[index & pf_col_mask];
      else atbuf = nt[(index-1) & pf_col_mask];

      DRAW_COLUMN(atbuf, v_line)
    }
    else
    {
      dst = (uint32 *)&buf[0x20 + (start<<4)];
    }

    for(column = start; column < end; column ++, index ++)
    {
      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN(atbuf, v_line)
    }

    /* set for Window */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    v_line  = (line & 7) << 3;
    nt      = (uint32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];
    dst     = (uint32 *)&buf[0x20 + (start << 4)];

    for(column = start; column < end; column ++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }
}

static void render_bg_vs(int line, int width)
{
  uint32 column, atex, atbuf, *src, *dst;
  uint32 v_line, *nt;

  /* common data */
  uint32 xscroll      = get_hscroll(line);
  uint32 yscroll      = 0;
  uint32 pf_col_mask  = playfield_col_mask;
  uint32 pf_row_mask  = playfield_row_mask;
  uint32 pf_shift     = playfield_shift;
  uint32 *vs          = (uint32 *)&vsram[0];

  /* Plane B */
  uint8 *buf    = ntb_buf;
  uint32 start  = 0;
  uint32 end    = width >> 4;

#ifdef LSB_FIRST
  uint32 shift  = (xscroll >> 16) & 0x0F;
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  uint32 shift  = (xscroll & 0x0F);
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* left-most column v-scrolling when fine h-scrolling value is applied */
  /* Gynoug, Cutie Suzuki no Ringside Angel, Formula One, Kawasaki Superbike Challenge */
  if (reg[12] & 1)
  {
    /* only in 40-cell mode, verified on MD2 */
    yscroll = vs[19] & (vs[19] >> 16);
    yscroll &= 0x3FF;
  }

  if(shift)
  {
    dst   = (uint32 *)&buf[0x10 + shift];

    v_line = (line + yscroll) & pf_row_mask;
    nt = (uint32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line = (v_line & 7) << 3;

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }
  else
  {
    dst = (uint32 *)&buf[0x20];
  }

  for(column = 0; column < end; column ++, index ++)
  {
#ifdef LSB_FIRST
    v_line = (line + ((vs[column] >> 16) & 0x3FF)) & pf_row_mask;
#else
    v_line = (line + (vs[column] & 0x3FF)) & pf_row_mask;
#endif
    nt = (uint32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line = (v_line & 7) << 3;

    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN(atbuf, v_line)
  }
  
  /* Window and Plane A */
  buf = nta_buf;

  uint32 a  = (reg[18] & 0x1F) << 3;
  uint32 w  = (reg[18] >> 7) & 1;

  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A*/
  if (a)
  {
    /* set for Plane A */
    start = clip[0].left;
    end   = clip[0].right;

#ifdef LSB_FIRST
    shift     = (xscroll & 0x0F);
    index     = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift     = (xscroll >> 16) & 0x0F;
    index     = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    if(shift)
    {
      dst = (uint32 *)&buf[0x10 + shift + (start<<4)];

      v_line = (line + yscroll) & pf_row_mask;
      nt = (uint32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
      v_line = (v_line & 7) << 3;

      /* Window bug */
      if (start) atbuf = nt[index & pf_col_mask];
      else atbuf = nt[(index-1) & pf_col_mask];

      DRAW_COLUMN(atbuf, v_line)
    }
    else
    {
      dst = (uint32 *)&buf[0x20 + (start<<4)];
    }

    for(column = start; column < end; column ++, index ++)
    {
#ifdef LSB_FIRST
      v_line = (line + (vs[column] & 0x3FF)) & pf_row_mask;
#else
      v_line = (line + ((vs[column] >> 16) & 0x3FF)) & pf_row_mask;
#endif
      nt = (uint32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
      v_line = (v_line & 7) << 3;

      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN(atbuf, v_line)
    }

    /* set for Window */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    v_line  = (line & 7) << 3;
    nt      = (uint32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];
    dst     = (uint32 *)&buf[0x20 + (start << 4)];
    for(column = start; column < end; column ++)
    {
      atbuf = nt[column];
      DRAW_COLUMN(atbuf, v_line)
    }
  }
}

static void render_bg_im2(int line, int width, int odd)
{
  uint32 column, atex, atbuf, offs, *src, *dst;

  /* common data */
  uint32 xscroll      = get_hscroll(line);
  uint32 yscroll      = *(uint32 *)&vsram[0];
  uint32 pf_col_mask  = playfield_col_mask;
  uint32 pf_row_mask  = playfield_row_mask;
  uint32 pf_shift     = playfield_shift;

  /* Plane B */
  uint8 *buf    = ntb_buf;
  uint32 start  = 0;
  uint32 end    = width >> 4;

#ifdef LSB_FIRST
  uint32 shift  = (xscroll >> 16) & 0x0F;
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
  uint32 v_line = (line + ((yscroll >> 17) & 0x3FF)) & pf_row_mask;
#else
  uint32 shift  = (xscroll & 0x0F);
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
  uint32 v_line = (line + ((yscroll >> 1) & 0x3FF)) & pf_row_mask;
#endif

  uint32 *nt    = (uint32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
  v_line        = (((v_line & 7) << 1) | odd) << 3;

  if(shift)
  {
    dst   = (uint32 *)&buf[0x10 + shift];
    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }
  else
  {
    dst = (uint32 *)&buf[0x20];
  }

  for(column = 0; column < end; column ++, index ++)
  {
    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }

  /* Window and Plane A */
  buf = nta_buf;
  uint32 a  = (reg[18] & 0x1F) << 3;
  uint32 w  = (reg[18] >> 7) & 1;
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* set for Plane A */
    start = clip[0].left;
    end   = clip[0].right;

#ifdef LSB_FIRST
    shift   = (xscroll & 0x0F);
    index   = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
    v_line  = (line + ((yscroll >> 1) & 0x3FF)) & pf_row_mask;
#else
    shift   = (xscroll >> 16) & 0x0F;
    index   = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
    v_line  = (line + ((yscroll >> 17) & 0x3FF)) & pf_row_mask;
#endif

    nt      = (uint32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line  = (((v_line & 7) << 1) | odd) << 3;

    if(shift)
    {
      dst = (uint32 *)&buf[0x10 + shift + (start<<4)];

      /* Window bug */
      if (start) atbuf = nt[index & pf_col_mask];
      else atbuf = nt[(index-1) & pf_col_mask];

      DRAW_COLUMN_IM2(atbuf, v_line)
    }
    else
    {
      dst = (uint32 *)&buf[0x20 + (start<<4)];
    }

    for(column = start; column < end; column ++, index ++)
    {
      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    /* set for Window */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    v_line = ((line & 7) << 1 | odd) << 3;
    nt      = (uint32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];
    dst     = (uint32 *)&buf[0x20 + (start << 4)];
    for(column = start; column < end; column ++)
    {
      atbuf = nt[column];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }
  }
}

static void render_bg_im2_vs(int line, int width, int odd)
{
  uint32 column, atex, atbuf, offs, *src, *dst;
  uint32 v_line, *nt;

  /* common data */
  uint32 xscroll      = get_hscroll(line);
  uint32 yscroll      = 0;
  uint32 pf_col_mask  = playfield_col_mask;
  uint32 pf_row_mask  = playfield_row_mask;
  uint32 pf_shift     = playfield_shift;
  uint32 *vs          = (uint32 *)&vsram[0];

  /* Plane B */
  uint8 *buf    = ntb_buf;
  uint32 start  = 0;
  uint32 end    = width >> 4;

#ifdef LSB_FIRST
  uint32 shift  = (xscroll >> 16) & 0x0F;
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 20) & pf_col_mask);
#else
  uint32 shift  = (xscroll & 0x0F);
  uint32 index  = pf_col_mask + 1 - ((xscroll >> 4) & pf_col_mask);
#endif

  /* left-most column v-scrolling when fine h-scrolling value is applied */
  /* Gynoug, Cutie Suzuki no Ringside Angel, Formula One, Kawasaki Superbike Challenge */
  if (reg[12] & 1)
  {
    /* only in 40-cell mode, verified on MD2 */
    yscroll = (vs[19] >> 1) & (vs[19] >> 17);
    yscroll &= 0x3FF;
  }

  if(shift)
  {
    dst   = (uint32 *)&buf[0x10 + shift];

    v_line  = (line + yscroll) & pf_row_mask;
    nt      = (uint32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line  = (((v_line & 7) << 1) | odd) << 3;

    atbuf = nt[(index-1) & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }
  else
  {
    dst = (uint32 *)&buf[0x20];
  }

  for(column = 0; column < end; column ++, index ++)
  {
#ifdef LSB_FIRST
    v_line  = (line + ((vs[column] >> 17) & 0x3FF)) & pf_row_mask;
#else
    v_line  = (line + ((vs[column] >> 1) & 0x3FF)) & pf_row_mask;
#endif
    nt      = (uint32 *)&vram[ntbb + (((v_line >> 3) << pf_shift) & 0x1FC0)];
    v_line  = (((v_line & 7) << 1) | odd) << 3;

    atbuf = nt[index & pf_col_mask];
    DRAW_COLUMN_IM2(atbuf, v_line)
  }

  /* Window and Plane A */
  buf = nta_buf;
  uint32 a  = (reg[18] & 0x1F) << 3;
  uint32 w  = (reg[18] >> 7) & 1;
  if (w == (line >= a))
  {
    /* Window takes up entire line */
    a = 0;
    w = 1;
  }
  else
  {
    /* Window and Plane A share the line */
    a = clip[0].enable;
    w = clip[1].enable;
  }

  /* Plane A */
  if (a)
  {
    /* set for Plane A */
    start = clip[0].left;
    end   = clip[0].right;

#ifdef LSB_FIRST
    shift   = (xscroll & 0x0F);
    index   = pf_col_mask + start + 1 - ((xscroll >> 4) & pf_col_mask);
#else
    shift   = (xscroll >> 16) & 0x0F;
    index   = pf_col_mask + start + 1 - ((xscroll >> 20) & pf_col_mask);
#endif

    if(shift)
    {
      dst = (uint32 *)&buf[0x10 + shift + (start<<4)];

      v_line  = (line + yscroll) & pf_row_mask;
      nt = (uint32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
      v_line  = (((v_line & 7) << 1) | odd) << 3;

      /* Window bug */
      if (start) atbuf = nt[index & pf_col_mask];
      else atbuf = nt[(index-1) & pf_col_mask];

      DRAW_COLUMN_IM2(atbuf, v_line)
    }
    else
    {
      dst = (uint32 *)&buf[0x20 + (start<<4)];
    }

    for(column = start; column < end; column ++, index ++)
    {
#ifdef LSB_FIRST
      v_line  = (line + ((vs[column] >> 1) & 0x3FF)) & pf_row_mask;
#else
      v_line  = (line + ((vs[column] >> 17) & 0x3FF)) & pf_row_mask;
#endif
      nt      = (uint32 *)&vram[ntab + (((v_line >> 3) << pf_shift) & 0x1FC0)];
      v_line  = (((v_line & 7) << 1) | odd) << 3;

      atbuf = nt[index & pf_col_mask];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }

    /* set for Window */
    start = clip[1].left;
    end   = clip[1].right;
  }

  /* Window */
  if (w)
  {
    v_line = ((line & 7) << 1 | odd) << 3;
    nt      = (uint32 *)&vram[ntwb | ((line >> 3) << (6 + (reg[12] & 1)))];
    dst     = (uint32 *)&buf[0x20 + (start << 4)];
    for(column = start; column < end; column ++)
    {
      atbuf = nt[column];
      DRAW_COLUMN_IM2(atbuf, v_line)
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Object render functions                                                  */
/*--------------------------------------------------------------------------*/

static int spr_over = 0;

static void render_obj(uint8 *buf, uint8 *table)
{
  uint8 sizetab[] = {8, 16, 24, 32};

  int i, count, column;
  int xpos, width;
  int pixelmax = bitmap.viewport.w;
  int pixelcount = 0;
  int masked = 0;

  uint8 *src, *s, *lb;
  uint32 size, v_line;
  uint32 attr, attr_mask, name, palette, index;

  object *obj_info = object_info[object_which];

  for(count = 0; count < object_count[object_which]; count ++)
  {
    /* sprite horizontal position */
    xpos = obj_info[count].xpos;

    /* sprite masking (requires at least one sprite with xpos > 0) */
    if (xpos)
    {
      spr_over = 1;
    }
    else if (spr_over)
    {
      spr_over = 0;
      masked = 1;
    }

    /* sprite horizontal ofsfet */
    xpos = xpos - 0x80;

    /* sprite size */
    size = obj_info[count].size;
    width = sizetab[(size >> 2) & 3];

    /* update pixel count (off-screen sprites are included) */
    pixelcount += width;

    /* draw visible sprites */
    if (((xpos + width) > 0) && (xpos < pixelmax) && !masked)
    {
      /* sprite attributes + pattern index */
      attr = obj_info[count].attr;
      attr_mask = attr & 0x1800;
      palette = (attr >> 9) & 0x70;
      name = attr & 0x07FF;

      /* sprite vertical offset */
      v_line = obj_info[count].ypos;
      s = &name_lut[((attr >> 3) & 0x300) | (size << 4) | ((v_line & 0x18) >> 1)];
      v_line = (v_line & 7) << 3;

      /* pointer into line buffer */
      lb = &buf[0x20 + xpos];

      /* adjust width for sprite limit */
      if (pixelcount > pixelmax)
      {
        width = width - pixelcount + pixelmax;
      }

      /* number of tiles to draw */
      width = width >> 3;

      /* render sprite cells (8-pixels column) */
      for(column = 0; column < width; column++, lb+=8)
      {
        index = attr_mask | ((name + s[column]) & 0x07FF);
        src = &bg_pattern_cache[(index << 6) | (v_line)];
        DRAW_SPRITE_TILE;
      }
    }

    /* sprite limit (256 or 320 pixels) */
    if (pixelcount >= pixelmax)
    {
      spr_over = 1;
      return;
    }
  }

  spr_over = 0;
}

static void render_obj_im2(uint8 *buf, uint8 *table, int odd)
{
  uint8 sizetab[] = {8, 16, 24, 32};

  int i, count, column;
  int xpos, width;
  int pixelmax = bitmap.viewport.w;
  int pixelcount = 0;
  int masked = 0;

  uint8 *src, *s, *lb;
  uint32 size, v_line;
  uint32 attr, attr_mask, name, palette, index;
  uint32 offs;

  object *obj_info = object_info[object_which];

  for(count = 0; count < object_count[object_which]; count ++)
  {
    /* sprite horizontal position */
    xpos = obj_info[count].xpos;

    /* sprite masking (requires at least one sprite with xpos > 0) */
    if (xpos)
    {
      spr_over = 1;
    }
    else if(spr_over)
    {
      spr_over = 0;
      masked = 1;
    }

    /* sprite horizontal ofsfet */
    xpos = xpos - 0x80;

    /* sprite size */
    size = obj_info[count].size;
    width = sizetab[(size >> 2) & 3];

    /* update pixel count (off-screen sprites are included) */
    pixelcount += width;

    /* draw visible sprites */
    if (((xpos + width) > 0) && (xpos < pixelmax) && !masked)
    {
      /* sprite attributes + pattern index */
      attr = obj_info[count].attr;
      attr_mask = (attr & 0x1800);
      palette = (attr >> 9) & 0x70;
      name = (attr & 0x03FF);

      /* sprite vertical offset */
      v_line = obj_info[count].ypos;
      s = &name_lut[((attr >> 3) & 0x300) | (size << 4) | ((v_line & 0x18) >> 1)];
      v_line = (((v_line & 7) << 1) | odd) << 3;      

      /* pointer into line buffer */
      lb = &buf[0x20 + xpos];

      /* adjust width for sprite limit */
      if (pixelcount > pixelmax)
      {
        width = width - pixelcount + pixelmax;
      }

      /* number of tiles to draw */
      width = width >> 3;

      /* render sprite cells (8-pixels column) */
      for(column = 0; column < width; column += 1, lb+=8)
      {
        index = (name + s[column]) & 0x3ff;
        offs = index << 7 | attr_mask << 6 | v_line;
        if(attr & 0x1000) offs ^= 0x40;
        src = &bg_pattern_cache[offs];
        DRAW_SPRITE_TILE;
      }
    }

    /* sprite limit (256 or 320 pixels) */
    if (pixelcount >= pixelmax)
    {
      spr_over = 1;
      return;
    }
  }

  spr_over = 0;
}

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown routines                                           */
/*--------------------------------------------------------------------------*/

void render_init(void)
{
  int bx, ax, i;

  /* Allocate and align pixel look-up tables */
  if (lut_base == NULL)
    lut_base = malloc ((LUT_MAX * LUT_SIZE) + LUT_SIZE);
  lut[0] = (uint8 *) (((uint32) lut_base + LUT_SIZE) & ~(LUT_SIZE - 1));
  for (i = 1; i < LUT_MAX; i += 1)
    lut[i] = lut[0] + (i * LUT_SIZE);

  /* Make pixel look-up table data */
  for (bx = 0; bx < 0x100; bx += 1)
    for (ax = 0; ax < 0x100; ax += 1)
    {
      uint16 index = (bx << 8) | (ax);
      lut[0][index] = make_lut_bg (bx, ax);
      lut[1][index] = make_lut_obj (bx, ax);
      lut[2][index] = make_lut_bg_ste (bx, ax);
      lut[3][index] = make_lut_obj_ste (bx, ax);
      lut[4][index] = make_lut_bgobj_ste (bx, ax);
    }

  /* Make pixel data tables */
  palette_init();

  /* Set up color update function */
#ifndef NGC
  switch(bitmap.depth)
  {
    case 8: color_update = color_update_8; break;
    case 15: color_update = color_update_15; break;
    case 16: color_update = color_update_16; break;
    case 32: color_update = color_update_32; break;
  }
#else
  color_update = color_update_16;
#endif

  /* Make sprite name look-up table */
  make_name_lut();
}

void render_reset(void)
{
  /* Clear display bitmap */
  memset(bitmap.data, 0, bitmap.pitch * bitmap.height);

  memset(bg_buf, 0, sizeof(bg_buf));
  memset(tmp_buf, 0, sizeof(tmp_buf));
  memset(nta_buf, 0, sizeof(nta_buf));
  memset(ntb_buf, 0, sizeof(ntb_buf));
  memset(obj_buf, 0, sizeof(obj_buf));

#ifndef NGC
  memset(&pixel_8, 0, sizeof(pixel_8));
  memset(&pixel_15, 0, sizeof(pixel_15));
  memset(&pixel_32, 0, sizeof(pixel_32));
#endif
  memset(&pixel_16, 0, sizeof(pixel_16));

  window_clip();
}


void render_shutdown(void)
{
  if(lut_base)
    free(lut_base);
}


/*--------------------------------------------------------------------------*/
/* Line render function                                                     */
/*--------------------------------------------------------------------------*/
void blank_line(int line, int offset, int width)
{
  memset(&tmp_buf[0x20 + offset], 0x40, width);
  remap_buffer(line);
}

void render_line(int line)
{
  uint8 *lb     = tmp_buf;
  int width     = bitmap.viewport.w;
  int x_offset  = bitmap.viewport.x;

  /* background color (blanked display or vertical borders) */
  if (!(reg[1] & 0x40) || (status & 8))
  {
    memset(&lb[0x20 - x_offset], 0x40, width + 2*x_offset);
  }
  else
  {
    /* update pattern generator */
    if (bg_list_index)
    {
      update_bg_pattern_cache(bg_list_index);
      bg_list_index = 0;
    }

    /* double-resolution mode */
    if(im2_flag)
    {
      int odd = odd_frame;

      /* render BG layers */
      if(reg[11] & 4)
        render_bg_im2_vs(line, width, odd);
      else
        render_bg_im2(line, width, odd);

      if (reg[12] & 8)
      {
        /* Shadow & Highlight */
        merge(&nta_buf[0x20], &ntb_buf[0x20], &bg_buf[0x20], lut[2], width);
        memset(&obj_buf[0x20], 0, width);
        render_obj_im2(obj_buf, lut[3], odd);
        merge(&obj_buf[0x20], &bg_buf[0x20], &lb[0x20], lut[4], width);
      }
      else
      {
        merge(&nta_buf[0x20], &ntb_buf[0x20], &lb[0x20], lut[0], width);
        render_obj_im2(lb, lut[1], odd);
      }
    }
    else
    {
      /* render BG layers */
      if(reg[11] & 4)
        render_bg_vs(line, width);
      else
        render_bg(line, width);

      if(reg[12] & 8)
      {
        /* Shadow & Highlight */
        merge(&nta_buf[0x20], &ntb_buf[0x20], &bg_buf[0x20], lut[2], width);
        memset(&obj_buf[0x20], 0, width);
        render_obj(obj_buf, lut[3]);
        merge(&obj_buf[0x20], &bg_buf[0x20], &lb[0x20], lut[4], width);
      }
      else
      {
        merge(&nta_buf[0x20], &ntb_buf[0x20], &lb[0x20], lut[0], width);
        render_obj(lb, lut[1]);
      }
    }

    /* left-most column blanking */
    if(reg[0] & 0x20) memset(&lb[0x20], 0x40, 0x08);

    /* horizontal borders */
    if (x_offset)
    {
      memset(&lb[0x20 - x_offset], 0x40, x_offset);
      memset(&lb[0x20 + width], 0x40, x_offset);
    }
  }

  /* pixel color remapping */
  remap_buffer(line);
}

void remap_buffer(int line)
{
  /* display disabled */
  if (reg[0] & 0x01) return;
  
  int width = bitmap.viewport.w + 2*bitmap.viewport.x;

  /* get line offset from framebuffer */
  line = (line + bitmap.viewport.y) % lines_per_frame;

  /* double resolution mode */
  if (config.render && interlaced)
    line = (line << 1) + odd_frame;

  /* NTSC Filter */
  if (config.ntsc)
  {
    if (reg[12]&1)
      md_ntsc_blit(md_ntsc, ( MD_NTSC_IN_T const * )pixel_16, tmp_buf+0x20-bitmap.viewport.x, width, line);
    else
      sms_ntsc_blit(sms_ntsc, ( SMS_NTSC_IN_T const * )pixel_16, tmp_buf+0x20-bitmap.viewport.x, width, line);
    return;
  }

#ifdef NGC
  /* directly fill a RGB565 texture */
  /* one tile is 32 byte = 4x4 pixels */
  /* tiles are stored continuously in texture memory */
  width >>= 2;
  uint16 *out = (uint16 *) (texturemem + (((width << 5) * (line >> 2)) + ((line & 3) << 3)));
  remap_texture(tmp_buf+0x20-bitmap.viewport.x, out, pixel_16, width);
#else
  void *out =((void *)&bitmap.data[(line * bitmap.pitch)]);
  switch(bitmap.depth)
  {
    case 8:
      remap_8(tmp_buf+0x20-bitmap.viewport.x, (uint8 *)out, pixel_8, width);
      break;
    case 15:
      remap_16(tmp_buf+0x20-bitmap.viewport.x, (uint16 *)out, pixel_15, width);
      break;
    case 16:
      remap_16(tmp_buf+0x20-bitmap.viewport.x, (uint16 *)out, pixel_16, width);
      break;
    case 32:
      remap_32(tmp_buf+0x20-bitmap.viewport.x, (uint32 *)out, pixel_32, width);
      break;
  }
#endif
}

/* Update Window Clipping (now only called on VDP registers change) */
void window_clip(void)
{
  /* Window size and invert flags */
  int hp = (reg[17] & 0x1f);
  int hf = (reg[17] >> 7) & 1;

  /* Display size  */
  int sw = (reg[12] & 1) ? 20 : 16;

  /* Clear clipping data */
  memset(&clip, 0, sizeof(clip));

  /* Perform horizontal clipping; the results are applied in reverse
      if the horizontal inversion flag is set */
  int a = hf;
  int w = hf ^ 1;

  if(hp)
  {
    if(hp > sw)
    {
      /* Plane W takes up entire line */
      clip[w].right = sw;
      clip[w].enable = 1;
    }
    else
    {
      /* Window takes left side, Plane A takes right side */
      clip[w].right = hp;
      clip[a].left = hp;
      clip[a].right = sw;
      clip[0].enable = clip[1].enable = 1;
    }
  }
  else
  {
    /* Plane A takes up entire line */
    clip[a].right = sw;
    clip[a].enable = 1;
  }
}

/*--------------------------------------------------------------------------*/
/* Sprites Parsing function                                                 */
/*--------------------------------------------------------------------------*/

void parse_satb(int line)
{
  uint8 sizetab[] = {8, 16, 24, 32};
  uint32 size, link = 0;
  int ypos, height;
  
  uint32 count = 0;
  uint32 limit = (reg[12] & 1) ? 20 : 16;
  uint32 total = limit << 2;
  
  uint16 *p = (uint16 *) &vram[satb];
  uint16 *q = (uint16 *) &sat[0];
  
  object *obj_info = object_info[object_which^1];

  do
  {
    /* Read ypos & size from internal SAT */ 
    ypos = line - ((q[link] >> im2_flag) & 0x1FF);
    size = q[link + 1] >> 8;
    height = sizetab[size & 3];

    if ((ypos >= 0) && (ypos < height))
    {
      /* Sprite limit (max. 16 or 20 sprites displayed per line) */
      if(count == limit)
      {
        status |= 0x40;
        break;
      }

      /* Update sprite list */
      /* name, attribute & xpos are parsed from VRAM */ 
      obj_info[count].attr  = p[link + 2];
      obj_info[count].xpos  = p[link + 3] & 0x1ff;
      obj_info[count].ypos  = ypos;
      obj_info[count].size  = size & 0x0f;
      ++count;
    }

    /* Read link data from internal SAT */ 
    link = (q[link + 1] & 0x7F) << 2;
    if(link == 0) break;
  }
  while (--total);

  /* Update sprite count for next line */
  object_count[object_which^1] = count;
}