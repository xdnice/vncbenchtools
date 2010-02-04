/*  Copyright (C) 2000 Const Kaplinsky <const@ce.cctpu.edu.ru>
 *  Copyright (C) 2008 Sun Microsystems, Inc.
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
 */

/* misc functions */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "rfb.h"

char updateBuf[UPDATE_BUF_SIZE], *sendBuf=NULL;
int ublen, sblen=0, sbptr=0;

rfbClientRec rfbClient;
rfbScreenInfo rfbScreen;
rfbPixelFormat rfbServerFormat;

XImage _image, *image=&_image;

void InitEverything (int color_depth)
{
  int i;

  for (i = 0; i < MAX_ENCODINGS; i++) {
    rfbClient.rfbRectanglesSent[i] = 0;
    rfbClient.rfbBytesSent[i] = 0;
  }
  rfbClient.rfbFramebufferUpdateMessagesSent = 0;
  rfbClient.rfbRawBytesEquivalent = 0;

  rfbClient.compStreamInited = 0;
  for (i = 0; i < MAX_ENCODINGS; i++) {
    rfbClient.zsActive[i] = FALSE;
  }

  rfbClient.format.depth = color_depth;
  rfbClient.format.bitsPerPixel = color_depth;
  if (color_depth == 24)
    rfbClient.format.bitsPerPixel = 32;

  rfbServerFormat.depth = color_depth;
  rfbServerFormat.bitsPerPixel = rfbClient.format.bitsPerPixel;
  #ifdef _BIG_ENDIAN
  rfbServerFormat.bigEndian = 1;
  #else
  rfbServerFormat.bigEndian = 0; 
  #endif
  rfbServerFormat.trueColour = 1;
  rfbScreen.bitsPerPixel = rfbClient.format.bitsPerPixel;

  switch (color_depth) {
  case 8:
    rfbClient.format.redMax = 0x07;
    rfbClient.format.greenMax = 0x07;
    rfbClient.format.blueMax = 0x03;
    rfbClient.format.redShift = 0;
    rfbClient.format.greenShift = 3;
    rfbClient.format.blueShift = 6;
    break;
  case 16:
    rfbClient.format.redMax = 0x1F;
    rfbClient.format.greenMax = 0x3F;
    rfbClient.format.blueMax = 0x1F;
    rfbClient.format.redShift = 11;
    rfbClient.format.greenShift = 5;
    rfbClient.format.blueShift = 0;
    break;
  default:                      /* 24 */
    rfbClient.format.redMax = 0xFF;
    rfbClient.format.greenMax = 0xFF;
    rfbClient.format.blueMax = 0xFF;
    rfbClient.format.redShift = 16;
    rfbClient.format.greenShift = 8;
    rfbClient.format.blueShift = 0;
  }
  rfbClient.format.bigEndian = 0;
  rfbClient.format.trueColour = 1;

  rfbServerFormat.redMax = rfbClient.format.redMax;
  rfbServerFormat.greenMax = rfbClient.format.greenMax;
  rfbServerFormat.blueMax = rfbClient.format.blueMax;
  rfbServerFormat.redShift = rfbClient.format.redShift;
  rfbServerFormat.greenShift = rfbClient.format.greenShift;
  rfbServerFormat.blueShift = rfbClient.format.blueShift;
  if (rfbServerFormat.bigEndian) {
     rfbServerFormat.redShift = rfbServerFormat.bitsPerPixel - 8 -
                                (rfbServerFormat.redShift & (~7)) +
                                (rfbServerFormat.redShift & 7);
     rfbServerFormat.greenShift = rfbServerFormat.bitsPerPixel - 8 -
                                  (rfbServerFormat.greenShift & (~7)) +
                                  (rfbServerFormat.greenShift & 7);
     rfbServerFormat.blueShift = rfbServerFormat.bitsPerPixel - 8 -
                                 (rfbServerFormat.blueShift & (~7)) +
                                 (rfbServerFormat.blueShift & 7);
  }

  rfbSetTranslateFunction(&rfbClient);

  sendBuf = (char *)malloc(SEND_BUF_SIZE);
  if (!sendBuf) {
    printf("ERROR: Could not allocate send buffer.\n");
    exit(1);
  }

  ublen = 0;

  image->width = 1280;
  image->height = 1024;
  image->bits_per_pixel = rfbClient.format.bitsPerPixel;
  image->bytes_per_line = ((image->width * image->bits_per_pixel) + 3) & (~3);
  image->data = (char *)malloc(image->width * image->bytes_per_line);
}

extern int decompress;

BOOL rfbSendUpdateBuf(rfbClientPtr cl)
{
  if(decompress) {
    if (sblen + ublen > SEND_BUF_SIZE) {
      printf("ERROR: Send buffer overrun.\n");
      return False;
    }
    memcpy(&sendBuf[sblen], updateBuf, ublen);
    sblen += ublen;
  }
  ublen = 0;
  return TRUE;
}

Bool
ReadFromRFBServer(char *out, unsigned int n)
{
  if (sbptr + n > SEND_BUF_SIZE) {
    printf("ERROR: Send buffer overrun. %d %d %d\n", sbptr, n, SEND_BUF_SIZE);
    return False;
  }
  memcpy(out, &sendBuf[sbptr], n);
  sbptr += n;
  return True;
};

int rfbLog (char *fmt, ...)
{
  va_list arglist;
  va_start(arglist, fmt);
  vfprintf(stdout, fmt, arglist);
  return 0;
}

Bool
rfbSendRectEncodingRaw(cl, x, y, w, h)
    rfbClientPtr cl;
    int x, y, w, h;
{
  rfbClient.rfbBytesSent[rfbEncodingZlib] +=
    12 + w * h * (cl->format.bitsPerPixel / 8);
}

