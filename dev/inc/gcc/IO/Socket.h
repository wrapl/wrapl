#ifndef IO_SOCKET_H
#define IO_SOCKET_H

#include <IO/Native.h>

#define RIVA_MODULE IO$Socket
#include <Riva-Header.h>

RIVA_TYPE(T);

//RIVA_CFUN(IO$Posix$t *, new, const char *, int);

#define IO$Socket$SOCK_STREAM	1
#define IO$Socket$SOCK_DGRAM	2
#define IO$Socket$SOCK_RAW		4

#define IO$Socket$PF_INET		8
#define IO$Socket$PF_LOCAL		16

#undef RIVA_MODULE

#endif
