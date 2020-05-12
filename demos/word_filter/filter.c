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

int ui_to_host;
int host_to_ui;

static const char *word_list[]
= {
  "agile",
  "disruptive",
  "ecosystem",
  "incentivize",
  "low-hanging fruit",
  "negative growth",
  "paradigm shift",
  "rightsizing",
  "synergies",
};

void censor(char *msg)
__attribute__((pirate_capability("sensitive_words")))
{
  for (size_t i = 0; i < ARRAY_LEN(word_list); i++) {
    char *cursor = msg;
    char const* word = word_list[i];
    char *found;
    while ((found = strstr(cursor, word))) {
      memset(found, '*', strlen(word));
      cursor = found+1;
    }
  }
}

void transmit(int channel, char const* msg, size_t n) {
  if ((ssize_t)sizeof n != pirate_write(channel, &n, sizeof n)) { return 1; }
  if ((ssize_t)n        != pirate_write(channel, msg, n      )) { return 1; }
  return 0;
}

int receive(int channel, char **msg_ptr, size_t *n_ptr) {
  size_t n;
  if ((ssize_t)sizeof n != pirate_read(channel, &n, sizeof n)) { return 1; }

  char *msg = realloc(*msg_ptr, n);
  if (NULL == msg) { return 1; }
  *msg_ptr = msg;
  *n_ptr = n;

  if ((ssize_t)n != pirate_read(channel, msg, n)) { return 1; }

  return 0;
}

int ui (void)
__attribute__((pirate_enclave_main("filter_ui")))
{
  puts("Connecting");

  ui_to_host = pirate_open_parse("pipe,filter_ui_to_host", O_WRONLY);
  if (-1 == ui_to_host) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  host_to_ui = pirate_open_parse("pipe,filter_host_to_ui", O_RDONLY);
  if (-1 == host_to_ui) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  puts("Connected");

  char *line = NULL;
  size_t line_sz = 0;

  for(;;) {
	  printf("Input   > ");
	  fflush(stdout);

	  if (-1 == getline(&line, &line_sz, stdin)) {
	    puts("\n");
	    break;
	  }

	  if (transmit(ui_to_host, line, line_sz)) { exit(EXIT_FAILURE); }
	  if (receive(host_to_ui, &line, &line_sz)) { exit(EXIT_FAILURE); }

	  printf("Response> %s", line);
  }

  pirate_close(ui_to_host);
  pirate_close(host_to_ui);

  return 0;
}

int host (void)
__attribute__((pirate_enclave_main("filter_host")))
{
  ui_to_host = pirate_open_parse("pipe,filter_ui_to_host", O_RDONLY);
  if (-1 == ui_to_host) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  host_to_ui = pirate_open_parse("pipe,filter_host_to_ui", O_WRONLY);
  if (-1 == host_to_ui) { perror("pirate_open_parse"); exit(EXIT_FAILURE); }

  char *line = NULL;
  size_t line_sz = 0;
  for (;;) {
    if (receive(ui_to_host, &line, &line_sz)) { exit(EXIT_FAILURE); }
    censor(line);
    if (transmit(host_to_ui, line, line_sz)) { exit(EXIT_FAILURE); }
  }

  return 0;
}
