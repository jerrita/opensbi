#ifndef SBI_TRF_H
#define SBI_TRF_H

// _fw_start + RS_OFFSET >= _fw_end + FW_STACK_SIZE
#define RS_OFFSET 0x200000

typedef unsigned long usize;
typedef long isize;

// sbi -> rs
enum trf_func_desc { INITIALIZE = 0, COPY_FROM_NORMAL, FUNC_SBI, FUNC_RS };
// rs -> sbi
enum gated_func_desc { OK = 0, TBI_PUT_CHAR, TBI_PUT_STR, TBI_TEST_FUNC };

enum trf_nginx_func_desc {
	NGINX_PRIVKEY_IMPORT = 10,
	NGINX_PRIVKEY_SIGN,
	NGINX_PRIVKEY_DECRYPT,
	NGINX_PRIVKEY_DESTROY,
	NGINX_PRIVKEY_GET_PUBKEY,
};

struct secure_ret {
	isize func_or_ret;
	usize a0, a1, a2;
};

struct trf_call {
	usize func_desc;
	usize a0, a1, a2, a3, a4, a5;
};

int copy_from_normal(char *buf, const char *src, int len);

#endif