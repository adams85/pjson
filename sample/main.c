#include "stdio.h"
#include "stdlib.h"
#include "string.h"

extern int tokenize();
extern int parse();

static void print_help() {
  puts("A simple CLI tool for demonstrating the features and usage of the pjson library.");
  puts("");
  puts("Usage: pjson [command]");
  puts("");
  puts("Commands:");
  puts("  parse: Reads JSON data from the standard input and collects statistics on the data stream while parsing it.");
  puts("  tokenize: Reads JSON data from the standard input and prints the tokens found in the data stream.");
  puts("");
  puts("Run 'pjson -?|-h|--help' to display this information again.");
  puts("");
}

int main(int argc, char *argv[]) {
  int (*run_command)();

  if (argc < 2 || strcmp(argv[1], "parse") == 0) run_command = &parse;
  else if (strcmp(argv[1], "tokenize") == 0) run_command = &tokenize;
  else if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    print_help();
    return EXIT_SUCCESS;
  }
  else {
    printf("Invalid command '%s'.\n", argv[1]);
    puts("");
    print_help();
    return EXIT_FAILURE;
  }

  return run_command();
}
