/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define MODULE TEGRABL_ERR_USBH

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <tegrabl_clock.h>
#include <tegrabl_timer.h>
#include <tegrabl_io.h>
#include <tegrabl_drf.h>
#include <tegrabl_addressmap.h>
#include <xhci_priv.h>
#include <xhci.h>
#include <tegrabl_usbh.h>
#include <usbh_protocol.h>
#include <tegrabl_debug.h>
#include <tegrabl_malloc.h>
#include <libfdt.h>
#include <tegrabl_devicetree.h>
#include <tegrabl_regulator.h>
#include <tegrabl_gpio.h>
#include <tegrabl_xusbh_soc.h>
#include <hub.h>

#define ENABLE_REGULATORS		0U

void xusbh_xhci_writel(uint32_t reg, uint32_t value)
{
	uint32_t val;

	NV_WRITE32(NV_ADDRESS_MAP_XUSB_HOST_PF_BAR0_BASE + reg, value);
	val = NV_READ32(NV_ADDRESS_MAP_XUSB_HOST_PF_BAR0_BASE + reg);
	value = val;
}

uint32_t xusbh_xhci_readl(uint32_t reg)
{
	return NV_READ32(NV_ADDRESS_MAP_XUSB_HOST_PF_BAR0_BASE + reg);
}

static tegrabl_error_t xusbh_xhci_halt(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t halted;
	uint32_t cmd;
	uint32_t timeout;

	halted = xusbh_xhci_readl(OP_USBSTS);
	cmd = xusbh_xhci_readl(OP_USBCMD);
	halted &= STS_HALT;
	cmd &= ~(XHCI_IRQS);
	if (halted == 0x0) {
		cmd &= ~CMD_RUN;
	}
	xusbh_xhci_writel(OP_USBCMD, cmd);

	timeout = 16000;
	while (timeout > 0) {
		halted = xusbh_xhci_readl(OP_USBSTS);
		if (halted == 0xffffffff) {
			return TEGRABL_ERROR(TEGRABL_ERR_NOT_DETECTED, 0);
		}

		if ((halted & STS_HALT) == STS_HALT) {
			break;
		}
		tegrabl_udelay(1);
		timeout--;
	}

	if (timeout == 0) {
		pr_warn("can NOT halt usb host controller\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_TIMEOUT, 0);
	}
	return err;
}

static tegrabl_error_t xusbh_xhci_reset(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t val_cmd;
	uint32_t val_sts;
	uint32_t timeout;

	err = xusbh_xhci_halt();
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}

	val_sts = xusbh_xhci_readl(OP_USBSTS);
	if ((val_sts & STS_HALT) == 0) {
		pr_warn("Host controller not halted, aborting reset.\n");
		return err;
	}

	val_cmd = xusbh_xhci_readl(OP_USBCMD);
	val_cmd |= CMD_RESET;
	xusbh_xhci_writel(OP_USBCMD, val_cmd);

	timeout = 200;
	do {
		val_cmd = xusbh_xhci_readl(OP_USBCMD);
		val_sts = xusbh_xhci_readl(OP_USBSTS);
		if ((val_cmd & CMD_RESET) == 0 && (val_sts & STS_CNR) == 0) {
			break;
		}
		tegrabl_mdelay(1);
		timeout--;
	} while (timeout > 0);

	if (timeout == 0) {
		xusbh_xhci_halt();
		err = TEGRABL_ERR_TIMEOUT;
		return err;
	}

	return err;
}

static tegrabl_error_t xusbh_port_reset(struct xusb_host_context *ctx)
{
	uint32_t temp;
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t timeout;

	/* wait xHC reset done */
	timeout = 1000;
	while (timeout > 0) {
		temp = xusbh_xhci_readl(OP_PORTSC(ctx->root_port_number + 3));
		if ((temp & PORT_CSC) != PORT_CSC || (temp & PORT_CONNECT) != PORT_CONNECT) {
			timeout--;
			tegrabl_mdelay(1);
		} else {
			break;
		}
	}

	if (timeout == 0) {
		pr_warn("the root port  %d can not reset\n", ctx->root_port_number + 3);
		return TEGRABL_ERR_TIMEOUT;
	}

	if ((temp & PORT_CSC) == PORT_CSC) {
		xusbh_xhci_writel(OP_PORTSC(ctx->root_port_number + 3), temp);
	}

	tegrabl_udelay(2);
	temp = xusbh_xhci_readl(OP_PORTSC(ctx->root_port_number + 3));
	if ((temp & PORT_CONNECT) == PORT_CONNECT) {
		temp |= PORT_RESET;
		xusbh_xhci_writel(OP_PORTSC(ctx->root_port_number + 3), temp);
	} else {
		pr_debug("USB 2.0 port %d disconnected\n", ctx->root_port_number);
	}

	tegrabl_mdelay(60);
	temp = xusbh_xhci_readl(OP_PORTSC(ctx->root_port_number + 3));
	if ((temp & PORT_RC) == PORT_RC) {
		temp &= ~PORT_PE;
		xusbh_xhci_writel(OP_PORTSC(ctx->root_port_number + 3), temp);
		temp = xusbh_xhci_readl(OP_PORTSC(ctx->root_port_number + 3));
		ctx->root_port_speed = DEV_PORT_SPEED(temp);
		if ((temp & PORT_PLS_MASK) == XDEV_U0) {
			pr_info("USB 2.0 port %d new %s USB device detected\n",
					ctx->root_port_number,
					DEV_HIGHSPEED(temp) ? "high-speed" :
					DEV_FULLSPEED(temp) ? "full-speed" :
					DEV_LOWSPEED(temp) ? "low-speed" : "unknown");
		}
	}

	/* set port_id = root_port_number + 4 in case we do not like */
	/* CSC event happens after interrupt enabled. clear CSC bit  */
	ctx->port_id = ctx->root_port_number + 4;
	temp = xusbh_xhci_readl(OP_PORTSC(ctx->root_port_number + 3));
	if ((temp & PORT_CSC) == PORT_CSC) {
		xusbh_xhci_writel(OP_PORTSC(ctx->root_port_number + 3), temp);
	}
	return err;
}

static tegrabl_error_t xusbh_xhci_run(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t val_cmd;
	uint32_t timeout;

	/* Start RUNNING */
	val_cmd = xusbh_xhci_readl(OP_USBCMD);
	val_cmd |= (CMD_RUN);
	xusbh_xhci_writel(OP_USBCMD, val_cmd);

	timeout = 16000;
	while (timeout > 0) {
		if ((xusbh_xhci_readl(OP_USBSTS) & STS_HALT) == 0x0) {
			break;
		}
		tegrabl_udelay(1);
		timeout--;
	}

	if (timeout == 0) {
		xusbh_xhci_halt();
		err = TEGRABL_ERR_TIMEOUT;
		return err;
	}

	/* reset port to U0 */
	err = xusbh_port_reset(ctx);
	return err;
}

static bool is_last_trb(struct xhci_ring *ring, struct TRB *trb)
{
	if (ring->type == TYPE_EVENT) {
		return (trb == (ring->first + ring->num_of_trbs));
	} else {
		return TRB_TYPE_LINK(trb->field[3]);
	}
}

static void set_deq_ptr(struct xhci_ring *ring)
{
	ring->deque_ptr++;
	if (is_last_trb(ring, ring->deque_ptr) == true) {
		if (ring->type == TYPE_EVENT) {
			ring->cycle_state ^= 1;
		}
		ring->deque_ptr = ring->first;
	}
}

static void set_enq_ptr(struct xhci_ring *ring)
{
	uint32_t chain;

	chain = ring->enque_curr_ptr->field[3] & TRB_CHAIN;
	ring->enque_curr_ptr++;
	if (is_last_trb(ring, ring->enque_curr_ptr) != true) {
		return;
	}
	if (ring->type == TYPE_EVENT) {
		ring->enque_curr_ptr = ring->first;
	} else {
		ring->enque_curr_ptr->field[3] &= ~TRB_CHAIN;
		ring->enque_curr_ptr->field[3] |= chain;
		ring->enque_curr_ptr->field[3] ^= TRB_CYCLE;
		tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, ring->enque_curr_ptr, sizeof(struct TRB),
							   TEGRABL_DMA_TO_DEVICE);
		ring->enque_curr_ptr = ring->first;
		ring->cycle_state ^= 1;
	}
}

static void xusbh_init_ep_ctx(struct xusb_host_context *ctx)
{
	struct EP_COMMON *ep;
	struct xhci_ring *ring;

	ring = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[0];

	/* setup input ctx */
	ep = (struct EP_COMMON *)ctx->curr_dev_priv->input_context;
	/* add slot and ep0 in control slot bit 0-1*/
	ep[0].field[0] = 0;
	ep[0].field[1] = 3;

	/* speed and ep entry index */
	ep[1].field[0] = (1 << SLOT_CTX_ENTRIES_OFFSET)							|
					 (ctx->curr_dev_priv->speed << SLOT_CTX_SPEED_OFFSET)	|
					 (ctx->curr_dev_priv->route_string);

	ep[1].field[1] = ctx->port_id << SLOT_CTX_ROOT_HUB_PORT_NUM_OFFSET;

	ep[2].field[1] = (EP_TYPE_CONTROL_BI << EP_CTX_EP_TYPE_OFFSET)		|
					 (3 << EP_CTX_ERR_COUNT_OFFSET)						|
					 (USB_HS_CONTROL_MAX_PACKETSIZE << EP_CTX_MAX_PKT_SIZE_OFFSET);
	ep[2].field[2] = U64_TO_U32_LO(ring->dma) | ring->cycle_state;
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ep, INPUT_CONTEXT_SIZE,
						   TEGRABL_DMA_TO_DEVICE);

	/* Control packet size before get descriptor device is issued. */
	ctx->curr_dev_priv->enum_dev.bMaxPacketSize0 = USB_HS_CONTROL_MAX_PACKETSIZE;

	pr_trace("Input device context\n");
	pr_trace("\tControl ctx:\n");
	pr_trace("\t\tdrop flags: 0x%08x\n", ep[0].field[0]);
	pr_trace("\t\tadd  flags: 0x%08x\n", ep[0].field[1]);
	pr_trace("\tSlot ctx:\n");
	pr_trace("\t\tfield 0   : 0x%08x\n", ep[1].field[0]);
	pr_trace("\t\tfield 1   : 0x%08x\n", ep[1].field[1]);
	pr_trace("\t\tfield 2   : 0x%08x\n", ep[1].field[2]);
	pr_trace("\t\tfield 3   : 0x%08x\n", ep[1].field[3]);
	pr_trace("\tEndpoint ctx:\n");
	pr_trace("\t\tfield 0   : 0x%08x\n", ep[2].field[0]);
	pr_trace("\t\tfield 1   : 0x%08x\n", ep[2].field[1]);
	pr_trace("\t\tfield 2   : 0x%08x\n", ep[2].field[2]);
	pr_trace("\t\tfield 3   : 0x%08x\n", ep[2].field[3]);
	pr_trace("\t\tfield 4   : 0x%08x\n", ep[2].field[4]);
	tegrabl_mdelay(100);
}

static void handle_port_status_change_event(struct xusb_host_context *ctx, struct TRB *event)
{
	uint32_t port_id;
	uint32_t temp;
	unsigned int max_ports;

	/* Port status change events always have a successful completion code */
	if (COMP_CODE(event->field[2]) != COMP_SUCCESS) {
		pr_warn("%s WARNING: xHC returned failed port status event\n",
			__func__);
	}
	port_id = PORT_ID(event->field[0]);
	pr_debug("Port Status Change Event for port %d\n", port_id);

	temp = xusbh_xhci_readl(CAP_HCSPARAMS1);
	max_ports = HCS_MAX_PORTS(temp);
	if ((port_id <= 0) || (port_id > max_ports)) {
		pr_warn("Invalid port id %d\n", port_id);
		return;
	}

	/* This should not happen. But we check here to make shure */
	/* controller and FW is still running correctly            */
	if (port_id != (uint32_t)(ctx->root_port_number + 4)) {
		pr_warn("Change usb2 root port id to %d\n", (int)port_id - 4);
		ctx->root_port_number = port_id - 4;
	}

	ctx->port_id = port_id;
	temp = xusbh_xhci_readl(OP_PORTSC(ctx->port_id - 1));
	if (temp & PORT_CSC) {
		xusbh_xhci_writel(OP_PORTSC(ctx->port_id - 1), temp);
	}
}

static tegrabl_error_t handle_command_completion_event(struct xusb_host_context *ctx, struct TRB *event)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint64_t cmd_trb_ptr;

	pr_debug("event: %p\n", event);
	if ((COMP_CODE(event->field[2]) != COMP_SUCCESS) && (COMP_CODE(event->field[2]) != COMP_SHORT_PACKET)) {
		pr_warn("%s: WARNING: Command was not successfully completed (0x%02x)\n",
			__func__, COMP_CODE(event->field[2]));
		err = TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
		/* Stash comp code in context for upper-layer use */
		ctx->comp_code = COMP_CODE(event->field[2]);
		pr_debug("%s: Stashed completion code 0x%02X\n", __func__,
			 ctx->comp_code);
		return err;
	}

	cmd_trb_ptr = (event->field[0] & (~0xff)) | ((uint64_t)event->field[1] << 32);
	if (cmd_trb_ptr != (uint64_t)ctx->cmd_ring.dma) {
		pr_warn("WARNING: event and command not matching, cmd_trb_ptr = 0x%x, cmd_ring.dma = 0x%x\n",
				(uint32_t)cmd_trb_ptr, (uint32_t)ctx->cmd_ring.dma);
//		return err;
	}
	if (ctx->slot_id == 0) {
		ctx->slot_id = (int)((event->field[3]>>24) & 0xff);
		pr_trace("slot id assigned is %d\n", ctx->slot_id);
	}

	return err;
}

static tegrabl_error_t handle_transfer_event(struct xusb_host_context *ctx, struct TRB *event)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	pr_debug("event: %p\n", event);
	/* now, we only report transfer successful or not */
	/* if needed, we can return trb pointer and transfer length */
	if ((COMP_CODE(event->field[2]) != COMP_SUCCESS) && (COMP_CODE(event->field[2]) != COMP_SHORT_PACKET)) {
		pr_warn("%s: WARNING: Command was not successfully completed (0x%02x)\n",
			__func__, COMP_CODE(event->field[2]));
		err = TEGRABL_ERROR(TEGRABL_ERR_COMMAND_FAILED, 0);
		/* Stash comp code in context for upper-layer use */
		ctx->comp_code = COMP_CODE(event->field[2]);
		pr_debug("%s: Stashed completion code 0x%02X\n", __func__,
			 ctx->comp_code);
	}

	return err;
}

static tegrabl_error_t xhci_handle_events(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct TRB *event;

	event = ctx->event_ring.deque_ptr;
	tegrabl_dma_unmap_buffer(TEGRABL_MODULE_XUSB_HOST, 0, event, 4 * sizeof(struct TRB),
							 TEGRABL_DMA_FROM_DEVICE);
	pr_debug("@%p  %08x  %08x  %08x  %08x\n", event, event->field[0], event->field[1], event->field[2],
			 event->field[3]);
	/* Does the HC or OS own the TRB? */
	if ((event->field[3] & TRB_CYCLE) != ctx->event_ring.cycle_state) {
		return TEGRABL_ERR_INVALID;
	}

	/* Only handle Command_Completion/Port Status Change/Doorbell */
	/* events. ALl of others are ignored and dropped              */
	switch (event->field[3] & TRB_TYPE_BITMASK) {
	case TRB_TYPE(TRB_COMPLETION):
		err = handle_command_completion_event(ctx, event);
		break;
	case TRB_TYPE(TRB_PORT_STATUS):
		handle_port_status_change_event(ctx, event);
		break;
	case TRB_TYPE(TRB_TRANSFER):
		err = handle_transfer_event(ctx, event);
		break;
	default:
		break;
	}
	set_deq_ptr(&ctx->event_ring);

	return err;
}

static tegrabl_error_t xusbh_wait_irq(struct xusb_host_context *ctx,
				uint32_t timeout)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t status;
	uint32_t irq_pending;
	uint64_t temp_64;
	struct TRB *event;

	while (timeout > 0) {
		status = xusbh_xhci_readl(OP_USBSTS);
		pr_debug("%s status 0x%x\n", __func__, status);
		if (status == 0xffffffff || (status & STS_FATAL) == STS_FATAL) {
			pr_warn("WARNING: Host System Error\n");
			xusbh_xhci_halt();
			return TEGRABL_ERR_HALT;
		}

		if ((status & STS_EINT) == STS_EINT) {
			break;
		}
		tegrabl_mdelay(1);
		timeout--;
	}

	if (timeout == 0) {
		status = xusbh_xhci_readl(OP_USBSTS);
		if ((status & STS_EINT) != STS_EINT) {
			pr_warn("%s: Timed out! status = 0x%02X\n",
				__func__, status);
			return TEGRABL_ERR_TIMEOUT;
		}
	}

	/* Clear the op reg interrupt status */
	status |= STS_EINT;
	xusbh_xhci_writel(OP_USBSTS, status);

	/* Acknowledge the PCI interrupt */
	irq_pending = xusbh_xhci_readl(RT_IMAN(0));
	pr_debug("%s irq_pending 0x%x\n", __func__, irq_pending);
	if (ER_IRQ_PENDING(irq_pending) == IMAN_IP) {
		irq_pending |= IMAN_IP;
		xusbh_xhci_writel(RT_IMAN(0), irq_pending);
	}

	event = ctx->event_ring.deque_ptr;
	while (xhci_handle_events(ctx) == TEGRABL_NO_ERROR) {
	};

	temp_64 = xusbh_xhci_readl(RT_ERDP0(0)) | ((uint64_t)xusbh_xhci_readl(RT_ERDP1(0)) << 32);
	pr_debug("%s erst_dequeue 0x%llx\n", __func__, (unsigned long long)temp_64);

	/* Clear the event handler busy flag (RW1C); event ring is empty. */
	if (event != ctx->event_ring.deque_ptr) {
		temp_64 &= ERST_PTR_MASK;
		temp_64 |= tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, ctx->event_ring.deque_ptr,
										  sizeof(struct TRB), TEGRABL_DMA_TO_DEVICE);
	}

	temp_64 |= ERST_EHB;
	pr_debug("%s update erst_dequeue 0x%lx\n", __func__, temp_64);
	xusbh_xhci_writel(RT_ERDP0(0), U64_TO_U32_LO(temp_64));
	xusbh_xhci_writel(RT_ERDP1(0), U64_TO_U32_HI(temp_64));

	return err;
}

static tegrabl_error_t xusbh_enable_interrupt(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t val;
	int count;

	val = xusbh_xhci_readl(RT_IMOD(0));
	val &= ~ER_IRQ_INTERVAL_MASK;
	val |= (uint32_t) 160;
	xusbh_xhci_writel(RT_IMOD(0), val);

	val = xusbh_xhci_readl(OP_USBCMD);
	val |= (CMD_EIE);
	xusbh_xhci_writel(OP_USBCMD, val);

	val = xusbh_xhci_readl(RT_IMAN(0));
	/* enabling event ring interrupter */
	xusbh_xhci_writel(RT_IMAN(0), ER_IRQ_ENABLE(val));

	/* clean all of interrupts in queue */
	count = 0;
	while (true) {
		if ((xusbh_xhci_readl(OP_USBSTS) & STS_EINT) != STS_EINT) {
			break;
		}
		xusbh_wait_irq(ctx, 1);
		count++;
		if (count >= 100) {
			pr_warn("too many interrupts, something wrong!\n");
			return TEGRABL_ERROR(TEGRABL_ERR_TIMEOUT, 0);
		}
	}

	return err;
}

static tegrabl_error_t xusbh_get_slot_id(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct TRB *cmd;

	/* Enable Slot Command */
	cmd = ctx->cmd_ring.enque_curr_ptr;
	memset(cmd, 0, sizeof(struct TRB));
	cmd->field[3] = TRB_TYPE(TRB_ENABLE_SLOT) | ctx->cmd_ring.cycle_state;
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)cmd, sizeof(struct TRB),
						   TEGRABL_DMA_TO_DEVICE);

	xusbh_xhci_writel(DB(0), 0);
	pr_debug("Ding Dong!  @%08x  0x%x\n", DB(0), xusbh_xhci_readl(DB(0)));
	err = xusbh_wait_irq(ctx, 100);
	set_enq_ptr(&ctx->cmd_ring);
	return err;
}

/* prepare setup TRB */
tegrabl_error_t prepare_ctrl_setup_trb(struct xusb_host_context *ctx,
									   struct device_request *device_request_ptr,
									   enum trt_type *trt_type)
{
	struct xhci_ring *setup = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[0];
	struct setup_trb *setup_trb_ptr = NULL;

	setup->start_cycle_state = setup->cycle_state;
	/* setup->enque_curr_ptr = setup->first; */
	setup->enque_start_ptr = setup->enque_curr_ptr;
	setup_trb_ptr = (struct setup_trb *)(setup->enque_curr_ptr);
	memset((void *)setup_trb_ptr, 0, sizeof(struct setup_trb));

	switch (device_request_ptr->bRequest) {
	case GET_DESCRIPTOR:
	case GET_CONFIGURATION:
	case SET_ADDRESS:
	case SET_CONFIGURATION:
	case CLEAR_FEATURE:
		break;
	default:
		/* open all standard requests */
		/* but only above verified    */
		/* return  TEGRABL_ERROR(TEGRABL_ERR_INVALID, 0);*/
		break;
	}
	setup_trb_ptr->bmRequestType = device_request_ptr->bmRequestTypeUnion.bmRequestType;
	setup_trb_ptr->bmRequest = device_request_ptr->bRequest;
	setup_trb_ptr->wValue = device_request_ptr->wValue;
	setup_trb_ptr->wIndex = device_request_ptr->wIndex;
	setup_trb_ptr->wLength = device_request_ptr->wLength;
	setup_trb_ptr->trb_tfr_len = USB_SETUP_PKT_SIZE;
	setup_trb_ptr->cycle_bit |= (~setup->cycle_state & 0x1);
	setup_trb_ptr->IDT = 1;
	setup_trb_ptr->trb_type = TRB_SETUP;
	if (device_request_ptr->wLength != 0) {
		if (device_request_ptr->bmRequestTypeUnion.stbitfield.dataTransferDirection == DEV2HOST) {
			*trt_type = TRT_IN_DATA;
		} else {
			*trt_type = TRT_OUT_DATA;
		}
	} else {
		*trt_type = TRT_NO_DATA;
	}
		setup_trb_ptr->TRT = *trt_type;
		setup->dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)setup_trb_ptr,
											sizeof(struct TRB), TEGRABL_DMA_TO_DEVICE);
		/* update TRB pointer */
		set_enq_ptr(setup);
		return TEGRABL_NO_ERROR;
}

/* prepare control data TRB */
void prepare_ctrl_data_trb(struct xusb_host_context *ctx, struct device_request *device_request_ptr,
						   void *buffer)
{
	struct xhci_ring *data = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[0];
	struct data_trb *data_trb_ptr = (struct data_trb *)(data->enque_curr_ptr);
	dma_addr_t dma;

	memset((void *)data_trb_ptr, 0, sizeof(struct TRB));

	if (buffer == NULL) {
		data_trb_ptr->data_buffer_lo = U64_TO_U32_LO(ctx->xusb_data_dma);
		data_trb_ptr->data_buffer_hi = U64_TO_U32_HI(ctx->xusb_data_dma);
	} else {
		dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, buffer, device_request_ptr->wLength,
									 TEGRABL_DMA_TO_DEVICE);
		data_trb_ptr->data_buffer_lo = U64_TO_U32_LO(dma);
		data_trb_ptr->data_buffer_hi = U64_TO_U32_HI(dma);
	}
	data_trb_ptr->trb_tfr_len = device_request_ptr->wLength;
	data_trb_ptr->cycle_bit = data->cycle_state;
	data_trb_ptr->trb_type = TRB_DATA;
	data_trb_ptr->ISP = 1;
	data_trb_ptr->DIR = device_request_ptr->bmRequestTypeUnion.stbitfield.dataTransferDirection;
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)data_trb_ptr, sizeof(struct TRB),
						   TEGRABL_DMA_TO_DEVICE);
	/* update TRB pointer */
	set_enq_ptr(data);
	return;
}

/* prepare status stage TRB */
void prepare_ctrl_status_trb(struct xusb_host_context *ctx, struct device_request *device_request_ptr)
{
	struct xhci_ring *status = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[0];
	struct status_trb *status_trb_ptr = (struct status_trb *)(status->enque_curr_ptr);
	memset((void *)status_trb_ptr, 0, sizeof(struct TRB));

	status_trb_ptr->trb_type = TRB_STATUS;
	status_trb_ptr->DIR = !(device_request_ptr->bmRequestTypeUnion.stbitfield.dataTransferDirection);
	status_trb_ptr->cycle_bit = status->cycle_state;
	status_trb_ptr->IOC = 1;
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)status_trb_ptr, sizeof(struct TRB),
						   TEGRABL_DMA_TO_DEVICE);
	/* update TRB pointer */
	set_enq_ptr(status);
	return;
}

#define INTERRUPT_EP_INTERVAL   3
void prepare_ep_ctx(struct xusb_host_context *ctx, uint32_t ep_index, enum xhci_endpoint_type ep_type)
{
	struct EP *ep_context = ctx->curr_dev_priv->dev_context + ep_index + 1;
	uint32_t ep_ring_index;

	/* TODO: This needs to be rewritten to check ep_index against ep_type! */
	if (ep_index == 0) {
		ep_ring_index = 0;
	} else {
		if (ep_type == EP_TYPE_BULK_IN) {
			ep_ring_index = 2;
		} else {
			ep_ring_index = 1;
		}
	}

	memset((void *)ep_context, 0x0, sizeof(struct EP));

	ep_context->dw0.ep_state = EP_STATE_RUNNING;
	ep_context->dw1.cerr = USB_MAX_TXFR_RETRIES;
	/* MAX_BURST_SIZE is 0 now. Should it be 0x10 to get better perf? */
	ep_context->dw1.max_burst_size = MAX_BURST_SIZE;
	ep_context->dw1.ep_type = ep_type;
	ep_context->dw2.DCS = ctx->curr_dev_priv->ep_ring[ep_ring_index].cycle_state;
	ep_context->dw2.tr_dequeue_ptr_lo = (U64_TO_U32_LO(ctx->curr_dev_priv->ep_ring[ep_ring_index].dma) >> 4);
	ep_context->dw3.tr_dequeue_ptr_hi = U64_TO_U32_HI(ctx->curr_dev_priv->ep_ring[ep_ring_index].dma);
	if ((ep_type == EP_TYPE_BULK_IN) || (ep_type == EP_TYPE_BULK_OUT)) {
		if (ep_ring_index == 0UL) {
			pr_warn("Invalid endpoint ring index %u\n", ep_ring_index);
			return;
		}
		ep_context->dw1.max_packet_size = ctx->curr_dev_priv->enum_dev.ep[ep_ring_index - 1].packet_size;
		ep_context->dw4.average_trb_length = 0;
	} else if (ep_type == EP_TYPE_CONTROL_BI) {
		ep_context->dw1.max_packet_size = ctx->curr_dev_priv->enum_dev.bMaxPacketSize0;
		ep_context->dw4.average_trb_length = 0;
	}
	return;
}

#define DB_VALUE(ep, stream)	((((ep) + 1) & 0xff) | ((stream) << 16))
static tegrabl_error_t xhci_ring_doorbell_wait(struct xusb_host_context *ctx,
											   enum xhci_endpoint_type trb_type,
											   uint8_t ep_index)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct TRB *trb;
	uint32_t ep_ring_index;

	if (ep_index == 0) {
		ep_ring_index = 0;
	} else {
		if (trb_type == EP_TYPE_BULK_IN) {
			ep_ring_index = 2;
		} else {
			ep_ring_index = 1;
		}
	}

	pr_debug("%s TRB: from @ %p\n", (ep_index == 0) ? "CONTROL" : "TRANSFER",
			 ctx->curr_dev_priv->ep_ring[ep_ring_index].enque_start_ptr);

	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)&ctx->curr_dev_priv->dev_context[ep_index+1],
						   sizeof(struct EP), TEGRABL_DMA_TO_DEVICE);
	tegrabl_udelay(2);
	/* change cycle bit */
	trb = (struct TRB *)ctx->curr_dev_priv->ep_ring[ep_ring_index].enque_start_ptr;
	trb->field[3] &= ~0x1;
	trb->field[3] |= ctx->curr_dev_priv->ep_ring[ep_ring_index].start_cycle_state;
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, trb, sizeof(struct TRB), TEGRABL_DMA_TO_DEVICE);
	/* Ring EP doorbell */
	xusbh_xhci_writel(DB(ctx->slot_id), DB_VALUE(ep_index, 0));
	pr_debug("Ding Dong!!!  Ring EP%d doorbell (%x)\n", ep_index, xusbh_xhci_readl(DB(ctx->slot_id)));

	err = xusbh_wait_irq(ctx, 1000);
	return err;
}

/* clear endpoint stall */
static tegrabl_error_t xusbh_process_ep_stall_clear_req(struct xusb_host_context *ctx,
														enum xhci_endpoint_type ep_type)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	enum trt_type type = TRT_NO_DATA;
	struct device_request device_request_var;

	/* Clear endpoint stall */
	device_request_var.bmRequestTypeUnion.bmRequestType = HOST2DEV_ENDPOINT;
	device_request_var.bRequest = CLEAR_FEATURE;
	device_request_var.wValue = 0;
	if (ep_type == EP_TYPE_CONTROL_BI) {
		device_request_var.wIndex = 0;
	} else if (ep_type == EP_TYPE_BULK_OUT) {
		device_request_var.wIndex = (ctx->curr_dev_priv->enum_dev.ep[USB_DIR_OUT].addr);
	} else if (ep_type == EP_TYPE_BULK_IN) {
		device_request_var.wIndex = ((ctx->curr_dev_priv->enum_dev.ep[USB_DIR_IN].addr) |= ENDPOINT_DESC_ADDRESS_DIR_IN);
	}
	device_request_var.wLength = 0;

	err = prepare_ctrl_setup_trb(ctx, &device_request_var, &type);
	if (err == TEGRABL_NO_ERROR) {
		prepare_ctrl_status_trb(ctx, &device_request_var);
		prepare_ep_ctx(ctx, 0, EP_TYPE_CONTROL_BI);
		err = xhci_ring_doorbell_wait(ctx, EP_TYPE_CONTROL_BI, 0);
		if (err != TEGRABL_NO_ERROR) {
			err = TEGRABL_ERROR(TEGRABL_ERR_NO_ACCESS, 0);
		}
	}
	return err;
}

tegrabl_error_t tegrabl_xusbh_process_ctrl_req(struct xusb_host_context *ctx,
											   struct device_request *device_request_ptr,
											   void *buffer)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	enum trt_type type = TRT_NO_DATA;
	uint32_t retry_count = USB_MAX_TXFR_RETRIES;

	while (retry_count) {
		err = prepare_ctrl_setup_trb(ctx, device_request_ptr, &type);
		if (err == TEGRABL_NO_ERROR) {
			if (type != TRT_NO_DATA) {
				prepare_ctrl_data_trb(ctx, device_request_ptr, buffer);
			}
			prepare_ctrl_status_trb(ctx, device_request_ptr);
			prepare_ep_ctx(ctx, 0, EP_TYPE_CONTROL_BI);

			err = xhci_ring_doorbell_wait(ctx, EP_TYPE_CONTROL_BI, 0);
			if (err == TEGRABL_NO_ERROR) {
				break;
			}
			if (err == TEGRABL_ERR_TIMEOUT) {
				err = xusbh_process_ep_stall_clear_req(ctx, EP_TYPE_CONTROL_BI);
				break;
			} else {
				retry_count--;
			}
		}
	}
	return err;
}

/* parse config descriptions */
tegrabl_error_t xhci_parse_config_desc(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_ERR_INVALID;
	struct usb_intf_desc *usb_intf_desc_ptr;
	struct usb_endpoint_desc *usb_edp_desc_ptr;
	uint32_t num_interfaces;
	uint32_t num_eps;
	uint32_t i, j;
	uint32_t ep_dir;
	bool ep_dir_in_flag = false;
	bool ep_dir_out_flag = false;
	uint32_t transfer_type;

	/* Initialize to 0 */
	num_eps = 0;
	/* Get ptr to interface descriptor */
	usb_intf_desc_ptr = (struct usb_intf_desc *)((uint8_t *)(ctx->xusb_data) + USB_CONFIG_DESC_SIZE);
	/* get number of supported interfaces */
	num_interfaces = ((struct usb_config_desc *)(ctx->xusb_data))->bNumInterfaces;
	pr_trace("num interfaces: %u\n", num_interfaces);

	/* Get endpoint descriptor for current interface descriptor */
	usb_edp_desc_ptr = (struct usb_endpoint_desc *)((uint8_t *)usb_intf_desc_ptr + USB_INTF_DESC_SIZE);

	/*
	 Check endpoint descriptors for a given interface till bulk only
	 IN and OUT type endpoints are found
	 */
	for (i = 0; ((i < num_interfaces) && (err != TEGRABL_NO_ERROR)); i++) {
		/*
		   get next interface descriptor pointer. Interface Descriptor is
		   followed by array of Endpoint descriptor of size num_eps
		   Value of num_eps used is the value from previous
		   interface Descriptor. In the beginining of loop it is 0
		*/
		usb_intf_desc_ptr = (struct usb_intf_desc *)(((uint8_t *)usb_intf_desc_ptr) + num_eps *
													USB_EDP_DESC_SIZE);
		/* Get supported number of endpoints for current
			interface descriptor
		*/
		num_eps = usb_intf_desc_ptr->bNumEndpoints;
		/* Get endpoint descriptor for current interface descriptor */
		usb_edp_desc_ptr = (struct usb_endpoint_desc *)(((uint8_t *)usb_intf_desc_ptr) + USB_INTF_DESC_SIZE);
		/*
		 * Save class, subclass and protocol to host context for use
		 * in class driver(s)
		 */
		ctx->curr_dev_priv->enum_dev.class = usb_intf_desc_ptr->bInterfaceClass;
		ctx->curr_dev_priv->enum_dev.subclass = usb_intf_desc_ptr->bInterfaceSubClass;
		ctx->curr_dev_priv->enum_dev.protocol = usb_intf_desc_ptr->bInterfaceProtocol;
		pr_trace("iface: %u,  class/subclass/proto: %x/%x/%x\n",
				i,
				ctx->curr_dev_priv->enum_dev.class,
				ctx->curr_dev_priv->enum_dev.subclass,
				ctx->curr_dev_priv->enum_dev.protocol);
		if (usb_intf_desc_ptr->bInterfaceClass == USB_CLASS_MSD ||
			usb_intf_desc_ptr->bInterfaceClass == USB_CLASS_HUB) {
			ctx->curr_dev_priv->enum_dev.interface_indx = usb_intf_desc_ptr->bInterfaceNumber;
			/* only support 2 bulk EPs or interrupt EP (for HUB) */
			for (j = 0; j < num_eps; j++) {
				transfer_type = usb_edp_desc_ptr->bmAttributes & ENDPOINT_DESC_ATTRIBUTES_TRANSFER_TYPE_MASK;
				if ((transfer_type == ENDPOINT_DESC_ATTRIBUTES_BULK_TYPE) ||
					(transfer_type == ENDPOINT_DESC_ATTRIBUTES_INTERRUPT_TYPE)) {
					ep_dir = (usb_edp_desc_ptr->bEndpointAddress) & ENDPOINT_DESC_ADDRESS_DIR_MASK;
					if (ep_dir == ENDPOINT_DESC_ADDRESS_DIR_IN) {
						ep_dir = USB_DIR_IN;
						ep_dir_in_flag = true;
					} else {
						ep_dir = USB_DIR_OUT;
						ep_dir_out_flag = true;
					}

					if (ctx->curr_dev_priv->enum_dev.ep[ep_dir].addr == 0) {
						pr_debug("%s: ep#%d, dir = %s, max packet = %d\n",
								 __func__, j, ep_dir ? "IN" : "OUT", usb_edp_desc_ptr->wMaxPacketSize);
						ctx->curr_dev_priv->enum_dev.ep[ep_dir].packet_size =
							(usb_edp_desc_ptr->wMaxPacketSize);
						ctx->curr_dev_priv->enum_dev.ep[ep_dir].addr =
							(usb_edp_desc_ptr->bEndpointAddress) & ENDPOINT_DESC_ADDRESS_ENDPOINT_MASK;
					}
					if ((ep_dir_in_flag == true) && (ep_dir_out_flag == true)) {
						ctx->curr_dev_priv->enum_dev.bInterval = usb_edp_desc_ptr->bInterval;
						err = TEGRABL_NO_ERROR;
						break;
					}
				}
				/* Check next endpoint descriptor */
				usb_edp_desc_ptr =
					(struct usb_endpoint_desc *)((uint8_t *)usb_edp_desc_ptr + USB_EDP_DESC_SIZE);
			}
		} else {
			/* Check next interface if there is any */
			continue;
		}
	}
	return err;
}

static tegrabl_error_t xhci_address_device(struct xusb_host_context *ctx, bool bsr)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct TRB *cmd;
	dma_addr_t dma;

	/* Address Device Command */
	cmd = ctx->cmd_ring.enque_curr_ptr;
	cmd->field[0] = U64_TO_U32_LO(ctx->curr_dev_priv->input_context_dma);
	cmd->field[1] = U64_TO_U32_HI(ctx->curr_dev_priv->input_context_dma);
	cmd->field[2] = 0;
	cmd->field[3] = TRB_TYPE(TRB_ADDR_DEV) | (ctx->slot_id << 24) | ctx->cmd_ring.cycle_state;
	if (bsr == true) {
		cmd->field[3] |= ADDR_DEV_BSR;
	}

	pr_debug("ADDR trb: %p  : %08x  %08x  %08x  %08x\n", cmd, cmd->field[0], cmd->field[1], cmd->field[2],
			 cmd->field[3]);

	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)cmd, sizeof(struct TRB),
								 TEGRABL_DMA_TO_DEVICE);
	xusbh_xhci_writel(OP_CRCR0, (U64_TO_U32_LO(dma) | 0x1));
	xusbh_xhci_writel(OP_CRCR1, U64_TO_U32_HI(dma));

	xusbh_xhci_writel(DB(0), 0);
	pr_debug("Ding Dong!  @%08x  0x%x\n", DB(0), xusbh_xhci_readl(DB(0)));
	err = xusbh_wait_irq(ctx, 100);
/*
	xhci_print_slot_ctx(ctx, 0);
	xhci_print_ep_ctx(ctx, 0, 3, 0);
	xhci_print_slot_ctx(ctx, 1);
	xhci_print_ep_ctx(ctx, 0, 3, 1);
*/
	set_enq_ptr(&ctx->cmd_ring);
	return err;
}

static tegrabl_error_t xhci_endpoint_config(
				struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct TRB *cmd;
	dma_addr_t dma;
	struct ctrl_ctx *ctrl;
	struct slot_ctx *slot;
	struct EP_COMMON *ep;
	struct xhci_ring *ring;
	int idx;


	ctrl = (struct ctrl_ctx *)ctx->curr_dev_priv->input_context;
	slot = (struct slot_ctx *)(ctx->curr_dev_priv->input_context + 1);
	ep = (struct EP_COMMON *)(ctx->curr_dev_priv->input_context + 1);
	ctrl[0].drop_flags = 0;
	ctrl[0].add_flags = 1;
	if (ctx->curr_dev_priv->enum_dev.ep[0].addr != 0) {
		idx = ctx->curr_dev_priv->enum_dev.ep[0].addr * 2 ;
		ctrl[0].add_flags |= (1 << (ctx->curr_dev_priv->enum_dev.ep[0].addr * 2));
		ring = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[1];
		ep[idx].field[2] = U64_TO_U32_LO(ring->dma) | ring->cycle_state;
		ep[idx].field[1] = (EP_TYPE_BULK_OUT << 3) | (3 << 1) |  /* error count*/
						   (ctx->curr_dev_priv->enum_dev.ep[0].packet_size << 16);
		slot->info[0] |= 0x10000000;
	}

	if (ctx->curr_dev_priv->enum_dev.ep[1].addr != 0) {
		idx = ctx->curr_dev_priv->enum_dev.ep[1].addr * 2 + 1;
		ctrl[0].add_flags |= (1 << (ctx->curr_dev_priv->enum_dev.ep[1].addr * 2 + 1));
		ring = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[2];
		ep[idx].field[2] = U64_TO_U32_LO(ring->dma) | ring->cycle_state;
		ep[idx].field[1] = (EP_TYPE_BULK_IN << 3) | (3 << 1) |  /* error count*/
						   (ctx->curr_dev_priv->enum_dev.ep[1].packet_size << 16);
		slot->info[0] |= 0x10000000;
	}
	pr_debug("ctrl_slot : add_flags = 0x%x\n", ctrl[0].add_flags);
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctrl, sizeof(struct EP) * 33,
						   TEGRABL_DMA_TO_DEVICE);
	/* Endpoint Config Command */
	cmd = ctx->cmd_ring.enque_curr_ptr;
	cmd->field[0] = U64_TO_U32_LO(ctx->curr_dev_priv->input_context_dma);
	cmd->field[1] = U64_TO_U32_HI(ctx->curr_dev_priv->input_context_dma);
	cmd->field[2] = 0;
	cmd->field[3] = TRB_TYPE(TRB_CONFIG_EP) | (ctx->slot_id << 24) | ctx->cmd_ring.cycle_state;
	pr_debug("trb: %p  : %08x  %08x  %08x  %08x\n", cmd, cmd->field[0], cmd->field[1], cmd->field[2],
			 cmd->field[3]);
	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)cmd, sizeof(struct TRB),
								 TEGRABL_DMA_TO_DEVICE);
	xusbh_xhci_writel(OP_CRCR0, (U64_TO_U32_LO(dma) | 0x1));
	xusbh_xhci_writel(OP_CRCR1, U64_TO_U32_HI(dma));

	xusbh_xhci_writel(DB(0), 0);
	pr_debug("Ding Dong!  @%08x  0x%x\n", DB(0), xusbh_xhci_readl(DB(0)));
	err = xusbh_wait_irq(ctx, 100);
	tegrabl_dma_unmap_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctx->curr_dev_priv->dev_context, sizeof(struct EP) * 10,
							 TEGRABL_DMA_FROM_DEVICE);
/*
	xhci_print_slot_ctx(ctx, 0);
	xhci_print_ep_ctx(ctx, 0, 3, 0);
	xhci_print_slot_ctx(ctx, 1);
	xhci_print_ep_ctx(ctx, 0, 3, 1);
*/
	set_enq_ptr(&ctx->cmd_ring);

	/* clean up input context after config */
	memset((void *)&ctrl[3], 0, sizeof(struct ctrl_ctx) * 30);
	ctrl[0].drop_flags = 0;
	ctrl[0].add_flags = 0;
	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctrl, sizeof(struct EP) * 33,
						   TEGRABL_DMA_TO_DEVICE);

	return err;
}

static bool xhci_is_device_non_MSD(struct xusb_host_context *ctx, uint8_t bDeviceClass)
{
	uint32_t i;
	uint32_t num_eps;
	struct usb_intf_desc *usb_intf_desc_ptr;
	uint32_t num_interfaces;
	bool is_dev_non_MSD = true;

	if (bDeviceClass != 0) {
		/* If this is first device */
		if (ctx->hub_slot_id == 0) {
			if ((bDeviceClass == USB_CLASS_HUB) ||
				(bDeviceClass == USB_CLASS_MSD)) {
				is_dev_non_MSD = false;
			}
		/* If this is 2nd device */
		} else {
			if ((bDeviceClass == USB_CLASS_MSD)) {
				is_dev_non_MSD = false;
			}
		}

	} else {
		usb_intf_desc_ptr = (struct usb_intf_desc *)((uint8_t *)(ctx->xusb_data) + USB_CONFIG_DESC_SIZE);
		num_interfaces = ((struct usb_config_desc *)(ctx->xusb_data))->bNumInterfaces;
		pr_trace("num interfaces: %u\n", num_interfaces);
		num_eps = 0;
		for (i = 0; i < num_interfaces; i++) {
			usb_intf_desc_ptr =
				(struct usb_intf_desc *)(((uint8_t *)usb_intf_desc_ptr) + (num_eps * USB_EDP_DESC_SIZE));
			pr_trace("iface: %u,  class: %x\n", i, usb_intf_desc_ptr->bInterfaceClass);
			if (usb_intf_desc_ptr->bInterfaceClass == USB_CLASS_MSD) {
				is_dev_non_MSD = false;
				break;
			}
			num_eps = usb_intf_desc_ptr->bNumEndpoints;
		}
	}

	return is_dev_non_MSD;
}

static tegrabl_error_t xusbh_enumerate_device(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct device_request device_request_var;
	uint32_t setup_buffer_size;

	pr_info("Start to enumerate device\n");

	err = xhci_address_device(ctx, false);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	pr_trace("address assigned\n");

	/* get device descriptor with addressed control endpoint 0 */
	pr_debug("get device descriptor\n");
	device_request_var.bmRequestTypeUnion.bmRequestType = DEV2HOST_DEVICE;
	device_request_var.bRequest = GET_DESCRIPTOR;
	device_request_var.wValue = USB_DT_DEVICE << 8;
	device_request_var.wIndex = 0;
	device_request_var.wLength = USB_HS_CONTROL_MAX_PACKETSIZE;
	err = tegrabl_xusbh_process_ctrl_req(ctx, &device_request_var, NULL);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	tegrabl_dma_unmap_buffer(TEGRABL_MODULE_XUSB_HOST, 0, ctx->xusb_data, SETUP_DATA_BUFFER_SIZE,
							 TEGRABL_DMA_FROM_DEVICE);

	ctx->curr_dev_priv->enum_dev.bDeviceClass =
									((struct usb_device_desc *)(ctx->xusb_data))->bDeviceClass;
	ctx->curr_dev_priv->enum_dev.bDeviceProtocol =
									((struct usb_device_desc *)(ctx->xusb_data))->bDeviceProtocol;
	ctx->curr_dev_priv->enum_dev.bMaxPacketSize0 =
									((struct usb_device_desc *)(ctx->xusb_data))->bMaxPacketSize0;
	ctx->curr_dev_priv->enum_dev.bNumConfigurations =
									((struct usb_device_desc *)(ctx->xusb_data))->bNumConfigurations;
	ctx->curr_dev_priv->enum_dev.vendor_id =
									((struct usb_device_desc *)(ctx->xusb_data))->idVendor;
	ctx->curr_dev_priv->enum_dev.product_id =
									((struct usb_device_desc *)(ctx->xusb_data))->idProduct;
	pr_debug("device class/proto: %x/%x\n",
			 ctx->curr_dev_priv->enum_dev.bDeviceClass,
			 ctx->curr_dev_priv->enum_dev.bDeviceProtocol);
	pr_debug("USB device %04X:%04X\n",
			 ctx->curr_dev_priv->enum_dev.vendor_id,
			 ctx->curr_dev_priv->enum_dev.product_id);

	/* get config descriptor only!! */
	pr_debug("get config descriptor only\n");
	device_request_var.bmRequestTypeUnion.bmRequestType = DEV2HOST_DEVICE;
	device_request_var.bRequest = GET_DESCRIPTOR;
	device_request_var.wValue = USB_DT_CONFIG << 8;
	device_request_var.wIndex = 0;
	device_request_var.wLength = USB_CONFIG_DESCRIPTOR_SIZE;
	err = tegrabl_xusbh_process_ctrl_req(ctx, &device_request_var, NULL);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	tegrabl_dma_unmap_buffer(TEGRABL_MODULE_XUSB_HOST, 0, ctx->xusb_data, USB_CONFIG_DESCRIPTOR_SIZE,
							 TEGRABL_DMA_FROM_DEVICE);
	/* update configuration value. */
	ctx->curr_dev_priv->enum_dev.bConfigurationValue = ((struct usb_config_desc *)(ctx->xusb_data))->bConfigurationValue;

	/* get complete config descriptor */
	/* All other fields remain same as above */
	pr_debug("get all config descriptor\n");
	device_request_var.wLength = ((struct usb_config_desc *)(ctx->xusb_data))->wTotalLength;

	if (SETUP_DATA_BUFFER_SIZE < device_request_var.wLength) {
		/* If total length of the descriptors is more than the existing setup data buffer size,
		 * dealloc the existing buffer and allocate the required length freshly.*/
		tegrabl_dealloc(TEGRABL_HEAP_DMA, ctx->xusb_data);

		setup_buffer_size = ROUND_UP(device_request_var.wLength, 1024);
		ctx->xusb_data = (uint32_t *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, setup_buffer_size);
		if (ctx->xusb_data == NULL) {
			pr_error("failed to allocate memory for xusb data buffer\n");
			err = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
			goto fail;
		}
		memset(ctx->xusb_data, 0x0, setup_buffer_size);
		ctx->xusb_data_dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0,
			(void *)ctx->xusb_data, setup_buffer_size, TEGRABL_DMA_TO_DEVICE);
	}

	err = tegrabl_xusbh_process_ctrl_req(ctx, &device_request_var, NULL);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}
	tegrabl_dma_unmap_buffer(TEGRABL_MODULE_XUSB_HOST, 0, ctx->xusb_data, SETUP_DATA_BUFFER_SIZE,
							 TEGRABL_DMA_FROM_DEVICE);

	/* Skip enumeration if the device is non-MSD because it cannot boot */
	if (xhci_is_device_non_MSD(ctx, ctx->curr_dev_priv->enum_dev.bDeviceClass)) {
		pr_info("This device is non-MSD, skip enumeration\n");
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_FOUND, 0);
		goto fail;
	}

	err = xhci_parse_config_desc(ctx);

	if (ctx->curr_dev_priv->enum_dev.class == USB_CLASS_MSD) {
		/* Endpoint Configuration Command */
		pr_debug("config endpoint\n");
		err = xhci_endpoint_config(ctx);
		if (err != TEGRABL_NO_ERROR) {
			return err;
		}
	}

	/* set configuration ! */
	pr_debug("set configuration\n");
	device_request_var.bmRequestTypeUnion.bmRequestType = HOST2DEV_DEVICE;
	device_request_var.bRequest = SET_CONFIGURATION;
	device_request_var.wValue = ctx->curr_dev_priv->enum_dev.bConfigurationValue;
	device_request_var.wIndex = 0;
	device_request_var.wLength = 0;

	err = tegrabl_xusbh_process_ctrl_req(ctx, &device_request_var, NULL);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}

	pr_info("\n");
	pr_info("Enumerated USB Device %04X:%04X\n",
			ctx->curr_dev_priv->enum_dev.vendor_id, ctx->curr_dev_priv->enum_dev.product_id);
	pr_info("\n");
	if (ctx->curr_dev_priv->enum_dev.ep[0].addr != 0) {
		ctx->curr_dev_priv->enum_dev.dev_addr = ctx->curr_dev_priv->enum_dev.ep[0].addr;
	}

fail:
	return err;
}

static void xhci_pad_ctrl_init(struct xusb_host_context *context)
{
}

static void xhci_clk_rst_init(struct xusb_host_context *context)
{
}

static tegrabl_error_t xhci_regulator_init(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	void *fdt;
	const uint32_t *temp;
	int32_t xhci_node_offset = 0;
	int32_t ports_node_offset = 0;
	int32_t vbus_node_offset = 0;
	int32_t reg_phandle = 0;
	uint32_t i = 0;
	bool is_available, is_enabled;

	err = tegrabl_dt_get_fdt_handle(TEGRABL_DT_BL, &fdt);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("%s %d failed\n", __func__, __LINE__);
		return err;
	}

	err = tegrabl_dt_get_node_with_compatible(fdt, 0, PADCTL_DT_COMPATIBLE, &xhci_node_offset);
	if (err != TEGRABL_NO_ERROR) {
		pr_warn("%s %d failed\n", __func__, __LINE__);
		return err;
	}

	ports_node_offset = fdt_subnode_offset(fdt, xhci_node_offset, "ports");
	if (ports_node_offset < 0) {
		pr_warn("%s %d failed\n", __func__, __LINE__);
		return err;
	}

	for (i = 0; i < HOST_PORTS_NUM; i++) {
		char prop_name[] = "usb2-X";

		snprintf(prop_name, sizeof(prop_name), "usb2-%d", i);

		/* get vbus node offset */
		vbus_node_offset = fdt_subnode_offset(fdt, ports_node_offset, prop_name);
		if (vbus_node_offset < 0) {
			pr_warn("%s %d failed\n", __func__, __LINE__);
			return err;
		}

		err = tegrabl_dt_is_device_available(fdt, vbus_node_offset, &is_available);
		if (err != TEGRABL_NO_ERROR) {
			pr_warn("%s %d failed\n", __func__, __LINE__);
			return err;
		}

		if (!is_available)
			continue;

		/* get vbus-supply node property */
		temp = fdt_getprop(fdt, vbus_node_offset, "vbus-supply", NULL);

		if (temp == NULL)
			continue;

		reg_phandle = fdt32_to_cpu(*temp);

		tegrabl_regulator_is_enabled(reg_phandle, &is_enabled);

		if (is_enabled) {
			pr_info("regulator of %s already enabled\n", prop_name);
			tegrabl_regulator_disable(reg_phandle);
		}

		err = tegrabl_regulator_enable(reg_phandle);

	}
	return err;
}

tegrabl_error_t xhci_controller_init(struct xusb_host_context *context)
{
	tegrabl_error_t e = TEGRABL_NO_ERROR;

	/* clock: Setup UTMIPLL under SW control. Section 7.1.3 of t194 Clocks IAS.
	 * This is by default under SW control, nothing to be done.
	 * */

	/* clock: deassert reset to XUSB AO block.
	 * This is not being exposed by BPMP-FW
	 * */

	/* Enable clock to XUSB PADCTL block.
	 * This is not being exposed by BPMP-FW
	 * */

	/* This sets clock source for core_host, falcon */
	e = tegrabl_usb_host_clock_init();
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/* This sets clock source for core_dev, SS, FS */
	e = tegrabl_usbf_clock_init();
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/* deassert reset to XUSB PADCTL block */
	tegrabl_car_rst_clear(TEGRABL_MODULE_XUSB_PADCTL, 0);

	/* TODO: Enable device operation on port0 */

	/* Initialize BIAS pad registers and perform BIAS pad tracking */
	e = xhci_init_bias_pad();
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/* Iniitalize USB2 padn registers */
	e = xhci_init_usb2_padn();
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/* clock: Setup UTMIPLL under HW control. Section 7.1.3 of clocks IAS */
	e = tegrabl_clk_pll_hw_sequencer_enable(false, TEGRABL_CLK_PLL_ID_UTMI_PLL);
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	/* TODO: PINMUX: program pinmux registers to setup IO pins for usb 2.0 ports */
	xhci_init_pinmux();

	/* UPHY programming.
	 * Expecting this to be done during BPMP-FW initialization.
	 * */

	/* release XUSB SS wake logic latching */
	xhci_release_ss_wakestate_latch();

	/* clock: step6: program the clocks and deassert the resets to the controller */
	/**
	 * 1. xxxx CLK_OUT_ENB_XUSB => enable
	 * 2. CLK_OUT_ENB_XUSB => ENB_XUSB_HOST
	 * 3. CLK_OUT_ENB_XUSB => ENB_XUSB_DEV
	 * 4. CLK_OUT_ENB_XUSB => ENB_XUSB_SS
	 * 5. set source of XUSB clocks ad PLLP_OUT0. (and divisor as 0x6 for below)
	 * CLK_SOURCE_XUSB_CORE_HOST, XUSB_CORE_DEV, XUSB_FALCON
	 * 6. Set source of XSUB_FS to FO_48M and divisor to 0x0
	 * 7. Set source of XUSB_SS to HSIC_480 and divisor to 0x6
	 * 8. deassert reset to XUSB. (XUSB_HOST_RST, XUSB_DEV_RST, XUSB_SS_RST)
	 */
	e = tegrabl_car_clk_enable(TEGRABL_MODULE_XUSB_HOST, 0, NULL);
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	e = tegrabl_car_clk_enable(TEGRABL_MODULE_XUSB_DEV, 0, NULL);
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	e = tegrabl_car_clk_enable(TEGRABL_MODULE_XUSB_SS, 0, NULL);
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	tegrabl_car_rst_clear(TEGRABL_MODULE_XUSB_HOST, 0);
	tegrabl_car_rst_clear(TEGRABL_MODULE_XUSB_DEV, 0);
	tegrabl_car_rst_clear(TEGRABL_MODULE_XUSB_SS, 0);

	/* TODO: Bring UPHY out of IDDQ.
	 * Expecting this to be done during BPMP-FW initialization.
	 * */

	/* Load XUSB FW */
	e = xusbh_load_firmware();
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	e = xhci_regulator_init();
	if (e != TEGRABL_NO_ERROR) {
		goto fail;
	}

	tegrabl_mdelay(1000);

	context->root_port_number = 0xff;
	if (xhci_set_root_port(context) == false) {
		e = TEGRABL_ERROR(TEGRABL_ERR_NOT_CONNECTED, 0);
		goto fail;
	}

fail:
	return e;
}

tegrabl_error_t init_common_data_struct(struct xusb_host_context *ctx)
{
	struct xhci_ring *ring;
	struct TRB *trb;
	uint32_t max_slot;
	uint32_t val;
	dma_addr_t dma;
	tegrabl_error_t e = TEGRABL_NO_ERROR;

	ctx->page_size_log2 = 12;
	ctx->page_size = 1 << ctx->page_size_log2;

	max_slot = xusbh_xhci_readl(CAP_HCSPARAMS1) & 0xff;
	val = xusbh_xhci_readl(OP_CONFIG);
	val &= ~0xff;
	val |= max_slot;
	xusbh_xhci_writel(OP_CONFIG, val);

	/* Allocate and initialize DCBAA */
	ctx->dcbaa = (uint64_t *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, DCBAA_SIZE);
	if (ctx->dcbaa == NULL) {
		pr_error("failed to allocate memory for DCBAA\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ctx->dcbaa, 0x0, DCBAA_SIZE);
	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctx->dcbaa, DCBAA_SIZE,
								 TEGRABL_DMA_TO_DEVICE);
	xusbh_xhci_writel(OP_DCBAAP0, U64_TO_U32_LO(dma));
	xusbh_xhci_writel(OP_DCBAAP1, U64_TO_U32_HI(dma));

	/* Allocate and initialize Command Ring */
	ring = (struct xhci_ring *)&ctx->cmd_ring;
	ring->first = (struct TRB *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, CR_SEGMENT_SIZE);
	if (ring->first == NULL) {
		pr_error("failed to allocate memory for command ring\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ring->first, 0x0, CR_SEGMENT_SIZE);
	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ring->first, CR_SEGMENT_SIZE,
								 TEGRABL_DMA_TO_DEVICE);
	ring->num_of_trbs = NUM_TRB_CMD_RING;
	trb = ring->first;
	ring->enque_start_ptr = ring->first;
	ring->enque_curr_ptr = ring->first;
	ring->deque_ptr = ring->first;
	ring->cycle_state = 1;
	ring->start_cycle_state = 1;
	ring->type = TYPE_COMMAND;
	trb += NUM_TRB_CMD_RING - 1;
	ring->dma = dma;
	xusbh_xhci_writel(OP_CRCR0, (U64_TO_U32_LO(dma) | 0x1));
	xusbh_xhci_writel(OP_CRCR1, U64_TO_U32_HI(dma));
	trb->field[0] = U64_TO_U32_LO(dma);
	trb->field[1] = U64_TO_U32_HI(dma);
	trb->field[3] = TRB_TYPE(TRB_LINK) | 0x2;

	/* Allocate and initialize Event Ring and ERST */
	ring = (struct xhci_ring *)&ctx->event_ring;
	ring->first = (struct TRB *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, ER_SEGMENT_SIZE);
	if (ring->first == NULL) {
		pr_error("failed to allocate memory for event ring\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ring->first, 0x0, ER_SEGMENT_SIZE);
	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ring->first, ER_SEGMENT_SIZE,
								 TEGRABL_DMA_TO_DEVICE);
	ring->num_of_trbs = NUM_TRB_EVENT_RING;
	ring->enque_start_ptr = ring->enque_curr_ptr = ring->deque_ptr = ring->first;
	ring->cycle_state = 1;
	ring->type = TYPE_EVENT;

	/* ERST */
	ctx->erst = (struct xhci_erst_entry *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, ERST_SIZE);
	if (ctx->erst == NULL) {
		pr_error("failed to allocate memory for ERST\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ctx->erst, 0x0, ERST_SIZE);
	ctx->erst->seg_addr = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ring->first,
												 ER_SEGMENT_SIZE, TEGRABL_DMA_TO_DEVICE);
	xusbh_xhci_writel(RT_ERDP0(0), U64_TO_U32_LO(ctx->erst->seg_addr));
	xusbh_xhci_writel(RT_ERDP1(0), U64_TO_U32_HI(ctx->erst->seg_addr));
	ctx->erst->seg_size = NUM_TRB_EVENT_RING;
	val = xusbh_xhci_readl(RT_ERSTSZ(0));
	val &= ERST_SIZE_MASK;
	val |= 1;
	xusbh_xhci_writel(RT_ERSTSZ(0), val);
	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctx->erst, ERST_SIZE,
								 TEGRABL_DMA_TO_DEVICE);
	xusbh_xhci_writel(RT_ERSTBA0(0), U64_TO_U32_LO(dma));
	xusbh_xhci_writel(RT_ERSTBA1(0), U64_TO_U32_HI(dma));

fail:
	return e;
}

tegrabl_error_t init_data_struct(struct xusb_host_context *ctx)
{
	struct xhci_ring *ring;
	struct TRB *trb;
	uint32_t i;
	dma_addr_t dma;
	tegrabl_error_t e = TEGRABL_NO_ERROR;

	/* Prepare input context, device context, input control context */
	ctx->curr_dev_priv->input_context = (struct EP *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, INPUT_CONTEXT_SIZE);
	if (ctx->curr_dev_priv->input_context == NULL) {
		pr_error("failed to allocate memory for input context\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ctx->curr_dev_priv->input_context, 0x0, INPUT_CONTEXT_SIZE);
	ctx->curr_dev_priv->input_context_dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctx->curr_dev_priv->input_context,
													INPUT_CONTEXT_SIZE, TEGRABL_DMA_TO_DEVICE);

	ctx->curr_dev_priv->dev_context = (struct EP *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, DEV_CONTEXT_SIZE);
	if (ctx->curr_dev_priv->dev_context == NULL) {
		pr_error("failed to allocate memory for device context\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ctx->curr_dev_priv->dev_context, 0x0, DEV_CONTEXT_SIZE);
	ctx->curr_dev_priv->dev_context_dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0,
		(void *)ctx->curr_dev_priv->dev_context, DEV_CONTEXT_SIZE, TEGRABL_DMA_TO_DEVICE);

	tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ctx->dcbaa, 128, TEGRABL_DMA_TO_DEVICE);

	/* Transfer rings for EP0, EP1(OUT), EP1(IN) */
	for (i = 0; i < 3; i++) {
		ring = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[i];

		ring->first = (struct TRB *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 16, TR_SEGMENT_SIZE);
		if (ring->first == NULL) {
			pr_error("failed to allocate memory for EP\n");
			e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
			goto fail;
		}
		memset(ring->first, 0x0, TR_SEGMENT_SIZE);
		dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0,
			(void *)ring->first, TR_SEGMENT_SIZE, TEGRABL_DMA_TO_DEVICE);

		ring->enque_start_ptr = ring->first;
		ring->enque_curr_ptr = ring->first;
		ring->deque_ptr = ring->first;
		ring->num_of_trbs = NUM_TRB_TX_RING;
		ring->cycle_state = 1;
		ring->start_cycle_state = 1;
		ring->dma = dma;
		trb = ring->first;
		trb += (NUM_TRB_TX_RING - 1);
		trb->field[0] = U64_TO_U32_LO(dma);
		trb->field[1] = U64_TO_U32_HI(dma);
		trb->field[3] = TRB_TYPE(TRB_LINK) | 0x2;
	}

	/* transfer buffers for this driver */
	ctx->xusb_data = (uint32_t *)tegrabl_alloc_align(TEGRABL_HEAP_DMA, 64, SETUP_DATA_BUFFER_SIZE);
	if (ctx->xusb_data == NULL) {
		pr_error("failed to allocate memory for xusb data buffer\n");
		e = TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
		goto fail;
	}
	memset(ctx->xusb_data, 0x0, SETUP_DATA_BUFFER_SIZE);
	ctx->xusb_data_dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0,
		(void *)ctx->xusb_data, SETUP_DATA_BUFFER_SIZE, TEGRABL_DMA_TO_DEVICE);

fail:
	return e;
}

tegrabl_error_t xhci_start(struct xusb_host_context *ctx)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	uint32_t port_no = 0;
	uint32_t dev_speed = 0;
	uint32_t port_device_bitmap = 0;

	/* after power up, the device should be reset already      */
	/* To be safe, reset it again at here. This is last chance */
	/* to bring controller clean state.                        */
	err = xusbh_xhci_reset();
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}

	err = init_common_data_struct(ctx);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to initialize usbh data structure\n");
		return err;
	}

	err = xusbh_xhci_run(ctx);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}

	/* enable interrupt and clean all pending events */
	err = xusbh_enable_interrupt(ctx);
	if (err != TEGRABL_NO_ERROR) {
		return err;
	}

	/* Enable slot */
	err = xusbh_get_slot_id(ctx);
	if (err != TEGRABL_NO_ERROR || ctx->slot_id == 0) {
		pr_warn("Usb slot is NOT ready\n");
		return TEGRABL_ERROR(TEGRABL_ERR_NOT_CONNECTED, 0);
	}

	/* Initialize memory for device priv data */
	ctx->dev_priv[ctx->slot_id] = tegrabl_malloc(sizeof(struct xusb_dev_priv));
	if (ctx->dev_priv[ctx->slot_id] == NULL) {
		pr_error("Failed to allocate memory for device with idx: %u\n", ctx->slot_id);
		return TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 0);
	}
	ctx->curr_dev_priv = ctx->dev_priv[ctx->slot_id];
	memset(ctx->curr_dev_priv, 0, sizeof(struct xusb_dev_priv));

	err = init_data_struct(ctx);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to initialize usb device priv data structures\n");
		return err;
	}

	/* Set device context in DCBAA */
	ctx->dcbaa[ctx->slot_id] = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0,
		(void *)ctx->curr_dev_priv->dev_context, DEV_CONTEXT_SIZE, TEGRABL_DMA_TO_DEVICE);

	/* configure endpoint and enumerate device */
	/* since there is no hub for first USB device, route string will be 0 */
	ctx->curr_dev_priv->route_string = 0;
	ctx->curr_dev_priv->speed = ctx->root_port_speed;
	ctx->hub_slot_id = 0;
	xusbh_init_ep_ctx(ctx);
	err = xusbh_enumerate_device(ctx);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to enumerate USB device\n");
		goto fail;
	}

	if (ctx->curr_dev_priv->enum_dev.bDeviceClass == USB_CLASS_HUB) {
		ctx->hub_slot_id = ctx->slot_id;
	} else {
		/* Enumerated device is not hub, so skip below hub ports scan/boot code */
		goto fail;
	}

	/* Configure hub and detect ports with device connected */
	err = hub_init(ctx);
	if (err != TEGRABL_NO_ERROR) {
		pr_error("Failed to initialize hub\n");
		goto fail;
	}
	err = hub_detect_device(ctx, &port_device_bitmap);
	if (err != TEGRABL_NO_ERROR) {
		goto fail;
	}

	for (port_no = 1; port_no <= ctx->hub_num_ports; port_no++) {

		if (((0x1 << port_no) & port_device_bitmap) == 0) {
			pr_trace("No device connected on port %u, skip enumeration\n", port_no);
			continue;
		}

		/* Set slot id and current device ptr to hub */
		ctx->slot_id = ctx->hub_slot_id;
		ctx->curr_dev_priv = ctx->dev_priv[ctx->hub_slot_id];
		err = hub_reset_port(ctx, port_no, &dev_speed);
		if (err != TEGRABL_NO_ERROR) {
			/* Try next port */
			err = TEGRABL_NO_ERROR;
			continue;
		}
		if (dev_speed == XHCI_LOW_SPEED) {
			pr_info("Port %u device is low speed, skip enumeration\n", port_no);
			continue;
		}

		/* Enable slot for this device connected to HUB */
		ctx->slot_id = 0;  /* Set slot_id to 0 here so future code can reassign it */
		err = xusbh_get_slot_id(ctx);
		if (err != TEGRABL_NO_ERROR || ctx->slot_id == 0) {
			pr_error("USB slot is NOT ready\n");
			return TEGRABL_ERROR(TEGRABL_ERR_NOT_CONNECTED, 0);
		}

		/* Initialize memory for device */
		ctx->dev_priv[ctx->slot_id] = tegrabl_malloc(sizeof(struct xusb_dev_priv));
		if (ctx->dev_priv[ctx->slot_id] == NULL) {
			pr_error("Failed to allocate memory for device with idx: %u\n", ctx->slot_id);
			return TEGRABL_ERROR(TEGRABL_ERR_NO_MEMORY, 1);
		}
		ctx->curr_dev_priv = ctx->dev_priv[ctx->slot_id];
		memset(ctx->curr_dev_priv, 0, sizeof(struct xusb_dev_priv));

		err = init_data_struct(ctx);
		if (err != TEGRABL_NO_ERROR) {
			pr_error("Failed to initialize usb device priv data structure\n");
			return err;
		}

		/* Set device context in DCBAA */
		ctx->dcbaa[ctx->slot_id] = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0,
			(void *)ctx->curr_dev_priv->dev_context, DEV_CONTEXT_SIZE, TEGRABL_DMA_TO_DEVICE);

		/* configure endpoint and enumerate device */
		ctx->curr_dev_priv->speed = dev_speed;
		ctx->curr_dev_priv->route_string = port_no;
		xusbh_init_ep_ctx(ctx);
		err = xusbh_enumerate_device(ctx);
		if (err != TEGRABL_NO_ERROR) {
			if (err != TEGRABL_ERROR(TEGRABL_ERR_NOT_FOUND, 0)) {
				pr_error("Failed to enumerate USB device\n");
			}
			continue;
		}
		break;  /* Enumeration successful, exit the loop */
	}

fail:
	return err;
}

#define MAX_TX_LENGTH   0x10000
tegrabl_error_t tegrabl_xhci_xfer_data(struct xusb_host_context *ctx,
				uint8_t ep_id, void *buffer, uint32_t *length)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;
	struct xhci_ring *ep_ring;
	struct normal_trb *trb;
	struct TRB *t;
	uint32_t size;
	dma_addr_t dma;
	uint32_t transfer_size;
	uint32_t need_trbs;
	uint32_t count;
	uint32_t total_packets;
	enum usb_dir dir;

	if (*length == 0) {
		return err;
	}

	dir = ((ep_id & 0x80) == 0x80) ? USB_DIR_IN : USB_DIR_OUT;
	ep_id = (ep_id & 0x7f) * 2 + dir;
	ep_ring = (struct xhci_ring *)&ctx->curr_dev_priv->ep_ring[(uint32_t)dir + 1];

	/* prepare normal trbs */
	dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)buffer, *length, TEGRABL_DMA_TO_DEVICE);

	/* extra 1 trb is needed when buffer is not at 64K bindary */
	transfer_size = MAX_TX_LENGTH - ((uint32_t)dma & (MAX_TX_LENGTH - 1));
	need_trbs = 0;
	if (transfer_size != 0) {
		need_trbs = 1;
	}

	if (transfer_size < *length) {
		transfer_size = *length - transfer_size;
		need_trbs += (transfer_size + MAX_TX_LENGTH - 1) >> 16;
	}
	pr_debug("%s - need %d trbs for xfer\n", __func__, (int)need_trbs);

	total_packets = DIV_ROUND_UP((*length), ctx->curr_dev_priv->enum_dev.ep[dir].packet_size);
	if (need_trbs == 1) {
		transfer_size = *length;
	} else {
		transfer_size = MAX_TX_LENGTH - ((uint32_t)dma & (MAX_TX_LENGTH - 1));
	}

	count = 0;
	ep_ring->enque_start_ptr = ep_ring->enque_curr_ptr;
	ep_ring->start_cycle_state = ep_ring->cycle_state;
	size = 0;
	while (count < need_trbs) {
		trb = (struct normal_trb *)ep_ring->enque_curr_ptr;
		if (ep_ring->enque_curr_ptr != ep_ring->enque_start_ptr) {
			trb->cycle_bit = ep_ring->cycle_state;
		}
		trb->data_buffer_lo = U64_TO_U32_LO(dma);
		trb->data_buffer_hi = U64_TO_U32_HI(dma);
		dma += transfer_size;
		trb->trb_tfr_len = transfer_size;
		trb->td_size = (total_packets - ((size + transfer_size) / ctx->curr_dev_priv->enum_dev.ep[dir].packet_size));
		if (count != (need_trbs - 1)) {
			trb->CH = 1;
		} else {
			trb->td_size = 0;
			trb->IOC = 1;
		}
		if (dir == USB_DIR_IN)
			trb->ISP = 1;
		trb->trb_type = TRB_NORMAL;

		size += transfer_size;
		dma += transfer_size;
		transfer_size = *length - size;
		if (transfer_size > MAX_TX_LENGTH) {
			transfer_size = MAX_TX_LENGTH;
		}
		count++;
		set_enq_ptr(ep_ring);

		t = (struct TRB *)trb;
		pr_debug("xfer[%d] %08x  %08x  %08x  %08x\n", (int)count, t->field[0], t->field[1],
				t->field[2], t->field[3]);
	}
	ep_ring->dma = tegrabl_dma_map_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)ep_ring->enque_start_ptr,
					  sizeof(struct TRB) * need_trbs, TEGRABL_DMA_TO_DEVICE);

	/* prepare ep context */
	/* ring ep doorbell and wait*/
	if (dir == USB_DIR_IN) {
		prepare_ep_ctx(ctx, (ep_id - 1), EP_TYPE_BULK_IN);
		err = xhci_ring_doorbell_wait(ctx, EP_TYPE_BULK_IN, (ep_id - 1));
	} else {
		prepare_ep_ctx(ctx, (ep_id - 1), EP_TYPE_BULK_OUT);
		err = xhci_ring_doorbell_wait(ctx, EP_TYPE_BULK_OUT, (ep_id - 1));
	}
	if (err != TEGRABL_NO_ERROR) {
		err = TEGRABL_ERROR(TEGRABL_ERR_NOT_SUPPORTED, 0);
	}

	if (dir == USB_DIR_IN) {
		tegrabl_dma_unmap_buffer(TEGRABL_MODULE_XUSB_HOST, 0, (void *)buffer, *length,
					 TEGRABL_DMA_FROM_DEVICE);
	}

	return err;
}

void xhci_power_down_controller(void)
{
	xhci_power_down_bias_pad();
	xhci_power_down_usb2_padn();
	xhci_usb3_phy_power_off();
}

tegrabl_error_t tegrabl_usbh_close(void)
{
	tegrabl_error_t err = TEGRABL_NO_ERROR;

	/* Powergate XUSBA, XUSBC partitions and assert reset to XUAB PADCTL, XUABA, XUSBC
	 * partitions before jumping to kernel */
	tegrabl_car_rst_set(TEGRABL_MODULE_XUSB_HOST, 0);
	tegrabl_car_rst_set(TEGRABL_MODULE_XUSB_SS, 0);
	tegrabl_car_rst_set(TEGRABL_MODULE_XUSB_DEV, 0);

	/*
	 * Unfortunately, reset pad control is not an option due to we support virtualization
	 * and bpmp will reject the reset request on t194.
	 * Power down pads in bootloader before jumping to the kernel.
	 */

	xhci_power_down_controller();

	err = tegrabl_usb_powergate();
	if (err != TEGRABL_NO_ERROR) {
		pr_error("powergating XUSBA and XUSBC partitions failed!!\n");
		goto fail;
	}

fail:
	return err;
}
