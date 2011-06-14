/* 
 * Spidermonkey Socket Module
 * Based on mod_socket module by:
 * Copyright (C) 2007, Jonas Gauffin <jonas.gauffin@gmail.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is
 * Jonas Gauffin
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Jonas Gauffin <jonas.gauffin@gmail.com>
 *
 *
 * mod_spidermonkey_js_sock.c -- js_sock Javascript Module
 *
 * 2010 Simon Vogl <simon@beko.at> - hefty modifications, bugfixes, added poll() and select statements.
 */
//#include "mod_spidermonkey.h"
#include <unistd.h>   
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <poll.h>

#include <jsapi.h>

// forward declare the class def:
extern JSClass js_sock_class;

enum sockstate { disconnected, connected, listening };

struct sock_obj {
	int sock;
//	switch_js_sock_t *js_sock;
//	apr_pool_t *pool;
	char *read_buffer;
	int buffer_size;
	int buffer_pos;
	sockstate state;
	jsrefcount saveDepth;
};
typedef struct sock_obj sock_obj_t;


/* js_sock Object */
/*********************************************************************************/
static JSBool js_sock_construct(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock_obj = 0;

	js_sock_obj = (sock_obj_t*)JS_malloc( cx, sizeof(sock_obj_t));

	js_sock_obj->read_buffer = (char*)JS_malloc(cx, 1024);
	js_sock_obj->buffer_size = 1024;
	js_sock_obj->buffer_pos = 0;
	js_sock_obj->sock = -1;
	js_sock_obj->state = disconnected;
	JS_SetPrivate(cx, obj, js_sock_obj);

	// enables us to listen on stdin etc...:
	if (argc==1) {
		if (JSVAL_IS_INT(argv[0])) {
			js_sock_obj->sock = JSVAL_TO_INT(argv[0]);
			js_sock_obj->state = connected;
		}
	}
	return JS_TRUE;
}

static void js_sock_destroy(JSContext * cx, JSObject * obj)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	if (js_sock == NULL)
		return;
	if (js_sock->sock>=0) { 
		close(js_sock->sock); 
	}
	JS_free(cx, js_sock);
}

static void getInfo(void)
{
}

static JSBool js_sock_connect(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	if (js_sock == NULL) {
		printf("Failed to find js object.\n");
		return JS_FALSE;
	}
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if (argc == 2) {
		char *host = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		char portnum[8];
		int32 port;
        struct addrinfo hints;
		struct addrinfo *result, *rp;
		int ret, sfd;

		JS_ValueToInt32(cx, argv[1], &port);
		sprintf(portnum,"%d",port); // HACK

		if (js_sock->sock != -1) {
			close(js_sock->sock);
			js_sock->sock=-1;
		}

       memset(&hints, 0, sizeof(struct addrinfo));
       hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
       hints.ai_socktype = SOCK_STREAM; /* tcp js_sock */
       hints.ai_flags = 0;
       hints.ai_protocol = 0;          /* Any protocol */

       ret = getaddrinfo(host, portnum, &hints, &result);
       if (ret != 0) {
			fprintf(stderr, "getaddrinfo failed\n");
			return JS_TRUE;
       }

		printf( "Connecting to: %s:%d.\n", host, port);
		js_sock->saveDepth = JS_SuspendRequest(cx);

		for (rp = result; rp != NULL; rp = rp->ai_next) {

		   sfd = socket(rp->ai_family, rp->ai_socktype,  rp->ai_protocol);
		   if (sfd == -1)
			   continue;

		   if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			   break;                  /* Success */

		   close(sfd);
		}

		if (rp == NULL) {               /* No address succeeded */
			JS_ReportError(cx, "Could not connect - no address found.\n");
			return JS_FALSE;
		}
		js_sock->sock = sfd;
		js_sock->state = connected;
		
		freeaddrinfo(result);     

		JS_ResumeRequest(cx, js_sock->saveDepth);
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}
	return JS_TRUE;
}

static JSBool js_sock_send(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	if (js_sock == NULL) {
		printf("Failed to find js object.\n");
		return JS_FALSE;
	}

	if (argc == 1) {
		char *buffer = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		int ret, len = strlen(buffer);
		js_sock->saveDepth = JS_SuspendRequest(cx);

		ret = write(js_sock->sock, buffer, len);

		JS_ResumeRequest(cx, js_sock->saveDepth);

		if (ret==len) {
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		} else 
		if (ret<0) { // exception on error
			JS_ReportError( cx, "send failed: %d %s.\n", errno, strerror(errno));
			return JS_FALSE;
		} else {
			printf( "switch_js_sock_send failed: %d.\n", ret);
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		} 
	}

	return JS_TRUE;
}

static JSBool js_sock_read_bytes(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	if (js_sock == NULL) {
		printf( "Failed to find js object.\n");
		return JS_FALSE;
	}
	errno=0;

	if (argc == 1) {
		int32 bytes_to_read;
		int ret;
		size_t len;

		JS_ValueToInt32(cx, argv[0], &bytes_to_read);
		len = (size_t) bytes_to_read;

		if (js_sock->buffer_pos>0 ) { // maybe we've got some old data...
			len -= js_sock->buffer_pos;
		}
		if (js_sock->buffer_size < bytes_to_read) {
			js_sock->read_buffer = (char*)JS_realloc(cx, js_sock->read_buffer, js_sock->buffer_size+bytes_to_read + 1024);
			js_sock->buffer_size += bytes_to_read + 1;
		}

		js_sock->saveDepth = JS_SuspendRequest(cx);

		if ( js_sock->buffer_pos >= bytes_to_read ) {
			ret = bytes_to_read;
		} else {
			ret = read(js_sock->sock, js_sock->read_buffer + js_sock->buffer_pos, len);
		}
		JS_ResumeRequest(cx, js_sock->saveDepth);

		if (ret < bytes_to_read ) {
			printf( "read_bytes failed: %d %s.\n", ret, strerror(errno));
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		} else {
			js_sock->read_buffer[bytes_to_read] = 0;
			jsval vec[bytes_to_read];
			for (int i=0;i<bytes_to_read;i++) {
				vec[i] = INT_TO_JSVAL((unsigned char)js_sock->read_buffer[i]);
			}
			JSObject* arr = JS_NewArrayObject(cx, bytes_to_read, vec);

			*rval = OBJECT_TO_JSVAL(arr);

			memmove(js_sock->read_buffer, 
					js_sock->read_buffer+bytes_to_read, 
					js_sock->buffer_pos-bytes_to_read);

			js_sock->buffer_pos -=bytes_to_read;
		}
	}

	return JS_TRUE;
}


static JSBool js_sock_wait_for_input(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	int timeout=-1;

	fd_set rfds;
	struct timeval tv;
	int ret;

	if (js_sock == NULL) {
		JS_ReportError(cx, "object error - no private data!" );
		return JS_FALSE;
	}

	if (js_sock->sock<0) {
			JS_ReportError(cx, "socket is closed!" );
			return JS_FALSE;
	}

	if (argc!=1) {
		JS_ReportError(cx, "pass an argument to wait_for_input!" );
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(argv[0])) {
		timeout = JSVAL_TO_INT(argv[0]);
	}

	if (timeout>0) {
		FD_ZERO(&rfds);
		FD_SET(js_sock->sock, &rfds);

        /* Wait up to five seconds. */
        tv.tv_sec = timeout/1000;
		timeout -= (tv.tv_sec)*1000;
        tv.tv_usec = timeout*1000;

		ret = select(js_sock->sock+1, &rfds, NULL,NULL, &tv);

		if (ret == -1) {
			fprintf(stderr, "select failed code: %s", strerror(errno) );
			JS_ReportError(cx, "select failed code: %s", strerror(errno) );
			return JS_FALSE;
		}
		if ( FD_ISSET(js_sock->sock, &rfds)) {
			// no data available...
			*rval = JSVAL_TRUE;
		} else {
			*rval = JSVAL_FALSE;
		}
	}
	return JS_TRUE;
}


#define LISTEN_BACKLOG 10

static JSBool js_sock_bind(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;
	char buf[8];

	if (argc != 1 
		|| ! JSVAL_IS_INT(argv[0]) ) {
			JS_ReportError(cx, "pass the port# to listen on!\n" );
			return JS_FALSE;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	sprintf(buf, "%d", JSVAL_TO_INT(argv[0]));
	s = getaddrinfo(NULL, buf, &hints, &result);
	if (s != 0) {
		JS_ReportError(cx, "getaddrinfo: %s\n", gai_strerror(s) );
		return JS_FALSE;
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		char one;
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
//		setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));

		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;                  /* Success */

		close(sfd);
	}
	freeaddrinfo(result);           /* No longer needed */
	if (rp == NULL) {               /* No address succeeded */
		JS_ReportError(cx, "Could not bind\n" );
		return JS_FALSE;
	}

	if (listen(sfd, LISTEN_BACKLOG) == -1) {
		JS_ReportError(cx, "Could not listen %s\n", strerror(errno) );
		return JS_FALSE;
	}

	js_sock->sock = sfd;
	js_sock->state = listening;

	return JS_TRUE;
}

static JSBool js_sock_accept(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	sock_obj_t *client_sock;
	struct sockaddr peer_addr;
	socklen_t peer_addr_size = sizeof(struct sockaddr_in);
	int ret;

	int cfd = accept(js_sock->sock, (struct sockaddr *) &peer_addr, &peer_addr_size);
	if (cfd == -1) {
		JS_ReportError(cx, "accept failed: %d.\n", strerror(errno) );
		return JS_FALSE;
	}
	printf("addr sz %d sid %d\n", peer_addr_size, cfd);

	JSObject * newSock = JS_ConstructObject(cx, &js_sock_class,NULL,NULL);
	client_sock = (sock_obj_t*)JS_GetPrivate(cx, newSock);
	client_sock->sock = cfd;
	client_sock->state = connected;

	*rval = OBJECT_TO_JSVAL(newSock);
	return JS_TRUE;
}

static JSBool js_sock_poll(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	int timeout=-1;

	fd_set rfds;
	struct timeval tv;
	int ret;

	if (js_sock == NULL) {
		JS_ReportError(cx, "Failed to find js object.\n" );
		return JS_FALSE;
	}

	if (js_sock->sock<0) {
			JS_ReportError(cx, "socket is closed!" );
			return JS_FALSE;
	}

	if (argc!=2
		|| ! JSVAL_IS_OBJECT(argv[0])
		|| ! JSVAL_IS_INT(argv[1])
		|| ! JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[0])) ) {
		JS_ReportError(cx, "pass an array of Sockets + timeout in ms!" );
		return JS_FALSE;
	}

	JSObject* socks =  JSVAL_TO_OBJECT(argv[0]);
	unsigned int len,i;

	timeout = JSVAL_TO_INT(argv[1]);
	JS_GetArrayLength(cx, socks, &len);

	struct pollfd fds[len];
	JSObject* sockArray[len];
	for (i=0;i<len;i++) {
		JSObject* theSock;
		jsval val;
		jsid id;

		JS_ValueToId(cx,INT_TO_JSVAL(i), &id);

		ret = JS_GetPropertyById(cx, socks, id, &val);

		if (! JSVAL_IS_OBJECT(val)) {
			JS_ReportError(cx, "array element %d is not an object!", i);
			return JS_FALSE;
		}
		theSock = JSVAL_TO_OBJECT(val);
		sockArray[i] = theSock;
		JSClass* theSockClass = JS_GET_CLASS(cx, theSock);
		if (&js_sock_class != theSockClass) {
			JS_ReportError(cx, "Works with socket elements only!", i);
			return JS_FALSE;
		}
		sock_obj_t *sock_data = (sock_obj_t *)JS_GetPrivate(cx, theSock);
		if (sock_data->sock==-1) {
			fprintf(stderr, "socket idx %d closed - IGNORING!\n",i);
			len--;
			continue;
		}
		fds[i].fd = sock_data->sock;
		fds[i].events= POLLIN | POLLPRI | POLLRDHUP | POLLRDNORM ;
		fds[i].revents=0;
	}

	poll(fds, len, timeout);

	for (i=0;i<len;i++) {
		jsval f_argv;
		jsval f_ret;
		int ret;
		if ( fds[i].revents & (POLLERR |POLLHUP |POLLNVAL|POLLRDHUP) ) {
			f_argv = INT_TO_JSVAL(fds[i].revents);
			ret = JS_CallFunctionName(cx, sockArray[i], "error", 1, &f_argv, &f_ret);
			if (ret == JS_FALSE) {
				JS_ReportError(cx, "function call returned false!", i);
				//return ret;
			}
		}
		if ( fds[i].revents & POLLIN ) {
			ret = JS_CallFunctionName(cx, sockArray[i], "data", 0, &f_argv, &f_ret);
			if (ret == JS_FALSE) {
				JS_ReportError(cx, "function call II returned false!", i);
				return ret;
			}
		}
	}

	return JS_TRUE;
}

/** read a websocket packet - framed by 0x00 ..[utf8 string data].. 0xff 
 */
static JSBool js_sock_read_ws_packet(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	int read_state=0;

	if (js_sock == NULL) {
		printf( "Failed to find js object.\n");
		return JS_FALSE;
	}

	int ret = 0;
	size_t len = 1;
	size_t total_length = 0;
	int can_run = 1;
	unsigned char tempbuf[2];

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	js_sock->saveDepth = JS_SuspendRequest(cx);
	while (can_run) {
		ret = read(js_sock->sock, tempbuf, 1);
		if (ret==-1) {
			JS_ReportError(cx, "read failed: %s", strerror(errno) );
			return JS_FALSE;
		}
		if (ret == 0)
			break;

		if (read_state==0) {  // try to sync to next frame
			if (tempbuf[0]==0x00) {
				read_state=1;
			}
			continue;
		}
		if (total_length == js_sock->buffer_size - 1) {
				size_t new_size = js_sock->buffer_size + 1024;
				char *new_buffer = (char*)JS_realloc(cx,js_sock->read_buffer, new_size);

				js_sock->buffer_size = new_size;
				js_sock->read_buffer = new_buffer;
		}

		js_sock->read_buffer[total_length] = tempbuf[0];
		++total_length;

		if (tempbuf[0]== 0xff) { 
			total_length--;
			break;
		}
	}
	JS_ResumeRequest(cx, js_sock->saveDepth);

	if (ret >=0) {
		js_sock->read_buffer[total_length] = 0;
 		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, js_sock->read_buffer));
	}

	return JS_TRUE;
}

/** write a websocket packet. pass one string that will be framed by the ws packet header and footer (0x00, 0xff).
 */
static JSBool js_sock_write_ws_packet(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	if (js_sock == NULL) {
		printf("Failed to find js object.\n");
		return JS_FALSE;
	}

	if (argc == 1) {
		char *buffer = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
		char frame[] = { 0x0, 0xff};
		int ret, len = strlen(buffer);
		js_sock->saveDepth = JS_SuspendRequest(cx);

		write(js_sock->sock, &frame[0], 1);
		ret = write(js_sock->sock, buffer, len);
		write(js_sock->sock, &frame[1], 1);

		JS_ResumeRequest(cx, js_sock->saveDepth);

		if (ret!=len ) {
			JS_ReportError(cx, "switch_js_sock_send failed: %d %s.\n", ret, strerror(errno));
			return JS_FALSE;
		}
	}

	return JS_TRUE;
}


/** read a line.
 */
static JSBool js_sock_read(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	int ret = 0;
	size_t len = 1;
	size_t total_length = 0;
	int can_run = 1;
	char* read_buffer;
	char* tmp;
	int read_size;

	if (js_sock == NULL) {
		printf( "Failed to find js object.\n");
		return JS_FALSE;
	}


	if (js_sock->buffer_pos>0) { // maybe we've got some old lines...
		read_buffer = js_sock->read_buffer;
		ret = js_sock->buffer_pos;

		while ( *read_buffer != '\n'  && ret>=0) {
			ret--;
			read_buffer++;
		}

		if (*read_buffer == '\n') {
			*read_buffer='\0';

			if (read_buffer > js_sock->read_buffer && read_buffer[-1]=='\r' ) {
				read_buffer[-1]='\0';
			}
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, js_sock->read_buffer ));

			if (ret>0) {
				memmove(js_sock->read_buffer, read_buffer+1, ret);
			} else {
				ret=0;
			}
			js_sock->buffer_pos = ret;

			return JS_TRUE;
		}
	}

	read_buffer = js_sock->read_buffer + js_sock->buffer_pos;
	read_size = js_sock->buffer_size - js_sock->buffer_pos;
	read_buffer[0] = '\0';

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	js_sock->saveDepth = JS_SuspendRequest(cx);
	while (can_run) {
		ret = read(js_sock->sock, read_buffer, read_size);
		if (ret==-1) {
			JS_ReportError(cx, "read failed: %s", strerror(errno) );
			return JS_FALSE;
		}
		if (ret == 0)
			break;
		// terminate on newline..:
		while ( *read_buffer != '\n'  && ret) {
			ret--;
			read_buffer++;
		}
		if (*read_buffer == '\n') {
			break;
		}
		if ( read_size <= 8 ) { // realloc early
				size_t new_size = js_sock->buffer_size + 1024;
				char *new_buffer = (char*)JS_realloc(cx,js_sock->read_buffer, new_size);

				js_sock->buffer_size = new_size;
				js_sock->read_buffer = new_buffer;

				read_size+=1024;
				read_buffer = new_buffer + new_size - read_size;
		}
		++total_length;
	}
	JS_ResumeRequest(cx, js_sock->saveDepth);

	if (*read_buffer == '\n') {

		*read_buffer = '\0';
		if (read_buffer>js_sock->read_buffer && read_buffer[-1]=='\r' ) {
			read_buffer[-1]='\0';
		}
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, js_sock->read_buffer + js_sock->buffer_pos ));

		if (ret>0) {
			// some bytes have been left over...
			memmove(js_sock->read_buffer, read_buffer+1, ret);
		}
		js_sock->read_buffer[ret]='\0';
		js_sock->buffer_pos = ret;
	} else {
		// no more data available - give up (we read half a line but want more.)
	}

	return JS_TRUE;
}

static JSBool js_sock_close(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	sock_obj_t *js_sock = (sock_obj_t*)JS_GetPrivate(cx, obj);
	if (js_sock == NULL) {
		printf( "Failed to find js object.\n");
		return JS_FALSE;
	}

	js_sock->saveDepth = JS_SuspendRequest(cx);
	close(js_sock->sock);
	js_sock->sock = -1;
	js_sock->state = disconnected;
	JS_ResumeRequest(cx, js_sock->saveDepth);
	return JS_TRUE;
}

static JSFunctionSpec js_sock_methods[] = {
	{"connect", js_sock_connect, 1},
	{"close", js_sock_close, 1},
	{"write", js_sock_send, 1},
	{"readBytes", js_sock_read_bytes, 1},
	{"read", js_sock_read, 1},
	{"read_ws_packet", js_sock_read_ws_packet, 1},
	{"write_ws_packet", js_sock_write_ws_packet, 1},
	{"waitForInput", js_sock_wait_for_input, 1},
	{"poll", js_sock_poll, 1},
	{"bind", js_sock_bind, 1},
	{"accept", js_sock_accept, 1},
	{0}
};

#define js_sock_ADDRESS 3
#define js_sock_PORT 2
#define js_sock_FD 1

static JSPropertySpec js_sock_props[] = {
// OLD CODE - maybe reuse later for statistics.
//	{"address", js_sock_ADDRESS, JSPROP_READONLY | JSPROP_PERMANENT},
//	{"port", js_sock_PORT, JSPROP_READONLY | JSPROP_PERMANENT},
	{"fd", js_sock_PORT, JSPROP_READONLY | JSPROP_PERMANENT},
	{0}
};


static JSBool js_sock_getProperty(JSContext * cx, JSObject * obj, jsval id, jsval * vp)
{
	JSBool res = JS_TRUE;
    sock_obj_t *js_sock = (sock_obj_t *)JS_GetPrivate(cx, obj);
	char *name;
	int param = 0;

	name = JS_GetStringBytes(JS_ValueToString(cx, id));
	/* numbers are our props anything else is a method */
	if (name[0] >= 48 && name[0] <= 57) {
		param = atoi(name);
	} else {
		return JS_TRUE;
	}

	switch (param) {
/*
	case js_sock_ADDRESS:
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "unknown"));
		break;
	case js_sock_PORT:
		*vp = INT_TO_JSVAL(1234);
		break;
*/
	case js_sock_FD:
		*vp = INT_TO_JSVAL(js_sock->sock);
		break;
	}

	return res;
}

JSClass js_sock_class = {
	"Socket", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, js_sock_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, js_sock_destroy, NULL, NULL, NULL,
	js_sock_construct
};

JS_BEGIN_EXTERN_C

JSBool SocketInit(JSContext * cx, JSObject * obj)
{
	JS_InitClass(cx, obj, 
					NULL, &js_sock_class, js_sock_construct, 
					1,
					js_sock_props, js_sock_methods, 
					js_sock_props, 
					js_sock_methods);
	return JS_TRUE;
}

JS_END_EXTERN_C


/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
