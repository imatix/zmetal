/*  =====================================================================
    zmetal client
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

#include "zapi.h"

#define REQUEST_TIMEOUT     500    //  msecs

int main (int argc, char *argv [])
{
    char *endpoint = NULL;
    if (argc > 1)
        endpoint = argv [1];
    else {
        printf ("Usage: client endpoint\n");
        exit (0);
    }
    zctx_t *ctx = zctx_new ();

    printf ("I: connecting to server at %s...\n", endpoint);
    void *control = zctx_socket_new (ctx, ZMQ_REQ);
    zmq_connect (control, endpoint);

    zmsg_t *request = zmsg_new ();
    zmsg_addstr (request, "Connection.Open");
    zmsg_addstr (request,
        "{\"protocol\":{\"name\":\"MTL\",\"version\":\"0.1\"}}");
    zmsg_send (&request, control);

    zmq_pollitem_t items [] = { { control, 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll (items, 1, REQUEST_TIMEOUT * 1000);
    if (items [0].revents & ZMQ_POLLIN) {
        zmsg_t *reply = zmsg_recv (control);
        zmsg_dump (reply);
        zmsg_destroy (&reply);
    }
    else
        printf ("E: no server present, try again later\n");

    zctx_destroy (&ctx);
    return 0;
}
