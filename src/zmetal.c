/*  =====================================================================
    zmetal helper
    Working with MTL statuss and frames.

    TODO:
    - convert strings to cJSON arrays (profiles, mechanisms, resources)
    - convert blobs to/from base64 encoding (response, challenge)
    - implement basic mechanism
    - define structures for data flow Commands

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

#include "../include/zmetal.h"
#include "import/cJSON/cJSON.h"
#include "import/cJSON/cJSON.c"

void
send_connection_open (void *socket, char *protocol_name, int protocol_version, char *virtual_host)
{
    cJSON *root = cJSON_CreateObject ();
    cJSON *protocol = cJSON_CreateObject();
    cJSON_AddItemToObject (root, "protocol", protocol);
    cJSON_AddStringToObject (root, "virtual-host", virtual_host);
    cJSON_AddStringToObject (protocol, "name", protocol_name);
    cJSON_AddNumberToObject (protocol, "version", protocol_version);
    char *json_text = cJSON_Print (root);

    zstr_sendm (socket, "Connection.Open");
    zstr_send (socket, json_text);
    free (json_text);
    cJSON_Delete (root);
}

void
send_connection_authorize (void *socket, char *mechanism, char *response)
{
    cJSON *root = cJSON_CreateObject ();
    char *json_text = cJSON_Print (root);

    zstr_sendm (socket, "Connection.Open");
    zstr_send (socket, json_text);
    free (json_text);
    cJSON_Delete (root);
}

void
send_connection_profile (void *socket, char *profile)
{
}

void
send_connection_reader (void *socket, char *resource_list, char *confirm)
{
}

void
send_connection_writer (void *socket, char *resource_list, char *confirm)
{
}


void
send_response_200 (void *socket, mtl_request_t *request)
{
    cJSON *root = cJSON_CreateObject ();
    cJSON_AddStringToObject (root, "status", "OK");
    char *json_text = cJSON_Print (root);
    cJSON_Delete (root);

    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, request->address);
    zmsg_add (msg, request->empty);
    zmsg_addstr (msg, "200");
    zmsg_addstr (msg, json_text);
    zmsg_send (&msg, socket);
    free (json_text);
    request->address = NULL;
    request->empty = NULL;
}

void
send_response_201 (void *socket, mtl_request_t *request,
                   char *profile_list)
{
    cJSON *root = cJSON_CreateObject ();
    cJSON_AddStringToObject (root, "status", "Ready");
    cJSON *profiles = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "profiles", profiles);
    cJSON_AddStringToObject (profiles, "", profile_list);
    char *json_text = cJSON_Print (root);
    cJSON_Delete (root);

    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, request->address);
    zmsg_add (msg, request->empty);
    zmsg_addstr (msg, "201");
    zmsg_addstr (msg, json_text);
    zmsg_send (&msg, socket);
    free (json_text);
    request->address = NULL;
    request->empty = NULL;
}

void
send_response_202 (void *socket, mtl_request_t *request,
                   int port, char *lease)
{
    cJSON *root = cJSON_CreateObject ();
    cJSON_AddStringToObject (root, "status", "Data Lease");
    cJSON_AddNumberToObject (root, "port", port);
    cJSON_AddStringToObject (root, "lease", lease);
    char *json_text = cJSON_Print (root);
    cJSON_Delete (root);

    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, request->address);
    zmsg_add (msg, request->empty);
    zmsg_addstr (msg, "202");
    zmsg_addstr (msg, json_text);
    zmsg_send (&msg, socket);
    free (json_text);
    request->address = NULL;
    request->empty = NULL;
}

void
send_response_401 (void *socket, mtl_request_t *request,
                   char *mechanism_list)
{
}

void
send_response_402 (void *socket, mtl_request_t *request,
                   char *mechanism, char *challenge)
{
}

void
send_response_501 (void *socket, mtl_request_t *request)
{
}


cJSON *
s_json_locate (cJSON *root, char *path)
{
    //  Calculate significant length of name
    char *slash = strchr (path, '/');
    int length = strlen (path);
    if (slash)
        length = slash - path;

    //  Find matching name starting at first child of root
    cJSON *child = root->child;
    while (child) {
        if (strlen (child->string) == length
        &&  memcmp (child->string, path, length) == 0) {
            if (slash)          //  Look deeper
                return s_json_locate (child, slash + 1);
            else
                return child;
        }
        child = child->next;
    }
    return NULL;
}

char *
s_json_str (cJSON *root, char *path)
{
    cJSON *item = s_json_locate (root, path);
    if (item && item->type == cJSON_String) {
        printf ("FOUND: %s=%s\n", path, item->valuestring);
        return strdup (item->valuestring);
    }
    else {
        printf ("MISSING: %s\n", path);
        return strdup ("");
    }
}

int
s_json_int (cJSON *root, char *path)
{
    cJSON *item = s_json_locate (root, path);
    if (item && item->type == cJSON_Number)
        return item->valueint;
    else
        return 0;
}


//  Turn a zmsg_t object into an mtl_request object

mtl_request_t *
mtl_request_new (zmsg_t *msg)
{
    //  Reject badly framed messages
    mtl_request_t *self = zmalloc (sizeof (mtl_request_t));

    self->address = zmsg_pop (msg);
    self->empty = zmsg_pop (msg);
    self->command = strlwc (zmsg_popstr (msg));
    self->json = zmsg_popstr (msg);
    cJSON *root = cJSON_Parse (self->json);

    if (streq (self->command, "connection.open")) {
        self->payload.connection_open.protocol_name
            = s_json_str (root, "protocol/name");
        self->payload.connection_open.protocol_version
            = s_json_int (root, "protocol/version");
        self->payload.connection_open.virtual_host
            = s_json_str (root, "virtual-host");
    }
    else
    if (streq (self->command, "connection.authorize")) {
        self->payload.connection_authorize.mechanism
            = s_json_str (root, "mechanism");
#if 0
        self->payload.connection_authorize.response_data
            = s_json_blob_data (root, "response");
        self->payload.connection_authorize.response_size
            = s_json_blob_size (root, "response");
#endif
    }
    else
    if (streq (self->command, "connection.profile")) {
        self->payload.connection_profile.profile
            = s_json_str (root, "profile");
    }
    else
    if (streq (self->command, "connection.reader")) {
#if 0
        self->payload.connection_reader.resourcec
            = s_json_array_count (root, "resource");
        self->payload.connection_reader.resourcev
            = s_json_array_value (root, "resource");
#endif
        self->payload.connection_reader.confirm
            = s_json_str (root, "confirm");
    }
    else
    if (streq (self->command, "connection.writer")) {
#if 0
        self->payload.connection_writer.resourcec
            = s_json_array_count (root, "resource");
        self->payload.connection_writer.resourcev
            = s_json_array_value (root, "resource");
#endif
        self->payload.connection_writer.confirm
            = s_json_str (root, "confirm");
    }
    else {
        printf ("E: invalid command: %s\n", self->command);
        mtl_request_destroy (&self);
    }
    cJSON_Delete (root);
    return self;
}

void
mtl_request_destroy (mtl_request_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mtl_request_t *self = *self_p;
        zframe_destroy (&self->address);
        zframe_destroy (&self->empty);
        if (streq (self->command, "connection.open")) {
            free (self->payload.connection_open.protocol_name);
            free (self->payload.connection_open.virtual_host);
        }
        else
        if (streq (self->command, "connection.authorize")) {
            free (self->payload.connection_authorize.mechanism);
        }
        else
        if (streq (self->command, "connection.profile")) {
            free (self->payload.connection_profile.profile);
        }
        else
        if (streq (self->command, "connection.reader")) {
            free (self->payload.connection_reader.confirm);
        }
        else
        if (streq (self->command, "connection.writer")) {
            free (self->payload.connection_writer.confirm);
        }
        free (self->command);
        free (self->json);
        free (self);
        *self_p = NULL;
    }
}

//  Functions to read and parse requests and responses

mtl_request_t *
mtl_request_recv (void *socket, int timeout)
{
    mtl_request_t *request = NULL;
    zmq_pollitem_t items [] = { { socket, 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll (items, 1, timeout * 1000);
    if (items [0].revents & ZMQ_POLLIN) {
        zmsg_t *msg = zmsg_recv (socket);
        zmsg_dump (msg);
        if (zmsg_size (msg) == 4)
            request = mtl_request_new (msg);
        else
            printf ("E: badly-framed request, expect 2 frames\n");
        zmsg_destroy (&msg);
    }
    return request;
}

mtl_response_t *
mtl_response_new (zmsg_t *msg)
{
    mtl_response_t *self = zmalloc (sizeof (mtl_response_t));

    self->code = zmsg_popstr (msg);
    self->json = zmsg_popstr (msg);
    cJSON *root = cJSON_Parse (self->json);

    if (streq (self->code, "200")) {
        self->payload.response_200.status
            = s_json_str (root, "status");
    }
    else
    if (streq (self->code, "201")) {
        self->payload.response_201.status
            = s_json_str (root, "status");
#if 0
        self->payload.response_201.profilec
            = s_json_array_count (root, "profile");
        self->payload.response_201.profilev
            = s_json_array_value (root, "profile");
#endif
    }
    else
    if (streq (self->code, "202")) {
        self->payload.response_202.status
            = s_json_str (root, "status");
        self->payload.response_202.port
            = s_json_int (root, "port");
        self->payload.response_202.lease
            = s_json_str (root, "lease");
    }
    else
    if (streq (self->code, "401")) {
        self->payload.response_401.status
            = s_json_str (root, "status");
#if 0
        self->payload.response_401.mechanismc
            = s_json_array_count (root, "mechanism");
        self->payload.response_401.mechanismv
            = s_json_array_value (root, "mechanism");
#endif
    }
    else
    if (streq (self->code, "402")) {
        self->payload.response_402.status
            = s_json_str (root, "status");
        self->payload.response_402.mechanism
            = s_json_str (root, "mechanism");
#if 0
        self->payload.response_402.challenge_data
            = s_json_blob_data (root, "challenge");
        self->payload.response_402.challenge_size
            = s_json_blob_size (root, "challenge");
#endif
    }
    else
    if (streq (self->code, "501")) {
        self->payload.response_501.status
            = s_json_str (root, "status");
    }
    else {
        printf ("E: invalid code: %s\n", self->code);
        mtl_response_destroy (&self);
    }
    cJSON_Delete (root);
    return self;
}

void
mtl_response_destroy (mtl_response_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mtl_response_t *self = *self_p;
        if (streq (self->code, "200")) {
            free (self->payload.response_200.status);
        }
        else
        if (streq (self->code, "201")) {
            free (self->payload.response_201.status);
        }
        else
        if (streq (self->code, "202")) {
            free (self->payload.response_202.status);
            free (self->payload.response_202.lease);
        }
        else
        if (streq (self->code, "401")) {
            free (self->payload.response_401.status);
        }
        else
        if (streq (self->code, "402")) {
            free (self->payload.response_402.status);
            free (self->payload.response_402.mechanism);
        }
        else
        if (streq (self->code, "501")) {
            free (self->payload.response_501.status);
        }
        free (self->code);
        free (self->json);
        free (self);
        *self_p = NULL;
    }
}

mtl_response_t *
mtl_response_recv (void *socket, int timeout)
{
    mtl_response_t *response = NULL;
    zmq_pollitem_t items [] = { { socket, 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll (items, 1, timeout * 1000);
    if (items [0].revents & ZMQ_POLLIN) {
        zmsg_t *msg = zmsg_recv (socket);
        zmsg_dump (msg);
        if (zmsg_size (msg) == 2)
            response = mtl_response_new (msg);
        else
            printf ("E: badly-framed response, expect 2 frames\n");
        zmsg_destroy (&msg);
    }
    return response;
}

//  Helper function to lower-case a string

char *strlwc (char *string)
{
    uint size = strlen (string);
    while (size) {
        size--;
        string [size] = tolower (string [size]);
    }
    return string;
}