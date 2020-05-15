#include "libpirate.h"

#include <fcntl.h>
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

static void write_all(int c, char const* buf, size_t count)
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

static void read_all(int c, char * buf, size_t count)
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

static void transmit(int c, char const* msg, size_t n)
{
  write_all(c, (char const*)&n, sizeof n);
  write_all(c, msg, n);
}

static size_t receive(int c, char **msg_ptr, size_t *n_ptr)
{
  size_t n;
  read_all(c, (char *)&n, sizeof n);
  expand_buffer(n, msg_ptr, n_ptr);
  read_all(c, *msg_ptr, n);
  return n;
}

/* Main function for the user interface.
 */
int ui(void)
__attribute__((pirate_enclave_main("filter_ui")))
{
  puts("Connecting");

  int writechan = pirate_open_parse("pipe,filter_ui_to_host", O_WRONLY);
  if (-1 == writechan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  int readchan = pirate_open_parse("pipe,filter_host_to_ui", O_RDONLY);
  if (-1 == readchan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  puts("Connected");

  char *line = NULL;
  size_t line_sz = 0;

  for(;;) {
    printf("Input> ");
    fflush(stdout);

    ssize_t len = getline(&line, &line_sz, stdin);
    if (len < 0) {
      puts("\n");
      break;
    }

    transmit(writechan, line, len + /*null-term*/1);
    receive(readchan, &line, &line_sz);

    printf("Response> %s", line);
  }

  return 0;
}

int host(void)
__attribute__((pirate_enclave_main("filter_host")))
{
  puts("Connecting");

  int readchan = pirate_open_parse("pipe,filter_ui_to_host", O_RDONLY);
  if (-1 == readchan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  int writechan = pirate_open_parse("pipe,filter_host_to_ui", O_WRONLY);
  if (-1 == writechan) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  puts("Connected");

  char *line = NULL;
  size_t line_sz = 0;

  for (;;) {
    size_t len = receive(readchan, &line, &line_sz);
    printf("Got> %s", line);
    censor(line);
    transmit(writechan, line, len);
    printf("Sent> %s", line);
  }

  return 0;
}
