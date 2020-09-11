/*
 * Copyright (c) 2014 - 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef __DT_COMMON_H
#define __DT_COMMON_H

#include <common.h>
#include <libfdt.h>
#include <sys/types.h>
#include <boot_arg.h>

#define BL_DTB_PARTITION_NAME "RP1"
#define KERNEL_DTB_PARTITION_NAME "DTB"

/**
 * @brief Structure to hold metadata for a dtb
 *
 * @param partition_name Name of the partition in storage eg: RP1, DTB, etc
 * @param dtb_name Name string for the dtb. eg: bldtb, kerneldtb, etc
 * @param load_address Load address of the dtb in memory
 * @param override_shim Shim override decision for this dtb
 * @param boot_arg Boot arg type
 */
struct dt_meta_data {
	const char *partition_name;
	const char *dtb_name;
	uintptr_t load_address;
	bool override_shim;
	boot_arg_type boot_arg;
};

/**
 * @brief Types of DTBs being used.
 */
typedef uint32_t dtb_type_t;
#define DTB_BOOTLOADER 0
#define DTB_KERNEL 1
	/* cardinality of this enum */
#define DTB_COUNT 2

/**
 * @brief Get the next node's offset
 *
 * @param fdt device tree handle
 * @param node Offset of current node
 */
static inline int32_t dt_next_node(const void *fdt, int node)
{
	return fdt_next_node(fdt, node, NULL);
}

/**
 * @brief iterate through all nodes starting from a certain node
 *
 * @param fdt device tree handle
 * @param node integer iterator over dt nodes
 * @param start_node offset of the node to begin traversal with
 */
#define dt_for_each_node_from(fdt, node, start_node)	\
	for (node = start_node;								\
		(node) >= 0;					\
		node = dt_next_node(fdt, node))

/**
 * @brief Iterate through all nodes
 *
 * @param fdt Device Tree handle
 * @param node Integer iterator over DT nodes
 */
#define dt_for_each_node(fdt, node) \
	dt_for_each_node_from(fdt, node, dt_next_node(fdt, -1))

/**
 * @brief Iterate the properties under a certain node
 *
 * @param fdt Device tree handle
 * @param offset integer iterator over each property
 * @param start_node offset of the node, whose properties to traverse
 */
#define dt_for_each_prop_of(fdt, offset, node) \
	for (offset = fdt_first_property_offset(fdt, node); \
		(offset >= 0); \
		(offset = fdt_next_property_offset(fdt, offset)))

/**
 * @brief Read an <addr,size> tuple in "reg" property
 *		 of a device node specified at particular offset.
 *
 * @param nodeoffset The offset to the particular node.
 * @param index The index of <addr,size> tuple to be read.
 * @param addr The addr part of the specified element will be read into
 *		  this field. address will not be read if this is NULL.
 * @param size The size part of the specified element will be read into
 *		  this field. size will not be read if this is NULL.
 *
 * @return NO_ERROR if the specified addr and/or size is read successfully,
 *		   otherwise appropriate error.
 */
status_t dt_read_reg_by_index(int nodeoffset, uint32_t index, uintptr_t *addr,
															uintptr_t *size);

/**
 * @brief Read the interrupt numbers from a device node.
 *
 * @param nodeoffset The offset to the particular node.
 * @param intr The pointer where the read interrupts are being read.
 * @param count Number of interrupts to be read int 'intr' param.
 *
 * @return NO_ERROR if the specified number of interrupts is read
 *		   successfully, otherwise appropriate error.
 */
status_t dt_get_gic_intr(int nodeoffset, uint32_t *intr, uint32_t count);

/**
 * @brief Returns the pointer to fdt.
 *
 * @param The required type of dtb handle which is to be returned
 *
 * @return The handle of fdt.
 *
 */
void *dt_get_fdt(dtb_type_t type);

/**
 * @brief Returns the pointer to fdt.
 *
 * @return The handle of fdt.
 *
 * This API is deprecated. It only exists due to legacy reasons.
 * Please do not use this anymore. Instead, use dt_get_fdt()
 *
 */
static inline void *dt_get_fdt_handle(void)
{
	/* Most of the calls are for bootloader dtb. Hence, sticking to this
       inline call until this API is deprecated */
	return dt_get_fdt(DTB_BOOTLOADER);
}

/**
 * @brief Returns the pointer to fdt metadata structure
 *
 * @param type The required type of dtb handle which is to be returned
 *
 * @return The pointer to fdt metadata structure
 *
 */
struct dt_meta_data dt_get_fdt_metadata(dtb_type_t type);

/**
 * @brief Get the number of children of a node
 *
 * @param fdt Handle to device tree blob
 * @param node_offset Handle to the parent node
 *
 * @return Number of children of the parent node
 */
uint32_t dt_get_child_count(const void *fdt, int node_offset);

/**
 * @brief Retrieve child node offset of a node with the given name
 *
 * @param fdt Handle to device tree blob
 * @param start_offset Node in FDT to start the search from.
 * @param name Name of the child node
 * @param res Callee filled. On success, holds offset of the child node that
 *		      matches the criteria.
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_get_child_with_name(const void *fdt, int start_offset, char *name,
																	int *res);

/**
 * @brief Get the next child of a node
 *		  You most certainly do not need to use this function.
 *		  Use the iterator macros instead
 *
 * @param fdt Handle to device tree blob
 * @param node_offset Handle to the parent node
 * @param prev_child Handle to the last sibling visited
 * @param res Callee filled. On success, holds offset of the next child node
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_get_next_child(const void *fdt, int node_offset, int prev_child, int *res);

#define dt_for_each_child(fdt, parent, child)		\
	for (child = fdt_first_subnode(fdt, parent);	\
		(child) != -FDT_ERR_NOTFOUND;				\
		child = fdt_next_subnode(fdt,child))

/**
 * @brief Read a property indexed in an array
 *		  Use dt_get_prop_array if you want the whole array of values
 *
 * @param fdt Handle to device tree blob
 * @param node_offset Handle to the parent node
 * @param prop_name Name of the property to read
 * @param sz Size of each element in array (in bytes - 1/4)
 * @param idx Index of the value in array
 * @param res Pointer to collect the result
 *
 * @return ERR_NOT_FOUND if the property is not available
 *		   NO_ERROR if success
 *		   On success, res points to the value retrieved from DTB
 */
status_t dt_get_prop_by_idx(const void *fdt, int node_offset, char *prop_name,
											size_t sz, uint32_t idx, void *res);

static inline status_t dt_get_prop_u8_by_idx(const void *fdt, int node,
									char *prop, uint32_t idx, void *res)
{
	return dt_get_prop_by_idx(fdt, node, prop, sizeof(uint8_t), idx, res);
}

static inline status_t dt_get_prop_u32_by_idx(const void *fdt, int node,
									char *prop, uint32_t idx, void *res)
{
	return dt_get_prop_by_idx(fdt, node, prop, sizeof(uint32_t), idx, res);
}

static inline status_t dt_get_prop(const void *fdt, int node_offset,
							char *prop_name, size_t sz, void *res)
{
	return dt_get_prop_by_idx(fdt, node_offset, prop_name, sz, 0, res);
}

static inline status_t dt_get_prop_u8(const void *fdt, int node,
										char *prop, void *res)
{
	return dt_get_prop_by_idx(fdt, node, prop, sizeof(uint8_t), 0, res);
}

static inline status_t dt_get_prop_u32(const void *fdt, int node,
										char *prop, void *res)
{
	return dt_get_prop_by_idx(fdt, node, prop, sizeof(uint32_t), 0, res);
}

/**
 * @brief Read an array property
 *
 * @param fdt Handle to device tree blob
 * @param node_offset offset of the DT node
 * @param prop_name Name of the property to read
 * @param sz Size of each element in array (in bytes - 1/4)
 * @param nmemb Number of array members to get. If 0, the entire array is
 *				returned. So prevent misuse, we make sure that num is non NULL
 *				when nmemb is zero.
 * @param res Pointer to the result values. Caller allocates the memory
 * @param num Number of array members successfully read; callee filled
 *
 * @return ERR_NOT_FOUND if the property is not available
 *		   ERR_NOT_SUPPORTED if an invalid sz value is passed
 *		   NO_ERROR if success
 *		   On success, the memory pointed to by res contains nmemb legitimate
 *		   values each of size sz. If the array does not have nmemb members,
 *		   num will point to the number of members read (< nmemb)
 */
status_t dt_get_prop_array(const void *fdt, int node_offset, char *prop_name,
						size_t sz, uint32_t nmemb, void *res, uint32_t *num);

static inline status_t dt_get_prop_u8_array(const void *fdt, int node,
					char *prop, uint32_t nmemb, void *res, uint32_t *num)
{
	return dt_get_prop_array(fdt, node, prop, sizeof(uint8_t), nmemb, res, num);
}

static inline status_t dt_get_prop_u32_array(const void *fdt, int node,
					char *prop, uint32_t nmem, void *res, uint32_t *num)
{
	return dt_get_prop_array(fdt, node, prop, sizeof(uint32_t), nmem, res, num);
}

/**
 * @brief Read a string property
 *
 * @param fdt Handle to device tree blob
 * @param node_offset offset of the DT node
 * @param prop_name Name of the property to read
 * @param res Pointer to hold the pointer to string value
 *
 * @return ERR_NOT_FOUND if the property is not available
 *		   NO_ERROR if success
 *		   On success, res points to the pointer to the string
 */
status_t dt_get_prop_string(const void *fdt, int node_offset, char *prop_name,
															const char **res);

/**
 * @brief Read a property that contains an array of strings
 *
 * @param fdt Handle to device tree blob
 * @param node_offset offset of the DT node
 * @param prop_name Name of the property to read
 * @param res Pointer to th array of char pointers. Callee filled
 * @param num Callee filled. Contains the number of terminated strings in
 *			  prop_name
 *
 * @return ERR_NOT_FOUND if the property is not available
 *		   NO_ERROR if success
 *		   On success, res points to the array of pointers to individual
 *		   strings, and num contains the number of strings in that array
 */
status_t dt_get_prop_string_array(const void *fdt, int node_offset,
								char *prop_name, const char **res,
													uint32_t *num);

/**
 * @brief Count number of strings in a string array
 *
 * @param fdt Handle to device tree blob
 * @param node_offset offset of the DT node
 * @param prop_name Name of the property to read
 * @param num Callee filled. Contains the number of terminated strings in
 *			  prop_name
 *
 * @return ERR_NOT_FOUND if the property is not available
 *		   NO_ERROR if success
 */
static inline status_t dt_get_prop_count_strings(const void *fdt,
				int node_offset, char *prop_name, uint32_t *num)
{
	return dt_get_prop_string_array(fdt, node_offset, prop_name, NULL, num);
}

/**
 * @brief Count number of elements of a given size in a DT property
 *
 * @param fdt Handle to device tree blob
 * @param node_offset offset of the DT node
 * @param prop_name Name of the property to read
 * @param sz Size of each element
 * @param num Callee filled. On success, contains the number of elements of
 *			  size sz in the property value
 *
 * @return ERR_NOT_FOUND if the property is not available
 *		   NO_ERROR if success
 */
status_t dt_count_elems_of_size(const void *fdt, int node_offset,
						char *prop_name, uint32_t sz, uint32_t *num);

/**
 * @brief Retrieve node offset of a node with the given name
 *		  Can be used to iterate over all nodes with a certain name
 *
 * @param fdt Handle to device tree blob
 * @param start_offset Node in FDT to start the search from.
 * @param name Name of the node
 * @param res Callee filled. On success, holds offset of the node that matches
 *		      the criteria.
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_get_node_with_name(const void *fdt, int start_offset, char *name,
																	int *res);

#define dt_for_each_node_with_name_from(fdt, node, start_offset, name)		\
	for (node = start_offset;												\
		dt_get_node_with_name(fdt, node, name, &(node)) == NO_ERROR;	\
		node = dt_next_node(fdt, node))

#define dt_for_each_node_with_name(fdt, node, name)		\
	dt_for_each_node_with_name_from(fdt, node, dt_next_node(fdt, -1), name)

/**
 * @brief Retrieve node offset of a node with the given path
 *
 * @param fdt Handle to device tree blob
 * @param path Path of the node relative to root node
 * @param res Callee filled. On success, holds offset of the node that matches
 *		      the criteria.
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_get_node_with_path(const void *fdt, const char *path, int *res);

/**
 * @brief Check if the platform device represented by the DT node is actually
 *	      present on the SOC. This is done by checking if the status property
 *	      is "ok" or "okay". If the property is absent, the device is assumed
 *	      to be present by default
 *
 * @param fdt Handle to device tree blob
 * @param node_offset offset of the DT node
 * @param res Callee filled. Holds true if device is present, else false
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_is_device_available(const void *fdt, int node_offset, bool *res);

/**
 * @brief Get the next 'available' (Refer to dt_is_device_available for more
 *		  info) node with offset > start_node
 *
 * @param fdt Device Tree handle
 * @param start_node Offset of the node to begin traversal with
 * @param res Callee filled. Contains the offset of the next available node
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_get_next_available(const void *fdt, int start_node, int *res);

#define dt_for_each_available_node_from(fdt, node, start)		\
	for (node = start;											\
		dt_get_next_available(fdt, node, &(node)) == NO_ERROR; \
		node = dt_next_node(fdt, node))

#define dt_for_each_available_node(fdt, node) \
	dt_for_each_available_node_from(fdt, node, dt_next_node(fdt, -1))

/**
 * @brief Check if a given string is enlisted amongst a device's compatibles
 *		  A node can enlist multiple compatibles to offer a range of drivers
 *		  that can be used to handle that device.
 *
 * @param fdt Handle to device tree blob
 * @param node_offset Offset of DT node
 * @param comp Compatible string to check for
 * @param res Callee filled. On success, holds true if the device is
 *		  compatible, else false
 *
 * @return ERR_NOT_FOUND if the compatible property is not found
 *		   ERR_NO_MEMORY is insufficient memory on the heap to hold compatibles
 *		   NO_ERROR if success
 */
status_t dt_is_device_compatible(const void *fdt, int node_offset,
										const char *comp, bool *res);

/**
 * @brief Retrieve node offset of a node with the given compatible property
 *		  Can be used to iterate over all nodes with a certain compatible
 *
 * @param fdt Handle to device tree blob
 * @param start_offset Node in FDT to start the search from.
 * @param comp Compatible property to search for
 * @param res Callee filled. On success, holds offset of the node that matches
 *		      the criteria.
 *
 * @return NO_ERROR if success, else ERR_NOT_FOUND
 */
status_t dt_get_node_with_compatible(const void *fdt, int start_offset,
												char *comp, int *res);

/**
 * @brief This function emulates the behaviour of mkdir -p

 * @param fdt Handle to device tree blob
 * @param node_path the node path to be created in device tree.
 *
 * @return integer offset of the node at given path, else ERR_GENERIC on a failure
*/
int dt_create_path(struct fdt_header *fdt, const char *node_path);

#define dt_for_each_compatible_from(fdt, node, start_offset, comp)			  \
	for (node = start_offset;												  \
		dt_get_node_with_compatible(fdt, node, comp, &(node)) == NO_ERROR;	  \
		node = dt_next_node(fdt, node))

#define dt_for_each_compatible(fdt, node, comp)	\
	dt_for_each_compatible_from(fdt, node, dt_next_node(fdt, -1), comp)

#endif /* __DT_COMMON_H */
