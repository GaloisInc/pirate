/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <jpeglib.h>

#include "common.h"

static unsigned char jpg_image[IMAGE_WIDTH * IMAGE_HEIGHT * 3];
static unsigned char jpg_row_image[IMAGE_WIDTH * 3];
// XDestroyImage() API will free this allocated memory
static char* translated_image;

static Display* x_display;
static Window x_window;
static XImage* x_image;
static GC x_context;
static XGCValues x_context_vals;
static XFontStruct* x_font;

int xwin_display_initialize() {
    x_display = XOpenDisplay(NULL);
    if (x_display == NULL) {
        ts_log(ERROR, "Failed to open X display");
        return -1;
    }
    int x_screen = DefaultScreen(x_display);
    x_window = XCreateSimpleWindow(x_display,
        RootWindow(x_display, x_screen), 10, 10, IMAGE_WIDTH, IMAGE_HEIGHT, 1,
        BlackPixel(x_display, x_screen), WhitePixel(x_display, x_screen));
    x_context = XCreateGC(x_display, x_window, 0, &x_context_vals);
    translated_image = calloc(IMAGE_WIDTH * IMAGE_HEIGHT * 4, 1);
    x_image = XCreateImage(x_display, CopyFromParent, 24, ZPixmap, 0, translated_image,
        IMAGE_WIDTH, IMAGE_HEIGHT, 32, 4 * IMAGE_WIDTH);
    x_font = XLoadQueryFont(x_display, "fixed");
    XSetFont(x_display, x_context, x_font->fid);
    XMapWindow(x_display, x_window);
    XSync(x_display, 0);
    return 0;
}

static void convert_jpg_to_ximage(const unsigned char *buf, unsigned long len) {
    int i, width, height, depth;
    unsigned x, y, z, k;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, buf, len);
    jpeg_read_header(&cinfo, 1);
    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;

    jpeg_start_decompress(&cinfo);
    width = cinfo.output_width;
    height = cinfo.output_height;
    depth = cinfo.num_components; //should always be 3

    row_pointer[0] = jpg_row_image;

    while(cinfo.output_scanline < cinfo.output_height) {
	    jpeg_read_scanlines(&cinfo, row_pointer, 1);
	    for(i = 0; i < (width * depth); i++) {
	        jpg_image[location++] = row_pointer[0][i];
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    for(z = k = y = 0; y < IMAGE_HEIGHT; y++) {
	    for(x = 0; x < IMAGE_WIDTH; x++) {
		    // for 24 bit depth, organization BGRX
            translated_image[k+0]=jpg_image[z+2];
			translated_image[k+1]=jpg_image[z+1];
			translated_image[k+2]=jpg_image[z+0];
			k+=4; z+=3;
		}
	}
}

void xwin_display_render_jpeg(const unsigned char *buf, unsigned long len) {
    convert_jpg_to_ximage(buf, len);
    XPutImage(x_display, x_window, x_context, x_image, 0, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
    XFlush(x_display);
}

static void show_line(char *msg, int msg_len, int offset) {
    int x, y, direction, ascent, descent;
    XCharStruct overall;

    XTextExtents(x_font, msg, msg_len, &direction, &ascent, &descent, &overall);
    x = IMAGE_WIDTH / 8;
    y = 20;
    y += offset * (x_font->ascent + x_font->descent);
    XDrawString(x_display, x_window, x_context, x, y, msg, msg_len);
}

void xwin_display_show_response(char *msg, int msg_len) {
    int pos = 0, row = 0, delta;
    char *pch;

    // trim message prefix
    pch = strstr(msg, "enc_digest:");
    if (pch == NULL) {
        return;
    }
    msg_len -= (pch - msg);
    msg = pch;
    // trim message suffix
    pch = strstr(msg, "        unauth_attr:");
    if (pch != NULL) {
        msg_len = (pch - msg);
    }
    while (pos < msg_len) {
        pch = strchr(msg + pos, '\n');
        if (pch != NULL) {
            delta = pch - msg - pos;
            show_line(msg + pos, delta - 1, row);
            pos += delta + 1;
        } else {
            show_line(msg + pos, msg_len - pos, row);
            pos = msg_len;
        }
        row += 1;
    }
    XFlush(x_display);
}

void xwin_display_terminate() {
    if (x_display != NULL) {
        XDestroyImage(x_image);
        XFreeGC(x_display, x_context);
        XFreeFont(x_display, x_font);
        XCloseDisplay(x_display);
    }
    x_image = NULL;
    x_font = NULL;
    x_display = NULL;
    translated_image = NULL;
}
