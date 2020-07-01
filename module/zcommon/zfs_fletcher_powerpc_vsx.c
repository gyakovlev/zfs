/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (C) 2020 Richard Yao. All rights reserved.
 * Copyright (C) 2020 Georgy Yakovlev. All rights reserved.
 */

#if defined(__powerpc64__)


#include <altivec.h>
#include <sys/spa_checksum.h>
#include <sys/simd.h>
#include <sys/strings.h>
#include <zfs_fletcher.h>

static void
fletcher_4_powerpc_vsx_init(fletcher_4_ctx_t *ctx)
{
	bzero(ctx->powerpc_vsx, 4 * sizeof (zfs_fletcher_powerpc_vsx_t));
}

static void
fletcher_4_powerpc_vsx_fini(fletcher_4_ctx_t *ctx, zio_cksum_t *zcp)
{
	uint64_t A, B, C, D;
	A = ctx->powerpc_vsx[0].v[0] + ctx->powerpc_vsx[0].v[1];
	B = 2 * ctx->powerpc_vsx[1].v[0] + 2 * ctx->powerpc_vsx[1].v[1] - ctx->powerpc_vsx[0].v[1];
	C = 4 * ctx->powerpc_vsx[2].v[0] - ctx->powerpc_vsx[1].v[0] + 4 * ctx->powerpc_vsx[2].v[1] -
	    3 * ctx->powerpc_vsx[1].v[1];
	D = 8 * ctx->powerpc_vsx[3].v[0] - 4 * ctx->powerpc_vsx[2].v[0] + 8 * ctx->powerpc_vsx[3].v[1] -
	    8 * ctx->powerpc_vsx[2].v[1] + ctx->powerpc_vsx[1].v[1];
	ZIO_SET_CHECKSUM(zcp, A, B, C, D);
}

static void
fletcher_4_powerpc_vsx_native(fletcher_4_ctx_t *ctx, const void *buf, uint64_t size)
{
        const uint64_t *ip = buf;
        const uint64_t *ipend = (uint64_t *)((uint8_t *)ip + size);
        vector long long int a, b, c, d;

        a = *(vector long long int const*)&(ctx)->powerpc_vsx[0];
        b = *(vector long long int const*)&(ctx)->powerpc_vsx[1];
        c = *(vector long long int const*)&(ctx)->powerpc_vsx[2];
        d = *(vector long long int const*)&(ctx)->powerpc_vsx[3];

        for (; ip < ipend; ip += 2) {
            vector int data = *(vector int *)ip;
            vector long long int t0, t1;

            t0 = vec_unpackl(data);
            t1 = vec_unpackh(data);
            a += t0;
            b += a;
            c += b;
            d += c;
            a += t1;
            b += a;
            c += b;
            d += c;
        }

        *(vector long long int*)&(ctx)->powerpc_vsx[0] = a;
        *(vector long long int*)&(ctx)->powerpc_vsx[1] = b;
        *(vector long long int*)&(ctx)->powerpc_vsx[2] = c;
        *(vector long long int*)&(ctx)->powerpc_vsx[3] = d;
}

static void
fletcher_4_powerpc_vsx_byteswap(fletcher_4_ctx_t *ctx, const void *buf, uint64_t size)
{
        const uint64_t *ip = buf;
        const uint64_t *ipend = (uint64_t *)((uint8_t *)ip + size);
        vector long long int a, b, c, d;
        vector unsigned short v8c = {8,8,8,8,8,8,8,8};

        a = *(vector long long int const*)&(ctx)->powerpc_vsx[0];
        b = *(vector long long int const*)&(ctx)->powerpc_vsx[1];
        c = *(vector long long int const*)&(ctx)->powerpc_vsx[2];
        d = *(vector long long int const*)&(ctx)->powerpc_vsx[3];

        for (; ip < ipend; ip += 2) {
            vector int data = *(vector int *)ip;
            vector long long int t0, t1;

            data = (vector int) vec_rl( (vector unsigned short) data, v8c);

            t0 = vec_unpackl(data);
            t1 = vec_unpackh(data);
            a += t0;
            b += a;
            c += b;
            d += c;
            a += t1;
            b += a;
            c += b;
            d += c;
        }

        *(vector long long int*)&(ctx)->powerpc_vsx[0] = a;
        *(vector long long int*)&(ctx)->powerpc_vsx[1] = b;
        *(vector long long int*)&(ctx)->powerpc_vsx[2] = c;
        *(vector long long int*)&(ctx)->powerpc_vsx[3] = d;
}


static boolean_t fletcher_4_powerpc_vsx_valid(void)
{
	return (B_TRUE);
}

const fletcher_4_ops_t fletcher_4_power_vsx_ops = {
	.init_native = fletcher_4_powerpc_vsx_init,
	.compute_native = fletcher_4_powerpc_vsx_native,
	.fini_native = fletcher_4_powerpc_vsx_fini,
	.init_byteswap = fletcher_4_powerpc_vsx_init,
	.compute_byteswap = fletcher_4_powerpc_vsx_byteswap,
	.fini_byteswap = fletcher_4_powerpc_vsx_fini,
	.valid = fletcher_4_powerpc_vsx_valid,
	.name = "powerpc_vsx"
};
#endif /* __powerpc64__ */
