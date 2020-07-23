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
#define DELTA_ZERO_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_ZERO_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key1[KEY_BYTES] = "secret key 1";
static unsigned char nonce1[NONCE_BYTES] = {0};

static const unsigned char key2[KEY_BYTES] = "secret key 2";
static unsigned char nonce2[NONCE_BYTES] = {0};

typedef struct encrypt_state {
    char *input;
    char *output;
    size_t len;
    int iter;
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
 **/

void* encrypt1(void* arg) {
    encrypt_state_t* state = (encrypt_state_t*) arg;
    char *input = state->input;
    char *output = state->output + DELTA_ZERO_BYTES;
    size_t len = state->len;

    while (1) {
        sem_wait(&encrypt1_sem);
        state->iter--;
        if (state->iter < 0) {
            sem_post(&encrypt2_sem);
            return NULL;
        }
        crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce1, key1);
        increment_nonce(nonce1, NONCE_BYTES);
        sem_post(&encrypt2_sem);
    }
}

void* encrypt2(void* arg) {
    encrypt_state_t* state = (encrypt_state_t*) arg;
    char *input = state->output;
    char *output = state->input + DELTA_ZERO_BYTES;
    size_t len = state->len;

    while (1) {
        sem_wait(&encrypt2_sem);
        state->iter--;
        if (state->iter < 0) {
            sem_post(&encrypt1_sem);
            return NULL;
        }
        crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce2, key2);
        increment_nonce(nonce2, NONCE_BYTES);
        sem_post(&encrypt1_sem);
    }
}


int main() {
    char input[80 + DELTA_ZERO_BYTES] = {0};
    char output[80 + DELTA_ZERO_BYTES] = {0};
    char encoded[120] = {0};
    
    size_t mlen;
    encrypt_state_t state;
    pthread_t tid1, tid2; 
    char* read_offset = input + ZERO_BYTES;
    const size_t read_length = 80 - ZERO_BYTES;

    char *success = fgets(read_offset, read_length, stdin);
    if (success == NULL) {
        exit(1);
    }
    mlen = strnlen(read_offset, read_length) + ZERO_BYTES;

    state.input = input;
    state.output = output;
    state.len = mlen;
    state.iter = 200000;

    sem_init(&encrypt1_sem, 0, 0);
    sem_init(&encrypt2_sem, 0, 0);

    pthread_create(&tid1, NULL, encrypt1, &state);
    pthread_create(&tid2, NULL, encrypt2, &state);

    sem_post(&encrypt1_sem);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    base64_encode(encoded, input + ZERO_BYTES, mlen - ZERO_BYTES);
    printf("%s\n", encoded);

    sem_destroy(&encrypt1_sem);
    sem_destroy(&encrypt2_sem);

    return 0;
}
