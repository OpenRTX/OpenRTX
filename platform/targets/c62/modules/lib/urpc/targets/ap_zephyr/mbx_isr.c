#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <venus_ap.h>

extern void Interrupt21_Handler(void);

static void
csk_mbx_isr(const struct device *dev)
{
	Interrupt21_Handler();
}

static int install_mbx_isr(void)
{
	IRQ_CONNECT(IRQ_CMN_MAILBOX_VECTOR, 2, csk_mbx_isr, NULL, 0);
	irq_enable(IRQ_CMN_MAILBOX_VECTOR);

	return 0;
}

SYS_INIT(install_mbx_isr, POST_KERNEL, 0);
