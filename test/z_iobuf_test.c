/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "zenoh/iobuf.h"

#define RUNS 1000

/*=============================*/
/*     Printing functions      */
/*=============================*/
void print_rbuf_overview(z_rbuf_t *rbf)
{
    printf("    RBuf => Capacity: %zu\n", rbf->ios.capacity);
}

void print_wbuf_overview(z_wbuf_t *wbf)
{
    printf("    WBuf => Expandable: %u, Capacity: %zu\n", wbf->is_expandable, wbf->capacity);
}

void print_iosli(z_iosli_t *ios)
{
    printf("IOSli: Capacity: %zu, Rpos: %zu, Wpos: %zu, Buffer: [ ", ios->capacity, ios->r_pos, ios->w_pos);
    for (size_t i = 0; i < ios->capacity; ++i)
    {
        printf("%02x", ios->buf[i]);
        if (i < ios->capacity - 1)
            printf(" ");
    }
    printf(" ]");
}

/*=============================*/
/*    Generating functions     */
/*=============================*/
int gen_bool()
{
    return rand() % 2;
}

uint8_t gen_uint8()
{
    return (uint8_t)rand() % 255;
}

size_t gen_size_t()
{
    return (size_t)rand();
}

z_rbuf_t gen_rbuf(size_t len)
{
    return z_rbuf_make(len);
}

z_wbuf_t gen_wbuf(size_t len)
{
    int is_expandable = 0;

    if (gen_bool())
    {
        is_expandable = 1;
        len = 1 + (gen_size_t() % len);
    }

    return z_wbuf_make(len, is_expandable);
}

/*=============================*/
/*           Tests             */
/*=============================*/
void rbuf_writable_readable()
{
    size_t len = 128;
    z_rbuf_t rbf = z_rbuf_make(len);
    printf("\n>>> RBuf => Writable and Readable\n");

    size_t writable = z_rbuf_space_left(&rbf);
    printf("    Writable: %zu\n", writable);
    assert(writable == len);

    size_t readable = z_rbuf_len(&rbf);
    printf("    Readable: %zu\n", readable);
    assert(readable == 0);

    z_rbuf_set_wpos(&rbf, len);

    size_t read = 0;
    while (read < len)
    {
        size_t to_read = 1 + gen_size_t() % (len - read);
        for (size_t i = 0; i < to_read; i++)
        {
            z_rbuf_read(&rbf);
        }
        read += to_read;

        writable = z_rbuf_space_left(&rbf);
        printf("    Writable: %zu\n", writable);
        assert(writable == 0);

        readable = z_rbuf_len(&rbf);
        printf("    Readable: %zu\n", readable);
        assert(readable == len - read);
    }
}

void wbuf_writable_readable()
{
    size_t len = 128;
    z_wbuf_t wbf = z_wbuf_make(len, 0);
    printf("\n>>> WBuf => Writable and Readable\n");

    size_t writable = z_wbuf_space_left(&wbf);
    printf("    Writable: %zu\n", writable);
    assert(writable == len);

    size_t readable = z_wbuf_len(&wbf);
    printf("    Readable: %zu\n", readable);
    assert(readable == 0);

    size_t written = 0;
    while (written < len)
    {
        size_t to_write = 1 + gen_size_t() % (len - written);
        for (size_t i = 0; i < to_write; i++)
        {
            z_wbuf_write(&wbf, 0);
        }
        written += to_write;

        writable = z_wbuf_space_left(&wbf);
        printf("    Writable: %zu\n", writable);
        assert(writable == len - written);

        readable = z_wbuf_len(&wbf);
        printf("    Readable: %zu\n", readable);
        assert(readable == written);
    }
}

void wbuf_write_rbuf_read()
{
    size_t len = 128;
    z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => Write and Read\n");
    print_wbuf_overview(&wbf);
    printf("    Writing %zu bytes\n", len);
    for (size_t i = 0; i < len; i++)
    {
        z_wbuf_write(&wbf, (uint8_t)i % 255);
    }

    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", z_wbuf_len_iosli(&wbf), wbf.r_idx, wbf.w_idx);
    printf("    Written: %zu, Readable: %zu\n", len, z_wbuf_len(&wbf));
    assert(z_wbuf_len(&wbf) == len);

    z_rbuf_t rbf = z_wbuf_to_rbuf(&wbf);
    printf("    Reading %zu bytes\n", len);
    printf("    [");
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t l = (uint8_t)i % 255;
        uint8_t r = z_rbuf_read(&rbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf("]\n");

    z_rbuf_free(&rbf);
    z_wbuf_free(&wbf);
}

void wbuf_write_rbuf_read_bytes()
{
    size_t len = 128;
    z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => Write and Read bytes\n");
    print_wbuf_overview(&wbf);

    printf("    Writing %zu bytes\n", len);
    uint8_t *buf01 = (uint8_t *)malloc(len * sizeof(uint8_t));
    for (size_t i = 0; i < len; i++)
    {
        buf01[i] = (uint8_t)i % 255;
    }
    z_wbuf_write_bytes(&wbf, buf01, 0, len);

    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", z_wbuf_len_iosli(&wbf), wbf.r_idx, wbf.w_idx);
    printf("    Written: %zu, Readable: %zu\n", len, z_wbuf_len(&wbf));
    assert(z_wbuf_len(&wbf) == len);

    z_rbuf_t rbf = z_wbuf_to_rbuf(&wbf);
    printf("    [");
    for (size_t i = 0; i < len; i++)
    {
        uint8_t l = buf01[i];
        uint8_t r = z_rbuf_read(&rbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf(" ]\n");

    free(buf01);
    z_rbuf_free(&rbf);
    z_wbuf_free(&wbf);
}

void wbuf_put_rbuf_get()
{
    size_t len = 128;
    z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => Put and Get\n");
    print_wbuf_overview(&wbf);
    // Initialize to 0
    for (size_t i = 0; i < len; i++)
    {
        z_wbuf_write(&wbf, 0);
    }
    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", z_wbuf_len_iosli(&wbf), wbf.r_idx, wbf.w_idx);

    printf("    Putting %zu bytes\n", len);
    // Put data
    for (size_t i = 0; i < len; i++)
    {
        z_wbuf_put(&wbf, (uint8_t)i % 255, i);
    }

    z_rbuf_t rbf = z_wbuf_to_rbuf(&wbf);
    printf("    Getting %zu bytes\n", len);
    printf("    [");
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t l = (uint8_t)i % 255;
        uint8_t r = z_rbuf_read(&rbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf(" ]\n");

    z_rbuf_free(&rbf);
    z_wbuf_free(&wbf);
}

void wbuf_set_pos_wbuf_get_pos()
{
    size_t len = 128;
    z_wbuf_t wbf = gen_wbuf(len);
    printf("\n>>> WBuf => SetPos and GetPos\n");
    print_wbuf_overview(&wbf);
    // Initialize to 0
    for (size_t i = 0; i < len; i++)
    {
        z_wbuf_write(&wbf, 0);
    }
    assert(z_wbuf_get_rpos(&wbf) == 0);
    assert(z_wbuf_get_wpos(&wbf) == len);
    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", z_wbuf_len_iosli(&wbf), wbf.r_idx, wbf.w_idx);

    for (size_t i = 0; i < 10; i++)
    {
        z_wbuf_clear(&wbf);
        assert(z_wbuf_get_rpos(&wbf) == 0);
        assert(z_wbuf_get_wpos(&wbf) == 0);

        size_t lw_pos = gen_size_t() % (len + 1);
        printf("    Setting WPos: %zu\n", lw_pos);
        z_wbuf_set_wpos(&wbf, lw_pos);

        size_t rw_pos = z_wbuf_get_wpos(&wbf);
        printf("    Getting WPos: %zu\n", rw_pos);
        assert(lw_pos == rw_pos);

        size_t lr_pos = gen_size_t() % (lw_pos + 1);
        printf("    Setting RPos: %zu\n", lr_pos);
        z_wbuf_set_rpos(&wbf, lr_pos);

        size_t rr_pos = z_wbuf_get_rpos(&wbf);
        printf("    Getting RPos: %zu\n", rr_pos);
        assert(lr_pos == rr_pos);
    }

    z_wbuf_free(&wbf);
}

void wbuf_add_iosli()
{
    uint8_t len = 16;
    z_wbuf_t wbf = z_wbuf_make(len, 1);
    printf("\n>>> WBuf => Add IOSli\n");
    print_wbuf_overview(&wbf);

    size_t written = 0;
    uint8_t counter = 0;
    while (written < 255)
    {
        uint8_t remaining = 255 - written;
        uint8_t range = remaining < len ? remaining : len;
        uint8_t to_write = 1 + gen_uint8() % range;
        if (gen_bool())
        {
            printf("    Writing %u bytes\n", to_write);
            for (uint8_t i = 0; i < to_write; i++)
            {
                z_wbuf_write(&wbf, counter);
                counter++;
            }
        }
        else
        {
            z_iosli_t *ios = (z_iosli_t *)malloc(sizeof(z_iosli_t));
            ios->r_pos = 0;
            ios->w_pos = 0;
            ios->capacity = to_write;
            ios->buf = (uint8_t *)malloc(to_write);

            for (uint8_t i = 0; i < to_write; i++)
            {
                z_iosli_write(ios, counter);
                counter++;
            }
            printf("    Adding IOSli with %u bytes\n", to_write);
            z_wbuf_add_iosli(&wbf, ios);
        }
        written += to_write;
    }

    printf("    IOSlices: %zu, RIdx: %zu, WIdx: %zu\n", z_wbuf_len_iosli(&wbf), wbf.r_idx, wbf.w_idx);
    for (size_t i = 0; i < z_wbuf_len_iosli(&wbf); i++)
    {
        z_iosli_t *ios = z_wbuf_get_iosli(&wbf, i);
        printf("    Idx: %zu => ", i);
        print_iosli(ios);
        printf("\n");
    }

    z_rbuf_t rbf = z_wbuf_to_rbuf(&wbf);
    printf("    [");
    for (uint8_t i = 0; i < counter; i++)
    {
        uint8_t l = i;
        uint8_t r = z_rbuf_read(&rbf);
        printf(" %02x:%02x", l, r);
        assert(l == r);
    }
    printf(" ]\n");

    z_rbuf_free(&rbf);
    z_wbuf_free(&wbf);
}

/*=============================*/
/*            Main             */
/*=============================*/
int main()
{
    for (unsigned int i = 0; i < RUNS; ++i)
    {
        printf("\n\n== RUN %u\n", i);
        rbuf_writable_readable();
        wbuf_writable_readable();
        wbuf_write_rbuf_read();
        wbuf_write_rbuf_read_bytes();
        wbuf_put_rbuf_get();
        wbuf_set_pos_wbuf_get_pos();
        wbuf_add_iosli();
    }
}
