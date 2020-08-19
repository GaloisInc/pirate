#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

#include "tweetnacl.h"
#include "base64.h"

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key1[KEY_BYTES] = "secret key 1";
static unsigned char nonce1[NONCE_BYTES] = {0};

static const unsigned char key2[KEY_BYTES] = "secret key 2";
static unsigned char nonce2[NONCE_BYTES] = {0};

typedef struct encrypt_state {
    char *buffer1;
    char *buffer2;
    size_t len;
} encrypt_state_t;

sem_t encrypt1_sem, encrypt2_sem;

static void increment_nonce(unsigned char *n, const size_t nlen) {
    size_t i = 0U;
    uint_fast16_t c = 1U;
    for (; i < nlen; i++) {
        c += (uint_fast16_t) n[i];
        n[i] = (unsigned char) c;
        c >>= 8;
    }
}

/**
 * Assume that encrypt1() and encrypt2() use different encryption
 * algorithms that are provided by different encryption libraries.
 * 
 * For simplicity they both use crypto_secretbox_xsalsa20poly1305
 * primitive by the TweetNaCl library https://tweetnacl.cr.yp.to
 * 
 * TODO: prepend the nonces to the output string
 **/

void* encrypt1(void* arg) {
    encrypt_state_t* state = (encrypt_state_t*) arg;
    char *input = state->buffer1;
    char *output = state->buffer2 + DELTA_BYTES;
    size_t len = state->len;

    sem_wait(&encrypt1_sem);
    size_t rv = crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce1, key1);
    increment_nonce(nonce1, NONCE_BYTES);
    sem_post(&encrypt2_sem);
    return (void*) rv;
}

void* encrypt2(void* arg) {
    encrypt_state_t* state = (encrypt_state_t*) arg;
    char *input = state->buffer2;
    char *output = state->buffer1;
    size_t len = state->len + DELTA_BYTES;

    sem_wait(&encrypt2_sem);
    size_t rv = crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce2, key2);
    increment_nonce(nonce2, NONCE_BYTES);
    sem_post(&encrypt1_sem);
    return (void*) rv;
}

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;
static const int base64_size = (((double_encryption_size / 3) + 1) * 4);

int main_encryption(char *buffer1, char *buffer2, char *encoded) {
    
    size_t mlen;
    encrypt_state_t state;
    pthread_t tid1, tid2;
    void *rv1, *rv2;

    char *newline, *read_offset = buffer1 + ZERO_BYTES;
    const size_t read_length = input_size;

    char *success = fgets(read_offset, read_length, stdin);
    if (success == NULL) {
        return -1;
    }
    // strip trailing newline
    newline = strrchr(read_offset, '\n');
    if (newline != NULL) {
        *newline = 0;
    }
    mlen = strnlen(read_offset, read_length) + ZERO_BYTES;

    state.buffer1 = buffer1;
    state.buffer2 = buffer2;
    state.len = mlen;

    pthread_create(&tid1, NULL, encrypt1, &state);
    pthread_create(&tid2, NULL, encrypt2, &state);

    sem_post(&encrypt1_sem);

    pthread_join(tid1, &rv1);
    pthread_join(tid2, &rv2);
    if (rv1 || rv2) {
        return -1;
    }

    base64_encode(encoded, buffer1 + BOX_ZERO_BYTES, mlen + DELTA_BYTES - BOX_ZERO_BYTES);
    printf("%s\n", encoded);

    return 0;
}

int main() {
    sem_init(&encrypt1_sem, 0, 0);
    sem_init(&encrypt2_sem, 0, 0);
    char *buffer1 = calloc(double_encryption_size, 1);
    char *buffer2 = calloc(double_encryption_size, 1);
    char *encoded = calloc(base64_size, 1);
    int rv = main_encryption(buffer1, buffer2, encoded);
    free(buffer1);
    free(buffer2);
    free(encoded);
    sem_destroy(&encrypt1_sem);
    sem_destroy(&encrypt2_sem);
    return rv;
}
