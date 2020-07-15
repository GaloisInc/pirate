#include "libpirate.h"

#include <fcntl.h>
#include <pal/pal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pirate enclave declare(filter_ui)
#pragma pirate enclave declare(filter_host)
#pragma pirate capability declare(sensitive_words)
#pragma pirate enclave capability(filter_host, sensitive_words)

#define ARRAY_LEN(arr) (sizeof arr / sizeof *arr)

static const char *word_list[]
__attribute__((pirate_capability("sensitive_words")))
= {
  "agile", "disruptive", "ecosystem", "incentivize",
  "low-hanging fruit", "negative growth", "paradigm shift",
  "rightsizing", "synergies",
};

static void censor(char *msg)
{
  for (size_t i = 0; i < ARRAY_LEN(word_list); i++) {
    char const* word = word_list[i];
    char *found;
    while ((found = strstr(msg, word))) {
      memset(found, '*', strlen(word));
    }
  }
}

static void expand_buffer(size_t n, char ** msg_ptr, size_t * n_ptr)
{
  if (n > *n_ptr) {
    *msg_ptr = realloc(*msg_ptr, n);
    if (NULL == *msg_ptr) { exit(EXIT_FAILURE); }
    *n_ptr = n;
  }
}

static void write_all(pirate_channel c, char const* buf, size_t count)
{
  size_t sofar = 0;
  while (sofar < count) {
    ssize_t result = pirate_write(c, buf + sofar, count - sofar);
    if (result < 0) {
      perror("pirate_write");
      exit(EXIT_FAILURE);
    }
    sofar += result;
  }
}

static void read_all(pirate_channel c, char * buf, size_t count)
{
  size_t sofar = 0;
  while (sofar < count) {
    ssize_t result = pirate_read(c, buf + sofar, count - sofar);
    if (result < 0) {
      perror("pirate_read");
      exit(EXIT_FAILURE);
    }
    sofar += result;
  }
}

static void transmit(pirate_channel c, char const* msg, size_t n)
{
  write_all(c, (char const*)&n, sizeof n);
  write_all(c, msg, n);
}

static size_t receive(pirate_channel c, char **msg_ptr, size_t *n_ptr)
{
  size_t n;
  read_all(c, (char *)&n, sizeof n);
  expand_buffer(n, msg_ptr, n_ptr);
  read_all(c, *msg_ptr, n);
  return n;
}

pirate_channel ui_to_host
    __attribute__((pirate_resource("ui_to_host", "filter_ui")))
    __attribute__((pirate_resource_param("permissions", "writeonly", "filter_ui")))
    __attribute__((pirate_resource("ui_to_host", "filter_host")))
    __attribute__((pirate_resource_param("permissions", "readonly", "filter_host")));
pirate_channel host_to_ui
    __attribute__((pirate_resource("host_to_ui", "filter_host")))
    __attribute__((pirate_resource_param("permissions", "writeonly", "filter_host")))
    __attribute__((pirate_resource("host_to_ui", "filter_ui")))
    __attribute__((pirate_resource_param("permissions", "readonly", "filter_ui")));

/* Main function for the user interface.
 */
int ui(void)
__attribute__((pirate_enclave_main("filter_ui")))
{
  puts("UI:\tConnected");

  char *line = NULL;
  size_t line_sz = 0;

  for(;;) {
    printf("UI:\tInput> ");
    fflush(stdout);

    ssize_t len = getline(&line, &line_sz, stdin);
    if (len < 0) {
      puts("\n");
      break;
    }

    transmit(ui_to_host, line, len + /*null-term*/1);
    receive(host_to_ui, &line, &line_sz);

    printf("UI:\tResponse> %s", line);
  }

  return 0;
}

int host(void)
__attribute__((pirate_enclave_main("filter_host")))
{
  puts("Host:\tConnected");

  char *line = NULL;
  size_t line_sz = 0;

  for (;;) {
    size_t len = receive(ui_to_host, &line, &line_sz);
    printf("Host:\tGot> %s", line);
    censor(line);
    transmit(host_to_ui, line, len);
    printf("Host:\tSent> %s", line);
  }

  return 0;
}
