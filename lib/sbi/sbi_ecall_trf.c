#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_trf.h>
#include <sbi/sbi_console.h>

typedef struct secure_ret *(*secure_rs_call_t)(struct trf_call sc);

extern char _fw_start[];

struct secure_ret secure_rs_call(struct trf_call sc)
{
	struct secure_ret ret;
	secure_rs_call_t rs_func = (secure_rs_call_t)(_fw_start + RS_OFFSET);
	__asm__ volatile("mv a0, %4\n\t" // func_desc -> a0
			 "mv a1, %5\n\t" // sc.a0 -> a1
			 "mv a2, %6\n\t" // sc.a1 -> a2
			 "mv a3, %7\n\t" // sc.a2 -> a3
			 "mv a4, %8\n\t" // sc.a3 -> a4
			 "mv a5, %9\n\t" // sc.a4 -> a5
			 "mv a6, %10\n\t" // sc.a5 -> a6
			 "jalr %11\n\t"	 // 调用函数
			 "mv %0, a0\n\t" // ret.func_or_ret <- a0
			 "mv %1, a1\n\t" // ret.a1 <- a1
			 "mv %2, a2\n\t" // ret.a2 <- a2
			 "mv %3, a3"	 // ret.a3 <- a3
			 : "=r"(ret.func_or_ret), "=r"(ret.a0), "=r"(ret.a1),
			   "=r"(ret.a2) // 输出
			 : "r"(sc.func_desc), "r"(sc.a0), "r"(sc.a1),
			   "r"(sc.a2), "r"(sc.a3), "r"(sc.a4), "r"(sc.a5),
			   "r"(rs_func)	// 输入
			 : "a0", "a1", "a2", "a3", "a4", "a5", "a6",
			   "ra" // 被修改的寄存器
	);
	return ret;
}

int copy_from_normal(char *buf, const char *src, int len)
{
	struct trf_call sc    = { .func_desc = COPY_FROM_NORMAL,
				  .a0	     = (usize)buf,
				  .a1	     = (usize)src,
				  .a2	     = len,
				  .a3	     = 0,
				  .a4	     = 0,
				  .a5	     = 0 };
	struct secure_ret ret = secure_rs_call(sc);
	if (ret.func_or_ret != OK) {
		sbi_printf("cfn: copy wrong...");
		for (;;)
			;
	}
	return ret.func_or_ret;
}

struct secure_ret secure_bridge(struct trf_call sc)
{
	struct secure_ret ret = secure_rs_call(sc);
	while (ret.func_or_ret != OK) {
		switch (ret.func_or_ret) {
		case TBI_PUT_CHAR:
			sbi_putc(ret.a1);
			break;
		case TBI_PUT_STR:
			sbi_puts((char *)ret.a1);
			break;
		case TBI_TEST_FUNC:
			sbi_puts("some c function from vendor! here is c code.\n");
			break;
		default:
			sbi_printf("[TRF] Unknown func_or_ret %ld returned.\n",
				   ret.func_or_ret);
			while (1)
				;
		}
		ret = secure_rs_call(sc);
	}
	return ret;
}

// For evaluation
struct secure_ret func_sbi(struct trf_call sc)
{
	struct secure_ret ret = { 0 };
	sbi_printf("[TRF] Eval: Some Sbi Func\n");
	return ret;
}

static int sbi_ecall_trf_handler(unsigned long extid, unsigned long funcid,
				 struct sbi_trap_regs *regs,
				 struct sbi_ecall_return *out)
{
	struct secure_ret sret = { 0 };
	long trf_error;

	sbi_printf("[TRF] TRF Called with funcid: %lx\n", funcid);
	sbi_printf("a0 = %lx, a1 = %lx, a2 = %lx, a3 = %lx, a4 = %lx, a5 = %lx\n",
		   regs->a0, regs->a1, regs->a2, regs->a3, regs->a4, regs->a5);
	struct trf_call sc = { .func_desc = funcid,
			       .a0	  = regs->a0,
			       .a1	  = regs->a1,
			       .a2	  = regs->a2,
			       .a3	  = regs->a3,
			       .a4	  = regs->a4,
			       .a5	  = regs->a5 };
	if (sc.func_desc == FUNC_SBI)
		sret = func_sbi(sc);
	else
		sret = secure_bridge(sc);

	trf_error = (long)sret.a0;
	out->value = sret.a1;
	return (int)trf_error;
}

struct sbi_ecall_extension ecall_trf;

static int sbi_ecall_trf_register_extensions(void)
{
	struct trf_call sc = { .func_desc = INITIALIZE };
	sbi_printf("[TRF] TRustForge = 0x%p\n",
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
