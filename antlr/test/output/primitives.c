#include <endian.h>
#include <stdint.h>


struct primitives {
float a __attribute__((aligned(4)));
double b __attribute__((aligned(8)));
int16_t d __attribute__((aligned(2)));
int32_t e __attribute__((aligned(4)));
int64_t f __attribute__((aligned(8)));
uint16_t g __attribute__((aligned(2)));
uint32_t h __attribute__((aligned(4)));
uint64_t i __attribute__((aligned(8)));
char j __attribute__((aligned(1)));
uint8_t k __attribute__((aligned(1)));
uint8_t l __attribute__((aligned(1)));
};

struct alt_primitives {
int8_t a __attribute__((aligned(1)));
int16_t b __attribute__((aligned(2)));
int32_t c __attribute__((aligned(4)));
int64_t d __attribute__((aligned(8)));
uint8_t e __attribute__((aligned(1)));
uint16_t f __attribute__((aligned(2)));
uint32_t g __attribute__((aligned(4)));
uint64_t h __attribute__((aligned(8)));
};

void encode_primitives(struct primitives* input, struct primitives* output) {
uint32_t a = *(uint32_t*) &input->a;
uint64_t b = *(uint64_t*) &input->b;
uint16_t d = *(uint16_t*) &input->d;
uint32_t e = *(uint32_t*) &input->e;
uint64_t f = *(uint64_t*) &input->f;
uint16_t g = *(uint16_t*) &input->g;
uint32_t h = *(uint32_t*) &input->h;
uint64_t i = *(uint64_t*) &input->i;
a = htobe32(a);
b = htobe64(b);
d = htobe16(d);
e = htobe32(e);
f = htobe64(f);
g = htobe16(g);
h = htobe32(h);
i = htobe64(i);
output->a = *(float*) &a;
output->b = *(double*) &b;
output->d = *(int16_t*) &d;
output->e = *(int32_t*) &e;
output->f = *(int64_t*) &f;
output->g = *(uint16_t*) &g;
output->h = *(uint32_t*) &h;
output->i = *(uint64_t*) &i;
output->j = input->j;
output->k = input->k;
output->l = input->l;
}

void encode_alt_primitives(struct alt_primitives* input, struct alt_primitives* output) {
uint16_t b = *(uint16_t*) &input->b;
uint32_t c = *(uint32_t*) &input->c;
uint64_t d = *(uint64_t*) &input->d;
uint16_t f = *(uint16_t*) &input->f;
uint32_t g = *(uint32_t*) &input->g;
uint64_t h = *(uint64_t*) &input->h;
b = htobe16(b);
c = htobe32(c);
d = htobe64(d);
f = htobe16(f);
g = htobe32(g);
h = htobe64(h);
output->a = input->a;
output->b = *(int16_t*) &b;
output->c = *(int32_t*) &c;
output->d = *(int64_t*) &d;
output->e = input->e;
output->f = *(uint16_t*) &f;
output->g = *(uint32_t*) &g;
output->h = *(uint64_t*) &h;
}

void decode_primitives(struct primitives* input, struct primitives* output) {
uint32_t a = *(uint32_t*) &input->a;
uint64_t b = *(uint64_t*) &input->b;
uint16_t d = *(uint16_t*) &input->d;
uint32_t e = *(uint32_t*) &input->e;
uint64_t f = *(uint64_t*) &input->f;
uint16_t g = *(uint16_t*) &input->g;
uint32_t h = *(uint32_t*) &input->h;
uint64_t i = *(uint64_t*) &input->i;
a = be32toh(a);
b = be64toh(b);
d = be16toh(d);
e = be32toh(e);
f = be64toh(f);
g = be16toh(g);
h = be32toh(h);
i = be64toh(i);
output->a = *(float*) &a;
output->b = *(double*) &b;
output->d = *(int16_t*) &d;
output->e = *(int32_t*) &e;
output->f = *(int64_t*) &f;
output->g = *(uint16_t*) &g;
output->h = *(uint32_t*) &h;
output->i = *(uint64_t*) &i;
output->j = input->j;
output->k = input->k;
output->l = input->l;
}

void decode_alt_primitives(struct alt_primitives* input, struct alt_primitives* output) {
uint16_t b = *(uint16_t*) &input->b;
uint32_t c = *(uint32_t*) &input->c;
uint64_t d = *(uint64_t*) &input->d;
uint16_t f = *(uint16_t*) &input->f;
uint32_t g = *(uint32_t*) &input->g;
uint64_t h = *(uint64_t*) &input->h;
b = be16toh(b);
c = be32toh(c);
d = be64toh(d);
f = be16toh(f);
g = be32toh(g);
h = be64toh(h);
output->a = input->a;
output->b = *(int16_t*) &b;
output->c = *(int32_t*) &c;
output->d = *(int64_t*) &d;
output->e = input->e;
output->f = *(uint16_t*) &f;
output->g = *(uint32_t*) &g;
output->h = *(uint64_t*) &h;
}
