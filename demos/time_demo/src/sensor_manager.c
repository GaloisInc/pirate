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
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#include <argp.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/ts.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <jpeglib.h>
#include <linux/videodev2.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "gaps_packet.h"
#include "ts_crypto.h"
#include "common.h"

typedef struct {
    verbosity_t verbosity;
    uint32_t validate;
    uint32_t request_delay_ms;
    uint32_t display;
    const char *ca_path;
    const char *cert_path;
    const char *tsr_dir;
    const char *video_device;
    gaps_app_t app;
} client_t;

const char *argp_program_version = DEMO_VERSION;
static struct argp_option options[] = {
    { "ca_path",      'C', "PATH", 0, "CA path",                          0 },
    { "cert_path",    'S', "PATH", 0, "Signing certificate path",         0 },
    { "verify",       'V', NULL,   0, "Verify timestamp signatures",      0 },
    { "save_path",    'O', "PATH", 0, "TSR output directory",             0 },
    { "video_device", 'D', "PATH", 0, "video device path",                0 },
    { "req_delay",    'd', "MS",   0, "Request delay in milliseconds",    0 },
    { "verbose",      'v', NULL,   0, "Increase verbosity level",         0 },
    { "headless",     'x', NULL,   0, "Run in headless mode",             0 },
    { 0 }
};

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480
#define DEFAULT_REQUEST_DELAY_MS 2000
#define DEFAULT_VIDEO_DEVICE "/dev/video0"

static unsigned char jpg_image[IMAGE_WIDTH * IMAGE_HEIGHT * 3];
static unsigned char jpg_row_image[IMAGE_WIDTH * 3];
// XDestroyImage() API will free this allocated memory
static char* translated_image;

static int video_fd;
static struct v4l2_buffer video_buf;
static void* video_mmap;

static Display* x_display;
static Window x_window;
static XImage* x_image;
static GC x_context;
static XGCValues x_context_vals;
static XFontStruct* x_font;

int ioctl_wait(int fd, unsigned long request, void *arg) {
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while ((ret == -1) && (errno == EINTR));
    return ret;
}

void sensor_manager_terminate() {
    if ((video_fd > 0) && (video_buf.length > 0)) {
        ioctl_wait(video_fd, VIDIOC_STREAMOFF, &video_buf.type);
    }
    if (video_fd > 0) {
        if ((video_mmap != NULL) && (video_buf.length > 0)) {
            munmap(video_mmap, video_buf.length);
        }
        close(video_fd);
    } else if (video_mmap != NULL) {
        free(video_mmap);
    }
    if (x_display != NULL) {
        XDestroyImage(x_image);
        XFreeGC(x_display, x_context);
        XFreeFont(x_display, x_font);
        XCloseDisplay(x_display);
    }
    video_fd = 0;
    video_mmap = NULL;
    x_image = NULL;
    x_font = NULL;
    x_display = NULL;
    translated_image = NULL;
}

#define DEFAULT_TSR_OUT_DIR "."

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    client_t *client = (client_t*) state->input;

    switch (key) {

    case 'C':
        client->ca_path = arg;
        break;

    case 'S':
        client->cert_path = arg;
        break;

    case 'V':
        client->validate = 1;
        break;

    case 'O':
        client->tsr_dir = arg;
        break;

    case 'D':
        client->video_device = arg;
        break;

    case 'd':
        client->request_delay_ms = strtol(arg, NULL, 10);
        break;

    case 'x':
        client->display = 0;
        break;

    case 'v':
        if (client->verbosity < VERBOSITY_MAX) {
            client->verbosity++;
        }
        break;

    default:
        break;
    }
    
    return 0;
}


static void parse_args(int argc, char *argv[], client_t *client) {
    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = "[FILE] [FILE] ...",
        .doc = "Sign files with the trusted timestamp service",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, client);
}

/* Generate sensor data */
static int read_sensor(int idx) {
    int read;
    fd_set fds;
    struct timeval tv = {0};
    char path[TS_PATH_MAX];
    FILE *f_in = NULL;

    if (video_fd <= 0) {
        snprintf(path, sizeof(path) - 1, "stock/%04u.jpg", idx % 16);
        if ((f_in = fopen(path, "r")) == NULL) {
            ts_log(ERROR, "Failed to open stock input file %s", path);
            return -1;
        }
        read = fread(video_mmap, 1, IMAGE_WIDTH * IMAGE_HEIGHT, f_in);
        if (read < 0) {
            ts_log(ERROR, "Failed to read stock input file %s", path);
            fclose(f_in);
            return -1;
        }
        video_buf.length = read;
        fclose(f_in);
        return 0;
    }

    if(ioctl_wait(video_fd, VIDIOC_QBUF, &video_buf) < 0) {
        ts_log(ERROR, "Failed to enqueue video buffer");
        return 1;
    }

    FD_ZERO(&fds);
    FD_SET(video_fd, &fds);

    tv.tv_sec = 2;
    if (select(video_fd+1, &fds, NULL, NULL, &tv) < 0) {
        ts_log(ERROR, "Failed to wait for video frame");
        return 1;
    }

    if(ioctl_wait(video_fd, VIDIOC_DQBUF, &video_buf) < 0) {
        ts_log(ERROR, "Failed to dequeue video buffer");
        return 1;
    }
    ts_log(INFO, "Retrieved image from camera");

    return 0;
}

static void convert_jpg_to_ximage() {
    int i, width, height, depth;
    unsigned x, y, z, k;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];

    unsigned long location = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, video_mmap, video_buf.length);
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

/* Save sensor data to a file */
static int save_sensor(const client_t* client, uint32_t idx) {
    char path[TS_PATH_MAX];
    FILE *f_out = NULL;

    if (client->tsr_dir == NULL) {
        return -1;
    }

    snprintf(path, sizeof(path) - 1, "%s/%04u.jpg", client->tsr_dir, idx);

    if ((f_out = fopen(path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open sensor output file");
        return -1;
    }

    if (fwrite(video_mmap, video_buf.length, 1, f_out) != 1) {
        ts_log(ERROR, "Failed to save sensor content");
        fclose(f_out);
        return -1;
    }

    fclose(f_out);

    if (client->display) {
        convert_jpg_to_ximage();
        XPutImage(x_display, x_window, x_context, x_image, 0, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
        XFlush(x_display);
    }

    return 0;
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

static void show_ts_response(char *msg, int msg_len) {
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

/* Save timestamp sign response to a file */
static int save_ts_response(const client_t *client, uint32_t idx, const tsa_response_t* rsp) {
    char path[TS_PATH_MAX];
    FILE *f_out = NULL;
    BIO *ts_resp_bio = NULL;
    BIO *display_bio = NULL;
    BUF_MEM *display_buf_mem = NULL;
    TS_RESP *response = NULL;
    PKCS7 *pkcs7 = NULL;

    if ((client->tsr_dir== NULL) ||
        (rsp->hdr.status != OK) ||
        (rsp->hdr.len > sizeof(rsp->ts))) {
        return 0;
    }

    snprintf(path, sizeof(path) - 1, "%s/%04u.tsr", client->tsr_dir, idx);

    if ((f_out = fopen(path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open TSR output file");
        return -1;
    }

    if (fwrite(rsp->ts, rsp->hdr.len, 1, f_out) != 1) {
        ts_log(ERROR, "Failed to save TSR content");
        fclose(f_out);
        return -1;
    }

    fclose(f_out);

    if (client->display) {
        ts_resp_bio = BIO_new_mem_buf(rsp->ts, rsp->hdr.len);
        if (ts_resp_bio == NULL) {
            ts_log(ERROR, "Failed to load TSR content");
            return -1;
        }

        response = d2i_TS_RESP_bio(ts_resp_bio, NULL);
        if (response == NULL) {
            BIO_free(ts_resp_bio);
            ts_log(ERROR, "Failed to convert TSR content");
            return -1;
        }

        display_bio = BIO_new(BIO_s_mem());
        if (display_bio == NULL) {
            ts_log(ERROR, "Failed to create TSR display");
            return -1;
        }

        pkcs7 = TS_RESP_get_token(response);
        PKCS7_print_ctx(display_bio, pkcs7, 0, NULL);
        BIO_get_mem_ptr(display_bio, &display_buf_mem);
        show_ts_response(display_buf_mem->data, display_buf_mem->length);

        BIO_free(display_bio);
        TS_RESP_free(response);
        BIO_free(ts_resp_bio);
    }

    return 0;
}

static int setup_camera(client_t* client) {
    struct v4l2_format fmt;
    struct v4l2_capability caps;
    struct v4l2_requestbuffers req;

    memset(&fmt, 0, sizeof(struct v4l2_format));
    memset(&caps, 0, sizeof(struct v4l2_capability));
    memset(&req, 0, sizeof(struct v4l2_requestbuffers));
    video_fd = open(client->video_device, O_RDWR);
    if (video_fd < 0) {
        ts_log(INFO, "Failed to open video device. Using stock images");
        video_mmap = calloc(IMAGE_HEIGHT * IMAGE_WIDTH, 1);
        return 0;
    }

    if (ioctl_wait(video_fd, VIDIOC_QUERYCAP, &caps) < 0) {
        ts_log(ERROR, "Failed to query video capabilities");
        sensor_manager_terminate();
        return -1;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = IMAGE_WIDTH;
    fmt.fmt.pix.height = IMAGE_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl_wait(video_fd, VIDIOC_S_FMT, &fmt) < 0) {
        ts_log(ERROR, "Failed to set video format");
        sensor_manager_terminate();
        return -1;
    }

    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl_wait(video_fd, VIDIOC_REQBUFS, &req) < 0) {
        ts_log(ERROR, "Failed to request video buffer");
        sensor_manager_terminate();
        return -1;
    }

    video_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_buf.memory = V4L2_MEMORY_MMAP;
    video_buf.index = 0;

    if(ioctl_wait(video_fd, VIDIOC_QUERYBUF, &video_buf) < 0) {
        ts_log(ERROR, "Failed to query video buffer");
        sensor_manager_terminate();
        return -1;
    }

    video_mmap = mmap(NULL, video_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, video_buf.m.offset);
    if (video_mmap == MAP_FAILED) {
        ts_log(ERROR, "Failed to memory map video buffer");
        sensor_manager_terminate();
        return -1;
    }

    if (ioctl_wait(video_fd, VIDIOC_STREAMON, &video_buf.type) < 0) {
        ts_log(ERROR, "Failed to start streaming I/O");
        sensor_manager_terminate();
        return -1;
    }

    return 0;
}

/* Acquire trusted timestamps */
static void *client_thread(void *arg) {
    client_t *client = (client_t *)arg;
    int idx = 0;

    proxy_request_t req = PROXY_REQUEST_INIT;
    tsa_response_t rsp = TSA_RESPONSE_INIT;

    const struct timespec ts = {
        .tv_sec = client->request_delay_ms / 1000,
        .tv_nsec = (client->request_delay_ms % 1000) * 1000000
    };

    while (gaps_running()) {
        /* Read sensor data */
        if (read_sensor(idx)) {
            ts_log(ERROR, "Failed to read sensor data");
            gaps_terminate();
            continue;
        }

        /* Save sensor data */
        if (save_sensor(client, idx)) {
            ts_log(ERROR, "Failed to save sensor data");
            gaps_terminate();
            continue;
        }

        /* Compose a request */
        if (ts_create_request_from_data(video_mmap, video_buf.length, &req) != 0) {
            ts_log(ERROR, "Failed to generate TS request");
            gaps_terminate();
            continue;
        }

        /* Send request */
        int sts = gaps_packet_write(CLIENT_TO_PROXY, &req, sizeof(req));
        if (sts == -1) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send signing request to proxy");
                gaps_terminate();
            }
            continue;
        }
        log_proxy_req(client->verbosity, "Request sent to proxy", &req);

        /* Get response */
        sts = gaps_packet_read(PROXY_TO_CLIENT, &rsp.hdr, sizeof(rsp.hdr));
        if (sts != sizeof(rsp.hdr)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive response header");
                gaps_terminate();
            }
            continue;
        }
        sts = gaps_packet_read(PROXY_TO_CLIENT, &rsp.ts, rsp.hdr.len);
        if (sts != ((int) rsp.hdr.len)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive response body");
                gaps_terminate();
            }
            continue;
        }
        log_tsa_rsp(client->verbosity, "Timestamp response received", &rsp);

        /* Optionally validate the signature */
        if (client->validate != 0) {
            if (ts_verify_data(video_mmap, video_buf.length,
                    client->ca_path, client->cert_path, &rsp) == 0) {
                ts_log(INFO, BCLR(GREEN, "Timestamp VERIFIED"));
            } else {
                ts_log(WARN, BCLR(RED, "FAILED to validate the timestamp"));
            }
            fflush(stdout);
        }

        /* Save the timestamp signature */
        if (save_ts_response(client, idx, &rsp) != 0) {
            ts_log(ERROR, "Failed to save timestamp response");
            gaps_terminate();
            continue;
        }

        if (client->request_delay_ms != 0) {
            nanosleep(&ts, NULL);
        }

        idx++;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    client_t client = {
        .verbosity = VERBOSITY_NONE,
        .validate = 0,
        .display = 1,
        .request_delay_ms = DEFAULT_REQUEST_DELAY_MS,
        .ca_path = DEFAULT_CA_PATH,
        .tsr_dir = DEFAULT_TSR_OUT_DIR,
        .video_device = DEFAULT_VIDEO_DEVICE,

        .app = {
            .threads = {
                THREAD_ADD(client_thread, &client, "ts_client"),
                THREAD_END
            },

            .on_shutdown = sensor_manager_terminate,

            .ch = {
                GAPS_CHANNEL(CLIENT_TO_PROXY, O_WRONLY, PIPE, NULL, 
                            "client->proxy"),
                GAPS_CHANNEL(PROXY_TO_CLIENT, O_RDONLY, PIPE, NULL,
                            "client<-proxy"),
                GAPS_CHANNEL_END
            }
        }
    };

    parse_args(argc, argv, &client);

    ts_log(INFO, "Starting sensor manager");

    if (client.display) {
        if (setup_camera(&client) != 0) {
            ts_log(ERROR, "Failed to setup camera");
            return -1;
        }

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
    } else {
        ts_log(INFO, "Headless mode. Using stock images");
        video_mmap = calloc(IMAGE_HEIGHT * IMAGE_WIDTH, 1);
    }

    if (gaps_app_run(&client.app) != 0) {
        ts_log(ERROR, "Failed to initialize the timestamp client");
        return -1;
    }

    return gaps_app_wait_exit(&client.app);
}
