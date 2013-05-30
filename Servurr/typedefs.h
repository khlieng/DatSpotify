#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
	int kc;
} write_req_t;

typedef struct {
	uv_udp_send_t req;
	uv_buf_t buf;
} udp_send_req_t;

typedef struct {
	int len;
	char * data;
} file_t;

#endif