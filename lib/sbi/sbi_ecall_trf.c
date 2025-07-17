#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_console.h>

// _fw_start + RS_OFFSET >= _fw_end + FW_STACK_SIZE
#define RS_OFFSET 0x200000

typedef unsigned long usize;
typedef long isize;

typedef struct bridge_ret *(*secure_rs_call_t)(struct sbi_trap_regs *regs);

struct bridge_ret {
	isize func; // func
	usize a1, a2, a3;
};

enum BridgedFunc { OK = 0, BBI_PUT_CHAR, BBI_PUT_STR };

extern char _fw_start[];
struct bridge_ret *secure_rs_call(struct sbi_trap_regs *regs)
{
	secure_rs_call_t rs_func = (secure_rs_call_t)(_fw_start + RS_OFFSET);
	return rs_func(regs);
}

static int sbi_ecall_trf_handler(unsigned long extid, unsigned long funcid,
				 struct sbi_trap_regs *regs,
				 struct sbi_ecall_return *out)
{
	sbi_printf("[TRF] TRF Called with funcid: %lx\n", funcid);
	struct bridge_ret *ret = secure_rs_call(regs);
	while (ret->func != OK) {
		switch (ret->func) {
		case BBI_PUT_CHAR:
			sbi_putc(ret->a1);
			break;
		case BBI_PUT_STR:
			sbi_puts((char *)ret->a1);
			break;
		default:
			sbi_printf("[TRF] Unknown BridgedFunc %ld\n called.",
				   ret->func);
			while (1)
				;
		}
		ret = secure_rs_call(regs);
	}
	return 0;
}

struct sbi_ecall_extension ecall_trf;

static int sbi_ecall_trf_register_extensions(void)
{
	sbi_printf("[TRF] TRustForge RS registered at 0x%p\n",
		   _fw_start + RS_OFFSET);
	return sbi_ecall_register_extension(&ecall_trf);
}

struct sbi_ecall_extension ecall_trf = {
	.name		     = "trf",
	.extid_start	     = SBI_EXT_TRF,
	.extid_end	     = SBI_EXT_TRF,
	.register_extensions = sbi_ecall_trf_register_extensions,
	.handle		     = sbi_ecall_trf_handler,
};
