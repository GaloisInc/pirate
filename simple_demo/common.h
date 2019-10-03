#ifndef _SIMPLE_DEMO_COMMON_H_
#define _SIMPLE_DEMO_COMMON_H_

#define HIGH_TO_LOW_CH 0
#define LOW_TO_HIGH_CH 1

// Example data
#define DATA_LEN            (32 << 10)         // 32 KB
typedef struct {
    char buf[DATA_LEN];
    int len;
} example_data_t;

#endif /* _SIMPLE_DEMO_COMMON_H_ */
