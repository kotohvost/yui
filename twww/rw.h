#ifndef _READWRITE_H_
#define _READWRITE_H_

#undef NETREAD
#define NETREAD		read_net
int read_net( int desc, const char *buf, size_t count );

#undef NETWRITE
#define NETWRITE	write_net
int write_net( int desc, const char *buf, size_t count );

#endif
