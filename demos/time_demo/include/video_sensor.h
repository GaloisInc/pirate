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

#ifndef _VIDEO_SENSOR_H_
#define _VIDEO_SENSOR_H_

int video_sensor_initialize(const char *video_device, int display);
int video_sensor_read(int idx);
void* video_sensor_get_buffer();
unsigned int video_sensor_get_buffer_length();
void video_sensor_terminate();

#endif
