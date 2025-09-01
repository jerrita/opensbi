#ifndef SBI_TRF_H
#define SBI_TRF_H

// _fw_start + RS_OFFSET >= _fw_end + FW_STACK_SIZE
#define RS_OFFSET 0x200000

typedef unsigned long usize;
typedef long isize;

enum trf_func_desc { INITIALIZE = 0, COPY_FROM_NORMAL };
enum gated_func_desc { OK = 0, TBI_PUT_CHAR, TBI_PUT_STR };

struct secure_ret {
	isize func_or_ret;
	usize a1, a2, a3;
};

struct trf_call {
	usize func_desc;
	usize a1, a2, a3;
};

int copy_from_normal(char *buf, const char *src, int len);

#endif