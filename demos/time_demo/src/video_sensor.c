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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include "common.h"
#include "video_sensor.h"

static int video_fd;
static struct v4l2_buffer video_buf;
static void* video_mmap;

static int ioctl_wait(int fd, unsigned long request, void *arg) {
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while ((ret == -1) && (errno == EINTR));
    return ret;
}

int video_sensor_initialize(const char *video_device, int display) {
    struct v4l2_format fmt;
    struct v4l2_capability caps;
    struct v4l2_requestbuffers req;

    if (!display) {
        video_mmap = calloc(IMAGE_HEIGHT * IMAGE_WIDTH, 1);
        return 0;
    }
    memset(&fmt, 0, sizeof(struct v4l2_format));
    memset(&caps, 0, sizeof(struct v4l2_capability));
    memset(&req, 0, sizeof(struct v4l2_requestbuffers));
    video_fd = open(video_device, O_RDWR);
    if (video_fd < 0) {
        ts_log(INFO, "Failed to open video device. Using stock images");
        video_mmap = calloc(IMAGE_HEIGHT * IMAGE_WIDTH, 1);
        return 0;
    }

    if (ioctl_wait(video_fd, VIDIOC_QUERYCAP, &caps) < 0) {
        ts_log(ERROR, "Failed to query video capabilities");
        video_sensor_terminate();
        return -1;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = IMAGE_WIDTH;
    fmt.fmt.pix.height = IMAGE_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl_wait(video_fd, VIDIOC_S_FMT, &fmt) < 0) {
        ts_log(ERROR, "Failed to set video format");
        video_sensor_terminate();
        return -1;
    }

    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl_wait(video_fd, VIDIOC_REQBUFS, &req) < 0) {
        ts_log(ERROR, "Failed to request video buffer");
        video_sensor_terminate();
        return -1;
    }

    video_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_buf.memory = V4L2_MEMORY_MMAP;
    video_buf.index = 0;

    if(ioctl_wait(video_fd, VIDIOC_QUERYBUF, &video_buf) < 0) {
        ts_log(ERROR, "Failed to query video buffer");
        video_sensor_terminate();
        return -1;
    }

    video_mmap = mmap(NULL, video_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, video_buf.m.offset);
    if (video_mmap == MAP_FAILED) {
        ts_log(ERROR, "Failed to memory map video buffer");
        video_sensor_terminate();
        return -1;
    }

    if (ioctl_wait(video_fd, VIDIOC_STREAMON, &video_buf.type) < 0) {
        ts_log(ERROR, "Failed to start streaming I/O");
        video_sensor_terminate();
        return -1;
    }

    return 0;
}

int video_sensor_read(int idx) {
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

void* video_sensor_get_buffer() {
    return video_mmap;
}

unsigned int video_sensor_get_buffer_length() {
    return video_buf.length;
}

void video_sensor_terminate() {
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
    video_fd = 0;
    video_mmap = NULL;
}
