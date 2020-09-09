#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#define ARRAY_LEN(arr) (sizeof arr / sizeof *arr)

class Filter {
public:
  Filter() {}
  virtual ~Filter() {}
  virtual void censor(char *msg) = 0;
};

class SensitiveFilter : public Filter {
public:
  SensitiveFilter() : Filter() {}
  ~SensitiveFilter() {}
  void censor(char *msg) final override {
    for (size_t i = 0; i < ARRAY_LEN(word_list); i++) {
      char const *word = word_list[i];
      char *found;
      while ((found = strstr(msg, word))) {
        memset(found, '*', strlen(word));
      }
    }
  }

private:
  static constexpr char *word_list[] = {
      "agile",          "disruptive",        "ecosystem",
      "incentivize",    "low-hanging fruit", "negative growth",
      "paradigm shift", "rightsizing",       "synergies",
  };
};

class NaiveFilter : public Filter {
public:
  NaiveFilter() : Filter() {}
  ~NaiveFilter() {}
  void censor(char *msg) final override {
    bool ShouldCensor = false;
    char *curTok = msg;
    while (*curTok != '\0') {
      if (ShouldCensor) {
        *curTok = '*';
      }

      curTok++;
      ShouldCensor = !ShouldCensor;
    }
  }
};

int main(void) {
  Filter *F = new NaiveFilter();
  char *line = NULL;
  size_t line_sz = 0;
  const size_t maxLen = 80;

  for (;;) {
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

    F->censor(line);

    printf("Response> %s", line);
  }
  free(line);
  free(F);

  return 0;
}
