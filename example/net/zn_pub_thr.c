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
#include <stdio.h>
#include <unistd.h>
#include "zenoh.h"

int main(int argc, char **argv)
{
    size_t len = 256;
    const char *path = "/test/thr";
    char *locator = 0;
    if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
    {
        printf("USAGE:\n\tzn_pub_thr [<payload-size>=%zu] [<path>=%s] [<locator>=auto]\n\n", len, path);
        return 0;
    }

    if (argc > 1)
    {
        len = atoi(argv[1]);
    }
    if (argc > 2)
    {
        path = argv[2];
    }
    if (argc > 3)
    {
        locator = argv[3];
    }

    printf("Running throughput test for payload of %zu bytes.\n", len);
    if (argc > 2)
    {
        locator = argv[2];
    }

    // Open a session
    zn_session_p_result_t rz = zn_open(locator, 0, 0);
    ASSERT_P_RESULT(rz, "Unable to open a session.\n");
    zn_session_t *z = rz.value.session;
    zn_start_recv_loop(z);
    zn_start_lease_loop(z);

    // Build the resource key
    zn_res_key_t rk = zn_rname(path);

    // Declare a resource
    zn_res_p_result_t rr = zn_declare_resource(z, &rk);
    ASSERT_P_RESULT(rr, "Unable to declare resource.\n");
    zn_res_t *res = rr.value.res;

    // Declare a publisher
    zn_pub_p_result_t rp = zn_declare_publisher(z, &res->key);
    ASSERT_P_RESULT(rp, "Unable to declare publisher.\n");

    // Create random data
    z_iobuf_t data = z_iobuf_make(len);
    for (unsigned int i = 0; i < len; ++i)
        z_iobuf_write(&data, i % 10);

    // Loop endessly and write data
    zn_res_key_t ri = zn_rid(res);
    while (1)
    {
        zn_write(z, &ri, data.buf, z_iobuf_readable(&data));
    }

    printf("DONE\n");
    return 0;
}
