/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 *
 */

/* Crypto system config file for target: Bootloader */

#ifndef __CRYPTO_SYSTEM_CONFIG_H1__
#define __CRYPTO_SYSTEM_CONFIG_H1__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include <tegrabl_error.h>
#include <tegrabl_malloc.h>
#include <tegrabl_timer.h>
#include <tegrabl_cache.h>
#include <tegrabl_debug.h>
#include <tegrabl_ast.h>
#include <tegrabl_dmamap.h>

#define ERR_BAD_STATE TEGRABL_ERR_INVALID_STATE
#define ERR_OUT_OF_RANGE TEGRABL_ERR_OUT_OF_RANGE
#define ERR_NOT_IMPLEMENTED TEGRABL_ERR_NOT_SUPPORTED
#define ERR_FAULT TEGRABL_ERR_FATAL
#define ERR_BAD_LEN TEGRABL_ERR_OUT_OF_RANGE
#define ERR_SIGNATURE_INVALID TEGRABL_ERR_VERIFY_FAILED
#define NO_ERROR TEGRABL_NO_ERROR
#define ERR_INVALID_ARGS TEGRABL_ERR_BAD_PARAMETER
#define ERR_NOT_VALID TEGRABL_ERR_INVALID
#define ERR_NOT_SUPPORTED TEGRABL_ERR_NOT_SUPPORTED
#define ERR_NO_MEMORY TEGRABL_ERR_NO_MEMORY
#define ERR_GENERIC TEGRABL_ERR_NOT_SUPPORTED
#define ERR_NOT_ALLOWED TEGRABL_ERR_NO_ACCESS
#define ERR_TIMED_OUT TEGRABL_ERR_NO_ACCESS
#define ERR_TOO_BIG TEGRABL_ERR_NO_ACCESS
#define ERR_NOT_FOUND TEGRABL_ERR_NOT_FOUND
#define status_t uint32_t
#define ROUNDUP(a, b) (((a) + ((b) - 1)) & ~((b) - 1))
#define ASSERT(x) TEGRABL_ASSERT(x)
#define DEBUG_ASSERT(x)
#define CRITICAL TEGRABL_LOG_CRITICAL

/* Define 0 if SCC mechanisms are not usable in the subsystem.
 *  All SCC protection systems turned on by default now.
 */
#define HAVE_SE_SCC		1 // Enables SE AES SCC features (default:1)
#define HAVE_PKA1_SCC_DPA	1 // PKA1 SCC features (default:1)
#define HAVE_PKA1_BLINDING	0 // PKA1 priv key operation blinding (default:1)
#define HAVE_JUMP_PROBABILITY	1 // PKA1 jump probability (default:1)
#define HAVE_SE_ASYNC			1
#define HAVE_SE_ASYNC_SHA		1

// For evaluating PKA1 KEY LOADING
// PKA1 RSA key loading seems to work
// ECC key loading code NOT TESTED/DOES NOT WORK for now
//
#define HAVE_PKA1_LOAD	1

#define HAVE_DEPRECATED 0	  // If non-zero => compiles in deprecated functions for
				  // backwards compatibility.
				  // Currently: AES && RSA keyslot clear functions
				  // These have new versions with device_id instead of engine_hint
				  // parameters. Define as zero unless you need the old versions...

#define HAVE_DYNAMIC_KEY_OBJECT 1 // In-kernel crypto context (crypto_context_t) can optionally
				  // be split into two objects:
				  //  the context and a separate key object;
				  // This keeps crypto_context_t much smaller since the major
				  // portion of it was the key object (more than a kilobyte)
				  //
				  // This benefits algorithms that do not use keys, specifically
				  //  DIGESTS (which are used e.g. in EDDSA verification)
				  //
				  // Reduces the crypto_context_t object size to =>
				  //  7*sizeof(uint32_t)+2*sizeof(void *) == 44 bytes;
				  //  (enabling USER_MODE will increase that size with 16 bytes
				  //   in Trusty OS)

#define HAVE_NC_REGIONS	0	// Handle non-contiguous phys memory regions for SE engine DMA
				// (XXX Not complete: AES cipher data is COPIED around =>FIXME)

#define HAVE_USER_MODE	0	// Support secure AARCH-32 user mode (Trusty TA's)

#define HAVE_SE_AES	1	// Support AES with the SE AES engine(s)
#define HAVE_SE_SHA	1	// Support SHA digests with the SE SHA engine
#define HAVE_CMAC_AES	1	// Support CMAC-AES macs with the SE AES engine (requires HAVE_SE_AES)
#define HAVE_HMAC_SHA	0	// Support HMAC-SHA-* macs with the SE SHA engine (requires HAVE_SE_SHA)
#define HAVE_SE_RANDOM	1	// Support SE DRNG random numbers (requires HAVE_SE_AES)
#define HAVE_SE_RSA	1	// Support SE unit RSA engine
#define HAVE_PKA1_RSA	1	// Add PKA1 RSA (Elliptic unit with up to 4096 bit RSA keys)
#define HAVE_PKA1_ECC	1	// Add PKA1 ECC (Elliptic unit with for ECDH, ECDSA, EC point operations)
#define HAVE_FPGA 0522

#define HAVE_PKA1_ECDH	0	// Add PKA1 ECDH operation (requires HAVE_PKA1_ECC)
#define HAVE_PKA1_ECDSA	1	// Add PKA1 ECDSA (requires HAVE_PKA1_ECC, HAVE_PKA1_MODULAR)
#define HAVE_PKA1_MODULAR 1	// Add PKA1 modular library (e.g. mandatory for ECDSA, requires HAVE_PKA1_ECC)

#define HAVE_RSA_CIPHER	0	// Add RSA encrypt/decrypt (plain && padding crypto)
#define HAVE_PLAIN_DH	0	// Diffie-Hellman-Merke key agreement (modular exponentiation,
				//   requires HAVE_RSA_CIPHER)

#define HAVE_RSAPSS_SIGNING	      0 // XXX Not implemented yet
#define HAVE_ECDSA_SIGNING	      0 // XXX Not implemented yet

#define HAVE_RSA_PKCS_VERIFY	      1	// RSA-PKCS#1v1.5 signature support
#define HAVE_RSA_PKCS_SIGN	      0 // XXX Not implemented yet

#if HAVE_PKA1_ECC

#ifndef HAVE_P521
#define HAVE_P521 		      0 // XXX NIST-P521 curve not fully implemented yet...
#endif

#define HAVE_NIST_CURVES	      1 // Support NIST EC curves
#define HAVE_NIST_CURVE_ALL	      0 // Support all NIST EC curves (or select individual curves)
#define HAVE_NIST_P256 1
#define HAVE_NIST_P192 0
#define HAVE_NIST_P224 0
#define HAVE_NIST_P384 0

#define HAVE_BRAINPOOL_CURVES	      0	// Support brainpool EC curves
#define HAVE_BRAINPOOL_CURVES_ALL     0	// Support brainpool EC curves (or select individual curves)

#if HAVE_BRAINPOOL_CURVES
#define HAVE_BRAINPOOL_TWISTED_CURVES 1		// Support brainpool EC twisted curves
#define HAVE_BRAINPOOL_TWISTED_CURVES_ALL 1	// Support brainpool EC twisted curves (or select individual curves)
#endif

#define HAVE_ELLIPTIC_20 1	// Code running in T194 with Elliptic-2.0 FW (with new curves and algos)
#define HAVE_PKA1_ED25519 1	// Add PKA1 ED25519 (requires HAVE_PKA1_ECC, HAVE_PKA1_MODULAR)

#ifndef HAVE_ELLIPTIC_20
#define HAVE_ELLIPTIC_20 0	// Code running in T194 with Elliptic-2.0 FW (with new curves and algos)
#endif

#if HAVE_ELLIPTIC_20

#ifndef HAVE_PKA1_ED25519
#define HAVE_PKA1_ED25519 0	// Add PKA1 ED25519 (requires HAVE_PKA1_ECC, HAVE_PKA1_MODULAR)
#endif

#ifndef HAVE_PKA1_X25519
#define HAVE_PKA1_X25519 0	// Add PKA1 X25519 (requires HAVE_PKA1_ECC)
#endif

// X25519 and ED25519 requires new PKA1 FW => HAVE_ELLIPTIC_20 must be nonzero
#if HAVE_PKA1_ED25519
#define HAVE_CURVE_ED25519 1
#else
#define HAVE_CURVE_ED25519 0
#endif

#if HAVE_PKA1_X25519
#define HAVE_CURVE_C25519 1
#else
#define HAVE_CURVE_C25519 0
#endif

#else /* HAVE_ELLIPTIC_20 */

/* This is unconditional because no support can exist */
#define HAVE_CURVE_ED25519 0
#define HAVE_CURVE_C25519  0
#define HAVE_PKA1_ED25519 0
#define HAVE_PKA1_X25519 0

#endif /* HAVE_ELLIPTIC_20 */

#endif /* HAVE_PKA1_ECC */

#ifndef HAVE_DEPRECATED
#define HAVE_DEPRECATED 0
#endif

/* Optional support for MD5 digest and HMAC-MD5 (set in makefile) */
#ifndef HAVE_MD5
#define HAVE_MD5	0	// Set 0 to drop MD5/HMAC-MD5 support
#endif

#define SE_NULL_SHA_DIGEST_FIXED_RESULTS 0	// Get correct results from sha-*(<NULL INPUT>)

/* Define non-zero to see each full operation memory allocation
 * (from beginning of INIT to end of RESET)
 */
#define TEGRA_MEASURE_MEMORY_USAGE 0	// Measure runtime memory usage

// Do not turn on the following yet (not implemented)

/* TODO => I have no way to test this (this is incomplete!!!) */
#define HAVE_SE1	0	// (untested) => secondary SE1 unit

#define HAVE_PKA1_TRNG	0	// Add PKA1 TNG (entropy seed generator)

/* TODO => I have not implemented these yet */
#define HAVE_PKA1_RNG	0	// (Not implemented yet) => Add PKA1 DRNG unit

/****************************/

#define __TEGRA__	1	// (used in md5.c only; XXX should use HOST_LITTLE_ENDIAN or some such instead)

#ifndef KERNEL_TEST_MODE
#define KERNEL_TEST_MODE 0
#endif

#ifndef SE_RD_DEBUG
#define SE_RD_DEBUG	0	// Compile with R&D code (allow override by makefile for now)
#endif

#if KERNEL_TEST_MODE
#if SE_RD_DEBUG == 0
#error "SE_RD_DEBUG must be nonzero to enable test code in Trusty"
#endif
#endif

/* Trusty specific macro to support logging */
#define TRUSTY_LOG(level, msg,...)

/* SE driver log macros for Trusty */
#define LOG_INFO(msg,...)
#define LOG_ERROR(msg,...)
#define LOG_ALWAYS(msg,...)
#define dprintf(level, msg,...)

#define ERROR_MESSAGE LOG_ALWAYS

/* get a 32 bit address from a 64 bit value in a TA context,
 * and a 64 bit address in a kernel context
 * These are NOPs in our code for now...
 */
/* No userspace contexts supported by this driver */
#define GET_DOMAIN_ADDR(domain, addr) (addr)

/* This is same as kvaddr_to_paddr(), but for some reason there is now
 * and static inline function wrapper for it => use the new version.
 *
 * Unfortunately the Trusty function prototype does not have const qualifier for the arg.
 */
#define DOMAIN_VADDR_TO_PADDR(domain, addr) tegrabl_dma_va_to_pa(TEGRABL_MODULE_SE, (void *)(uintptr_t)addr);

/* Get CACHE_LINE from <arch/defines.h> or define is for your CPU
 */
#ifndef CACHE_LINE
#define CACHE_LINE 64U	/* Cortex A-57 and 18x && 19x have 64 byte cache line */
#endif

#if CACHE_LINE == 64
#undef CACHE_LINE
#define CACHE_LINE 64U	/* misra compatibility */
#endif

#if CACHE_LINE == 32
#undef CACHE_LINE
#define CACHE_LINE 32U	/* misra compatibility */
#endif

/* For aligning data objects WRITTEN to by the SE DMA engine; in
 * practice this is relevant only for AES related OUTPUT BUFFERS
 * (Nothing else is written with the DMA).
 *
 * E.g. SE DRNG output is also affected, but that is also generated by
 * SE AES engine and written by DMA. But e.g. CMAC is not affected
 * because the CMAC output is not written by DMA, even though it is
 * calculated with AES engine.
 *
 * If the buffers are not aligned you can get them aligned by the
 * compiler using e.g. like this:
 *
 * DMA_ALIGN_DATA uint8_t my_output_buffer[ DMA_ALIGN_SIZE(MY_BUFFER_SIZE) ];
 *
 * If the AES buffers are not cache line aligned there is VERY BIG
 * RISK that the data will get corrupted after the DMA has written
 * the correct result to phys memory.
 *
 * E.g. cache flushes would not prevent this in general case due to
 * cache prefetches. So, to prevent corruptions please always align
 * DMA output buffers (both by address and size)!
 */
#ifndef DMA_ALIGN_DATA
#define DMA_ALIGN_DATA __ALIGNED(CACHE_LINE)
#endif

/*
 * Required for aligning SE engine AES output buffers
 */
#define DMA_ALIGN_SIZE(size) ROUNDUP(size, CACHE_LINE)

/* Convert the value to ordinal type: it may be a virtual address or size */
#define IS_DMA_ALIGNED(value) ((((uintptr_t)(value)) & (CACHE_LINE - 1U)) == 0U)

#define TEGRA_READ32(a)     NV_READ32(a)
#define TEGRA_WRITE32(a, v) NV_WRITE32(a, v)

/* System physical and virtual address types */
#define PHYS_ADDR_T uint64_t
#define VIRT_ADDR_T uintptr_t
#define addr_t uintptr_t

// Max host address length for phys addresses
#define PHYS_ADDR_LENGTH 8

/* Word align objects in memory.
 *
 * Used e.g. for aligning EC curve parameter uint8_t arrays
 * which are accessed via (const uint32_t *)pointer.
 *
 * If CPU supports misaligned pointer accesses,
 * this does not need to be defined at all.
 */
#define ALIGN32 TEGRABL_ALIGN(sizeof(uint32_t))

/* Use a compiler fence before cache ops
 *
 * With GCC the following could be used as compiler fence =>
 */
#define CF do { __asm__ volatile("" ::: "memory"); } while(false)

#if !defined(CONFIG_ENABLE_DCACHE)

#define SE_CACHE_FLUSH(vaddr, size)
#define SE_CACHE_FLUSH_INVALIDATE(vaddr, size)
#define SE_CACHE_INVALIDATE(vaddr, size)

#elif defined(CONFIG_DCACHE_WRITETHROUGH)

#define SE_CACHE_FLUSH(vaddr, size)
#define SE_CACHE_FLUSH_INVALIDATE(vaddr, size) \
	do { CF; tegrabl_arch_invalidate_dcache_range(vaddr,size); } while(false)
#define SE_CACHE_INVALIDATE(vaddr, size) \
	do { CF; tegrabl_arch_invalidate_dcache_range(vaddr,size); } while(false)

#else /* CONFIG_ENABLE_DCACHE */

#define SE_CACHE_FLUSH(vaddr, size) do { CF; tegrabl_arch_clean_dcache_range(vaddr, size); } while(false)
#define SE_CACHE_FLUSH_INVALIDATE(vaddr, size) do { CF; tegrabl_arch_clean_invalidate_dcache_range(vaddr, size); } while(false)
#define SE_CACHE_INVALIDATE(vaddr, size) do { CF; tegrabl_arch_invalidate_dcache_range(vaddr, size); } while(false)

#endif /* CONFIG_ENABLE_DCACHE */

/* Mutex lock for the SMP Trusty OS.
 *
 * The SW_MUTEX must be taken before the HW mutex and
 *  released after HW mutex.
 *
 * XXX TODO => check if return values (status_t) could be handled in some way!!!
 */
#define SW_MUTEX_TAKE
#define SW_MUTEX_RELEASE

struct engine_s;
void tegrabl_crypto_engine_reserve(const struct engine_s *engine);
void tegrabl_crypto_engine_release(const struct engine_s *engine);

#define SYSTEM_ENGINE_RESERVE(engine) tegrabl_crypto_engine_reserve(engine)
#define SYSTEM_ENGINE_RELEASE(engine) tegrabl_crypto_engine_release(engine)

#define SE_ERROR(x) x

#if SE_RD_DEBUG

/* XXX Add all valid address range bounds here.
 * XXX Need to check that a range is withing the defined physaddr range
 * XXX  => take the macro from dloader code, already written there
 */
/* 40 bit address range accessible by SE */
#define SE_PADDR_RANGE1_LOW_BOUND  0x0UL			/* XXX fix the lower bound */
#define SE_PADDR_RANGE1_HIGH_BOUND 0xFFFFFFFFFFFFFFFFFFUL	/* XXX fix the upper bound */

/* Rejects only NULL paddr for now in Trusty (XXX TODO: improve this) */
#define VALID_PADDR_SE_RANGE(paddr, size) ((paddr) != (PHYS_ADDR_T)NULL)

#define SE_PHYS_RANGE_CHECK(paddr, size)

/* Debug feature: Trace all register writes and reads if nonzero; shows
 * engine, register offset and name, values written and read
 *
 * If engine is SE0_AES1 the names may be for SE0_AES0 (they are
 *  offset to SE0_AES1 if AES1 is selected, but the debug strings
 *  do not change...)
 */
#ifndef TRACE_REG_OPS
#define TRACE_REG_OPS 0
#endif

/* If nonzero: trace code execution, output other trace data as well */
#ifndef MODULE_TRACE
#define MODULE_TRACE  0
#endif

#if MODULE_TRACE

/* To get location info in error messages when debugging with trace:
 * use ltrace for this as well...
 */
#undef  ERROR_MESSAGE
#define ERROR_MESSAGE LTRACEF

#define LOG_HEXBUF(name,base,len) do { LTRACEF(""); dump_data_object(name, (const uint8_t *)base, len, len); } while(false)
#endif

/* Trusty logs written to UART directly, not via linux shared buffers (if excessive logs
 * enabled the shared buffers overflow constantly)
 */
#define JUKI_RD_MODE_LOG_UART (MODULE_TRACE || TRACE_REG_OPS || KERNEL_TEST_MODE)

#define DUMP_DATA_PARAMS(name,flags,dp)					\
  { struct se_data_params *__di = dp;					\
    if (__di) {								\
      LTRACEF("%s [" #dp "(%p)] => src=%p input_size=%u: dst=%p output_size=%u\n", \
	      name,__di,__di->src,__di->input_size,__di->dst,__di->output_size); \
      if (((flags) & 0x1U) && __di->src) { LOG_HEXBUF(" " #dp "->src:", __di->src, __di->input_size); } \
      if (((flags) & 0x2U) && __di->dst) { LOG_HEXBUF(" " #dp "->dst:", __di->dst, __di->output_size); } \
    }									\
  }

#else /* SE_RD_DEBUG */

#define SE_PHYS_RANGE_CHECK(paddr, size)
#define TRACE_REG_OPS 0
#define MODULE_TRACE  0

#undef LTRACEF

#endif	/* SE_RD_DEBUG */

#define PAGE_MASK (PAGE_SIZE - 1U)

#ifndef LOG_HEXBUF
#define LOG_HEXBUF(name,base,len)
#endif

#ifndef DEBUG_ASSERT_PHYS_DMA_OK
/* This means all addresses are valid for SE DMA */
#define DEBUG_ASSERT_PHYS_DMA_OK(addr,size)
#endif

#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT(x)
#endif

#ifndef DUMP_DATA_PARAMS
#define DUMP_DATA_PARAMS(name, flags, dp)
#endif

#ifndef LTRACEF
#define LTRACEF(...)
#endif

/* Perf monitor the crypto ops from context setup to dofinal() completed */
#define TEGRA_MEASURE_PERFORMANCE 0

/* Add platform specific code for microsecond time functions.
 * These do not exist in the Trusty OS...
 *
 * Added uSec functions, but do not use the udelay() to
 *  slow down the device status register polling (also added
 *  usec performance timing to the crypto context level operations
 *  and these numbers are affected by delay loops => disabled udelay())
 */
#define HAVE_USEC_COUNTER 1

#if HAVE_USEC_COUNTER

uint32_t usec_counter(void);
void     udelay(uint32_t n);

#if 0	// Do not use for now => better crypto perf.
#define TEGRA_UDELAY(usec) udelay(usec)
#endif

#define GET_USEC_TIMESTAMP usec_counter()
#endif /* HAVE_USEC_COUNTER */

#ifndef TEGRA_UDELAY

#define TEGRA_UDELAY(usec)

/* If there is no decent delay function, use larger
 * poll loop max value for PKA1 operation wait complete.
 */
#define TEGRA_DELAY_FUNCTION_UNDEFINED 1

/* with zero defined delay value => TEGRA_UDELAY is not called
 * in the corresponding polling loop.
 */
#define TEGRA_UDELAY_VALUE_SE0_OP_INITIAL   0 // initial delay before polling the status
#define TEGRA_UDELAY_VALUE_SE0_OP	    0	// SE engine operation complete poll interval
#define TEGRA_UDELAY_VALUE_SE0_MUTEX	    0	// SE mutex lock poll interval

#define TEGRA_UDELAY_VALUE_PKA1_OP_INITIAL  0  // initial delay before polling the status
#define TEGRA_UDELAY_VALUE_PKA1_OP   	    0	// PKA1 operation complete poll interval
#define TEGRA_UDELAY_VALUE_PKA1_MUTEX	    0	// PKA1 mutex lock poll interval

#else

/* with zero defined delay value => TEGRA_UDELAY is not called in the
 * corresponding polling loop. Use minimal values for Trusty with
 * udelay() to avoid crypto performance drops.
 *
 * I guess there is not much pwr consumption diff in polling a SE
 * status register and polling a usec timer register => polling SE
 * status register is better for performance.
 */
#define TEGRA_UDELAY_VALUE_SE0_OP_INITIAL   1 // initial delay before polling the status
#define TEGRA_UDELAY_VALUE_SE0_OP	    1	// SE engine operation complete poll interval
#define TEGRA_UDELAY_VALUE_SE0_MUTEX	    1	// SE mutex lock poll interval

#define TEGRA_UDELAY_VALUE_PKA1_OP_INITIAL  1  // initial delay before polling the status
#define TEGRA_UDELAY_VALUE_PKA1_OP   	    1	// PKA1 operation complete poll interval
#define TEGRA_UDELAY_VALUE_PKA1_MUTEX	    1	// PKA1 mutex lock poll interval

#define TEGRA_DELAY_FUNCTION_UNDEFINED	    0

#endif /* TEGRA_UDELAY */

/* This needs to return aligned memory chunks which can be accessed
 * with SE DMA.
 *
 * Further, allocates physically contiguous memory pages (in case size > 4096)
 *
 * The SE driver WILL NOT WORK if the SE engine data input (or output)
 * is not physically contiquous from [ <base> .. <base> + <size - 1> ]
 * and HAVE_NC_REGIONS is also defined zero.
 *
 * Note:the SE engine is now configured to write out output of all
 * possible results to HW registers instead of memory. This has the
 * benefit that the caches do not need to be cleared and data does not
 * need to be cache line aligned for accessing the output data written
 * by SE HW.
 *
 * The AES block cipher output is unfortunately still an exception
 * (AES output => to memory). [CMAC-AES is written to registers, as
 * well as SHA and RSA, so CMAC is not an issue].
 *
 * This means that AES output buffers need special care
 * and attention.
 *
 * NOTE: SE DRNG is OK (all cases handled), so AES cipher output is now the
 * only remaining issue.
 *
 * Since SE HW no longer has linked list supports in T186 => need to
 * implement something like that in SW for AES input and output handling.
 * TODO => FIXME!!!!
 */

#define GET_CONTIGUOUS_ZEROED_MEMORY(align, size) \
  ({ void *__buf = tegrabl_alloc_align(TEGRABL_HEAP_DMA, align, size); if (NULL != __buf) { memset(__buf, 0, size); } __buf; })

#define RELEASE_CONTIGUOUS_MEMORY(addr) do { if (NULL != (addr)) { tegrabl_dealloc(TEGRABL_HEAP_DMA, addr); } } while(0)

/* Allocating zeroed memory which does not need not be contiguous or aligned
 * (i.e. it is not accessed by DMA engine with phys addresses)
 */
#define GET_ZEROED_MEMORY(size)	tegrabl_calloc(1, size)

#define RELEASE_MEMORY(addr) do { if (NULL != (addr)) { tegrabl_free(addr); } } while(0)

#if TEGRA_MEASURE_MEMORY_USAGE
#define MEASURE_MEMORY_START(purpose)					\
  do { bool _mes_start = measure_mem_start(purpose, __func__, __LINE__); \
    if (!BOOL_IS_TRUE(_mes_start)) {					\
      ERROR_MESSAGE("Mem measure start failed: %s\n", __func__); goto fail; \
    } } while(false)
#define MEASURE_MEMORY_STOP   measure_mem_stop(__func__, __LINE__)

/* Otherwise the circular log buffer will overflow */
#if JUKI_RD_MODE_LOG_UART == 0
#undef JUKI_RD_MODE_LOG_UART
#define JUKI_RD_MODE_LOG_UART 1
#endif

#else

#define MEASURE_MEMORY_START(purpose)
#define MEASURE_MEMORY_STOP
#endif

/* SYSTEM_MAP_DEVICE is a function name => override it in this define
 * or define a function called SYSTEM_MAP_DEVICE (what ever you prefer).
 *
 * The function is called at most MAX_CRYPTO_DEVICES times with
 * device_id values set from 0..(MAX_CRYPTO_DEVICES-1U) matching the
 * numeral values of se_cdev_id_t enums. If a device if not configured,
 * it is not called to be initialized.
 *
 * Currently the values are like this, but please check se_cdev_id_t
 * for the actual mapping, each with it's own base address:
 *
 *  SE_CDEV_SE0  = 0,
 *  SE_CDEV_PKA1 = 1,
 *  SE_CDEV_RNG1 = 2,
 *  SE_CDEV_SE1  = 3,
 *
 * Must return non-zero on error.
 *
 * If the function returns 0 and the *base_p == NULL then the device does not
 * exist in that system.
 */
status_t bpmp_map_crypto_device(uint32_t device_id, void **base_p);

#define SYSTEM_MAP_DEVICE bpmp_map_crypto_device

/* Map SE0, SE1, PKA1 and RNG1 and set the base address to the arg 
 * in Trusty. Returning NULL base address is treated like engine does not exist,
 * disabling that engine.
 *
 * Return non-zero value to indicate an error (or just panic the system).
 */
status_t SYSTEM_MAP_DEVICE(uint32_t device_id, void **base_p);

#if HAVE_USER_MODE
/* Trusty function to find the app local storage crypto context object
 * from the trusty application object.
 *
 * This is used only in Trusty TA specific code.
 * If you have no TA => ignore, this function is not called.
 */
int get_crypto_als_slot(void);
#endif


#endif /* __CRYPTO_SYSTEM_CONFIG_H__ */
