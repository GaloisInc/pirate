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

// This belongs in another enclave
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

int main(void) {

  char *line = NULL;
  size_t line_sz = 0;
  const size_t maxLen = 80;

  for(;;) {
    printf("Input> ");
    fflush(stdout);

    ssize_t len = getline(&line, &line_sz, stdin);
    
    if (len < 0) {
      puts("\n");
      break;
    }

    // Bound string size and maxLen
    if (len > maxLen) {
      line[maxLen] = 0; // This is valid because line_sz must be at least len +1
    }

    censor(line);

    printf("Response> %s", line);
  }
  free(line);

  return 0;
}