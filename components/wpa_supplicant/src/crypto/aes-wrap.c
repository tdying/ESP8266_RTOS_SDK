/*
 * AES Key Wrap Algorithm (128-bit KEK) (RFC3394)
 *
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "sdkconfig.h"

#ifndef CONFIG_ESP_AES

#include "utils/includes.h"

#include "utils/common.h"
#include "crypto/aes.h"
#include "crypto/aes_wrap.h"

/**
 * aes_wrap - Wrap keys with AES Key Wrap Algorithm (128-bit KEK) (RFC3394)
 * @kek: 16-octet Key encryption key (KEK)
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @plain: Plaintext key to be wrapped, n * 64 bits
 * @cipher: Wrapped key, (n + 1) * 64 bits
 * Returns: 0 on success, -1 on failure
 */
int  aes_wrap(const u8 *kek, int n, const u8 *plain, u8 *cipher)
{
	u8 *a, *r, b[16];
	int i, j;
	void *ctx;

	a = cipher;
	r = cipher + 8;

	/* 1) Initialize variables. */
	os_memset(a, 0xa6, 8);
	os_memcpy(r, plain, 8 * n);

	ctx = aes_encrypt_init(kek, 16);
	if (ctx == NULL)
		return -1;

	/* 2) Calculate intermediate values.
	 * For j = 0 to 5
	 *     For i=1 to n
	 *         B = AES(K, A | R[i])
	 *         A = MSB(64, B) ^ t where t = (n*j)+i
	 *         R[i] = LSB(64, B)
	 */
	for (j = 0; j <= 5; j++) {
		r = cipher + 8;
		for (i = 1; i <= n; i++) {
			os_memcpy(b, a, 8);
			os_memcpy(b + 8, r, 8);
			aes_encrypt(ctx, b, b);
			os_memcpy(a, b, 8);
			a[7] ^= n * j + i;
			os_memcpy(r, b + 8, 8);
			r += 8;
		}
	}
	aes_encrypt_deinit(ctx);

	/* 3) Output the results.
	 *
	 * These are already in @cipher due to the location of temporary
	 * variables.
	 */

	return 0;
}
#else
#include <string.h>
#include "esp_aes.h"

int aes_wrap(const uint8_t *kek, int n, const uint8_t *plain, uint8_t *cipher)
{
	int ret;
	uint8_t *a, *r, b[16];
	esp_aes_t ctx;

	a = cipher;
	r = cipher + 8;

	/* 1) Initialize variables. */
	memset(a, 0xa6, 8);
	memcpy(r, plain, 8 * n);

	ret = esp_aes_set_encrypt_key(&ctx, kek, 128);
	if (ret)
		return ret;

	/* 2) Calculate intermediate values.
	 * For j = 0 to 5
	 *     For i=1 to n
	 *         B = AES(K, A | R[i])
	 *         A = MSB(64, B) ^ t where t = (n*j)+i
	 *         R[i] = LSB(64, B)
	 */
	for (int j = 0; j <= 5; j++) {
		r = cipher + 8;
		for (int i = 1; i <= n; i++) {
			memcpy(b, a, 8);
			memcpy(b + 8, r, 8);
			esp_aes_encrypt_ecb(&ctx, b, b);
			memcpy(a, b, 8);
			a[7] ^= n * j + i;
			memcpy(r, b + 8, 8);
			r += 8;
		}
	}

	/* 3) Output the results.
	 *
	 * These are already in @cipher due to the location of temporary
	 * variables.
	 */

	return 0;
}

#endif /* CONFIG_ESP_AES */
