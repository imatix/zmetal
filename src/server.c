/*  =====================================================================
    zmetal server
    An implementation of http://rfc.zeromq.org/spec:11

    ---------------------------------------------------------------------
    Copyright (c) 1991-2011 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the zmetal project: http://zmetal.zeromq.org

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =====================================================================
*/

#include "zmetal.c"

int main (int argc, char *argv [])
{
    char *endpoint = NULL;
    if (argc > 1)
        endpoint = argv [1];
    else {
        printf ("Usage: server endpoint\n");
        exit (0);
    }
    zctx_t *ctx = zctx_new ();

    printf ("I: server ready at %s...\n", endpoint);
    void *client = zctx_socket_new (ctx, ZMQ_ROUTER);
    int rc = zmq_bind (client, endpoint);
    assert (rc == 0);

    while (TRUE) {
        mtl_request_t *request = mtl_request_recv (client, -1);
        if (!request)
            break;              //  Interrupted
        if (streq (request->command, "connection.open")) {
            send_response_201 (client, request, "test");
        }
        mtl_request_destroy (&request);
    }
    zctx_destroy (&ctx);
    return 0;
}
