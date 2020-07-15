#include "libpirate.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(arr) (sizeof arr / sizeof *arr)

static const char *word_list[]
= {
  "agile", "disruptive", "ecosystem", "incentivize",
  "low-hanging fruit", "negative growth", "paradigm shift",
  "rightsizing", "synergies",
};

static void censor(char *msg)
__attribute__((pirate_enclave("word_filter")))
{
  for (size_t i = 0; i < ARRAY_LEN(word_list); i++) {
    char const* word = word_list[i];
    char *found;
    while ((found = strstr(msg, word))) {
      memset(found, '*', strlen(word));
    }
  }
}

int main(void) {

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

    censor(line);

    printf("Response> %s", line);
  }

  return 0;
}