/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Main entry point to the OS. Initializes modules in order and creates
 * the default thread.
 */
#include <compiler.h>
#include <debug.h>
#include <string.h>
#include <app.h>
#include <arch.h>
#include <platform.h>
#include <target.h>
#include <lib/heap.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <tegrabl_debug.h>
#include <tegrabl_malloc.h>

extern void *__ctor_list;
extern void *__ctor_end;
extern int __bss_start;
extern int __version_start;
extern int _end;

static int bootstrap2(void *arg);

extern void kernel_init(void);

static void call_constructors(void)
{
	void **ctor;

	ctor = &__ctor_list;
	while (ctor != &__ctor_end) {
		void (*func)(void);

		func = (void (*)(void))*ctor;

		func();
		ctor++;
	}
}

/* called from arch code */
void lk_main(void) __NO_RETURN __EXTERNALLY_VISIBLE;
void lk_main(void)
{
	/*char *version_string = (char *)&__version_start;*/
	inc_critical_section();

	// get us into some sort of thread context
	thread_init_early();

	// early arch stuff
	lk_init_level(LK_INIT_LEVEL_ARCH_EARLY - 1);
	arch_early_init();

	// do any super early platform initialization
	lk_init_level(LK_INIT_LEVEL_PLATFORM_EARLY - 1);
	platform_early_init();

	// do any super early target initialization
	lk_init_level(LK_INIT_LEVEL_TARGET_EARLY - 1);
	target_early_init();

	/* dprintf(INFO, "Cboot Version: %s\n", version_string); */

	// deal with any static constructors
	dprintf(SPEW, "calling constructors\n");
	call_constructors();

	// bring up the kernel heap
	tegrabl_printf("initializing heap\n");
	lk_init_level(LK_INIT_LEVEL_HEAP - 1);

	if (platform_init_heap() != TEGRABL_NO_ERROR)
		panic("Heap Init Failed\n");

	// initialize the kernel
	lk_init_level(LK_INIT_LEVEL_KERNEL - 1);
	kernel_init();

	lk_init_level(LK_INIT_LEVEL_THREADING - 1);
	// create a thread to complete system initialization
	dprintf(SPEW, "creating bootstrap completion thread\n");

	thread_t *t = thread_create("bootstrap2", &bootstrap2, NULL,
				    DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);

	thread_detach(t);
	thread_resume(t);


	// become the idle thread and enable interrupts to start the scheduler
	thread_become_idle();
}

static int bootstrap2(void *arg)
{
	dprintf(SPEW, "top of bootstrap2()\n");

	lk_init_level(LK_INIT_LEVEL_ARCH - 1);
	arch_init();

	// Call all functions at LEVEL_ARCH
	lk_init_level(LK_INIT_LEVEL_PLATFORM - 1);

	// initialize the rest of the platform
	dprintf(SPEW, "initializing platform\n");
	platform_init();

	// Call all functions at LEVEL_PLATFORM
	lk_init_level(LK_INIT_LEVEL_TARGET - 1);

	// initialize the target
	dprintf(SPEW, "initializing target\n");
	target_init();

	// Call all functions at LEVEL_TARGET
	lk_init_level(LK_INIT_LEVEL_APPS - 1);

#if 0
	// Initialize OF layer
	dprintf(SPEW, "initializing OF layer\n");
	of_init_all_drivers();
#endif

	dprintf(SPEW, "calling apps_init()\n");
	apps_init();

	// Call all the rest
	lk_init_level(LK_INIT_LEVEL_LAST);
	return 0;
}

