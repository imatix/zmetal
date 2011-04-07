/*  =====================================================================
    zmetal header file
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

#ifndef __ZMETAL_H_INCLUDED__
#define __ZMETAL_H_INCLUDED__

#include "zapi.h"

#ifdef __cplusplus
extern "C" {
#endif

//  ---------------------------------------------------------------------
//  Client-side API
//  ---------------------------------------------------------------------

typedef struct {
    char *status;       //  200 OK
} response_200_t;

typedef struct {
    char *status;       //  201 Ready
    char *profile_list;
} response_201_t;

typedef struct {
    char *status;       //  202 Data Lease
    int port;
    char *lease;
} response_202_t;

typedef struct {
    char *status;       //  401 Unauthorized
    char *mechanism_list;
} response_401_t;

typedef struct {        //  401 Challenged
    char *status;
    char *mechanism;
    char *challenge;
} response_402_t;

typedef struct {        //  501 Not Implemented
    char *status;
} response_501_t;

//  These is a server response as read off the wire in the client

typedef struct {
    char *code;
    char *json;
    union {
        response_200_t response_200;
        response_201_t response_201;
        response_202_t response_202;
        response_401_t response_401;
        response_402_t response_402;
        response_501_t response_501;
    } payload;
} mtl_response_t;

void send_connection_open (void *socket, char *protocol_name,
                           int protocol_version, char *virtual_host);
void send_connection_authorize (void *socket, char *mechanism, char *response);
void send_connection_profile (void *socket, char *profile);
void send_connection_reader (void *socket, char *resource_list, char *confirms);
void send_connection_writer (void *socket, char *resource_list, char *confirms);

mtl_response_t *
    mtl_response_new (zmsg_t *response);
void
    mtl_response_destroy (mtl_response_t **self_p);
mtl_response_t *
    mtl_response_recv (void *socket, int timeout);


//  ---------------------------------------------------------------------
//  Server-side API
//  ---------------------------------------------------------------------

typedef struct {
    char *protocol_name;
    int protocol_version;
    char *virtual_host;
} connection_open_t;

typedef struct {
    char *mechanism;
    char *response;
} connection_authorize_t;

typedef struct {
    char *profile;
} connection_profile_t;

typedef struct {
    char *resource_list;
    char *confirm;
} connection_reader_t;

typedef struct {
    char *resource_list;
    char *confirm;
} connection_writer_t;

//  These is a client request as read off the wire in the server

typedef struct {
    zframe_t *address;
    zframe_t *empty;
    char *command;
    char *json;
    union {
        connection_open_t
            connection_open;
        connection_authorize_t
            connection_authorize;
        connection_profile_t
            connection_profile;
        connection_reader_t
            connection_reader;
        connection_writer_t
            connection_writer;
    } payload;
} mtl_request_t;

void send_response_200 (void *socket, mtl_request_t *request);
void send_response_201 (void *socket, mtl_request_t *request,
                        char *profile_list);
void send_response_202 (void *socket, mtl_request_t *request,
                        int port, char *lease);
void send_response_401 (void *socket, mtl_request_t *request,
                        char *mechanism_list);
void send_response_402 (void *socket, mtl_request_t *request,
                        char *mechanism, char *challenge);
void send_response_501 (void *socket, mtl_request_t *request);

mtl_request_t *
    mtl_request_new (zmsg_t *request);
void
    mtl_request_destroy (mtl_request_t **self_p);
mtl_request_t *
    mtl_request_recv (void *socket, int timeout);


//  ---------------------------------------------------------------------
//  Random helper functions
//  ---------------------------------------------------------------------

char *strlwc (char *string);

#ifdef __cplusplus
}
#endif

#endif
