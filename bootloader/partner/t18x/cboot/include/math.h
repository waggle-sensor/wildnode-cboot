/*
 * Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __MATH_H
#define __MATH_H

#include <stdio.h>
#include <stdlib.h>

static inline unsigned long gcd(unsigned long m, unsigned long n)
{
	unsigned long tmp;
	while (m) {
		tmp = m;
		m = n % m;
		n = tmp;
	}
	return n;
}

static inline unsigned long lcm(unsigned long m, unsigned long n)
{
	return m / gcd(m , n) * n;
}


#define ROUND_UP(a,b)		((((a) + (b) -1 ) / (b)) * (b))
#define ROUND_DOWN(a,b)		((a) - ((a) % ((b))))

#define DIV_ROUNDUP(a, b)  ((ROUND_UP((a), (b))) / (b))
#define DIV_ROUNDDOWN(a, b) ((ROUND_DOWN((a), (b))) / (b))
#define DIV_ROUNDCLOSEST(a, b) \
		((ROUND_UP((a), (b)) - (a)) <= ((a) - ROUND_DOWN((a), (b))) ? \
		(ROUND_UP((a), (b)) / (b)) : (ROUND_DOWN((a), (b)) / (b)))

#endif
