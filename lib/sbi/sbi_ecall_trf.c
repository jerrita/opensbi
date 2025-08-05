#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_console.h>

// _fw_start + RS_OFFSET >= _fw_end + FW_STACK_SIZE
#define RS_OFFSET 0x200000

typedef unsigned long usize;
typedef long isize;

enum trf_func_desc { INITIALIZE = 0 };

struct bridge_ret {
	isize func; // func
	usize a1, a2, a3;
};

struct trf_call {
	usize func_desc;
	usize a1, a2, a3;
};

typedef struct bridge_ret *(*secure_rs_call_t)(struct trf_call sc);

enum BridgedFunc { OK = 0, BBI_PUT_CHAR, BBI_PUT_STR };

extern char _fw_start[];
struct bridge_ret secure_rs_call(struct trf_call sc)
{
	struct bridge_ret ret;
	secure_rs_call_t rs_func = (secure_rs_call_t)(_fw_start + RS_OFFSET);
	__asm__ volatile("mv a0, %4\n\t" // func_desc -> a0
			 "mv a1, %5\n\t" // a1 -> a1
			 "mv a2, %6\n\t" // a2 -> a2
			 "mv a3, %7\n\t" // a3 -> a3
			 "jalr %8\n\t"	 // 调用函数
			 "mv %0, a0\n\t" // ret.func <- a0
			 "mv %1, a1\n\t" // ret.a1 <- a1
			 "mv %2, a2\n\t" // ret.a2 <- a2
			 "mv %3, a3"	 // ret.a3 <- a3
			 : "=r"(ret.func), "=r"(ret.a1), "=r"(ret.a2),
			   "=r"(ret.a3) // 输出
			 : "r"(sc.func_desc), "r"(sc.a1), "r"(sc.a2),
			   "r"(sc.a3), "r"(rs_func)	// 输入
			 : "a0", "a1", "a2", "a3", "ra" // 被修改的寄存器
	);
	return ret;
}

void secure_bridge(struct trf_call sc)
{
	struct bridge_ret ret = secure_rs_call(sc);
	while (ret.func != OK) {
		switch (ret.func) {
		case BBI_PUT_CHAR:
			sbi_putc(ret.a1);
			break;
		case BBI_PUT_STR:
			sbi_puts((char *)ret.a1);
			break;
		default:
			sbi_printf("[TRF] Unknown BridgedFunc %ld called.",
				   ret.func);
			while (1)
				;
		}
		ret = secure_rs_call(sc);
	}
}

static int sbi_ecall_trf_handler(unsigned long extid, unsigned long funcid,
				 struct sbi_trap_regs *regs,
				 struct sbi_ecall_return *out)
{
	sbi_printf("[TRF] TRF Called with funcid: %lx\n", funcid);
	struct trf_call sc;
	secure_bridge(sc);
	return 0;
}

struct sbi_ecall_extension ecall_trf;

static int sbi_ecall_trf_register_extensions(void)
{
	struct trf_call sc = { .func_desc = INITIALIZE };
	sbi_printf("[TRF] TRustForge RS registered at 0x%p\n",
		   _fw_start + RS_OFFSET);
	secure_bridge(sc);
	return sbi_ecall_register_extension(&ecall_trf);
}

struct sbi_ecall_extension ecall_trf = {
	.name		     = "trf",
	.extid_start	     = SBI_EXT_TRF,
	.extid_end	     = SBI_EXT_TRF,
	.register_extensions = sbi_ecall_trf_register_extensions,
	.handle		     = sbi_ecall_trf_handler,
};
