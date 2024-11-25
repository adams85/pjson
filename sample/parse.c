#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "pjson.h"
#include "stats_parser.h"

/* Helpers */

static const char *datatype_name(pjson_token_type token_type) {
  switch (token_type) {
    case PJSON_TOKEN_NULL: return "null";
    case PJSON_TOKEN_FALSE: return "false";
    case PJSON_TOKEN_TRUE: return "true";
    case PJSON_TOKEN_NUMBER: return "number";
    case PJSON_TOKEN_STRING: return "string";
    case PJSON_TOKEN_CLOSE_BRACKET: return "array";
    case PJSON_TOKEN_CLOSE_BRACE: return "object";
    default: return "<unexpected>";
  }
}

/* Command entry point */

int parse() {
  if (is_tty_stdout) {
#ifdef __WINDOWS__
    printf("Input JSON, press CTRL-Z in an empty line and finally ENTER.\n");
#else
    printf("Input JSON, then press Ctrl+D.\n");
#endif
  }

  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status status = -1; // unnecessary assignment, just for keeping the compiler happy
  bool value_found = false;

  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser.base.base);

  char buf[128];
  int num_read;

  // This example also shows how to parse a stream of multiple JSON values.

  for (;;) {
    while ((num_read = read_from_stdin(buf)) > 0) {
      status = pjson_feed(&tokenizer, (uint8_t *)buf, num_read);
    Resume:
      if (status != PJSON_STATUS_DATA_NEEDED) break;
    }

    if (num_read < 0) {
      pjson_close(&tokenizer);
      printf("Read error (%d).\n", num_read);
      return EXIT_FAILURE;
    }
    else if (num_read == 0) { // EOS
      status = pjson_close(&tokenizer);

      if (status == PJSON_STATUS_COMPLETED) return EXIT_SUCCESS;
    }

    if (status != PJSON_STATUS_COMPLETED) {
      status = pjson_close(&tokenizer);
      switch (status) {
        case PJSON_STATUS_NO_TOKENS_FOUND:
          if (value_found) return EXIT_SUCCESS;

          puts("No tokens found.");
          return EXIT_FAILURE;

        case PJSON_STATUS_SYNTAX_ERROR:
          printf("Syntax error at position %zu.\n", tokenizer.index);
          return EXIT_FAILURE;

        case PJSON_STATUS_UTF8_ERROR:
          printf("UTF-8 encoding error at position %zu.\n", tokenizer.index);
          return EXIT_FAILURE;

        case PJSON_STATUS_MAX_DEPTH_EXCEEDED:
          printf("Maximum depth of %d exceeded.\n", STATS_PARSER_MAX_DEPTH);
          return EXIT_FAILURE;

        default:
          printf("Unexpected error (%d).\n", status);
          return EXIT_FAILURE;
      }
    }

    value_found = true;

    // Output collected data in a format similar to what https://onlinetools.com/json/analyze-json uses,
    // this way it's easy to compare and verify the results.

    puts("General JSON Info:");
    puts("------------------");
    printf("Top-level type:             %s\n", datatype_name(parser.toplevel_datatype));
    printf("Max. depth:                 %zu\n", parser.max_depth + 1);
    if (parser.toplevel_datatype == PJSON_TOKEN_CLOSE_BRACKET || parser.toplevel_datatype == PJSON_TOKEN_CLOSE_BRACE) {
      printf("Max. array item count:      %zu\n", parser.max_array_item_count);
      printf("Max. object property count: %zu\n", parser.max_object_property_count);
      puts("");

      puts("Number of Data Types:");
      puts("---------------------");
      printf("Number of objects:  %zu\n", parser.datatype_counts[PJSON_TOKEN_CLOSE_BRACE - PJSON_TOKEN_NULL]);
      printf("Number of arrays:   %zu\n", parser.datatype_counts[PJSON_TOKEN_CLOSE_BRACKET - PJSON_TOKEN_NULL]);
      printf("Number of strings:  %zu\n", parser.datatype_counts[PJSON_TOKEN_STRING - PJSON_TOKEN_NULL]);
      printf("Number of numbers:  %zu\n", parser.datatype_counts[PJSON_TOKEN_NUMBER - PJSON_TOKEN_NULL]);
      printf("Number of booleans: %zu\n", parser.datatype_counts[PJSON_TOKEN_FALSE - PJSON_TOKEN_NULL] + parser.datatype_counts[PJSON_TOKEN_TRUE - PJSON_TOKEN_NULL]);
      printf("Number of null:     %zu\n", parser.datatype_counts[PJSON_TOKEN_NULL - PJSON_TOKEN_NULL]);
      printf("Number of keys:     %zu\n", parser.key_count);
      printf("Number of true:     %zu\n", parser.datatype_counts[PJSON_TOKEN_TRUE - PJSON_TOKEN_NULL]);
      printf("Number of false:    %zu\n", parser.datatype_counts[PJSON_TOKEN_FALSE - PJSON_TOKEN_NULL]);
    }

    // Reset parser state.
    stats_parser_reset(&parser, true);

    // If the current data hasn't been consumed entirely, consume the rest.
    if ((uintptr_t)tokenizer.token_start - (uintptr_t)buf < (uintptr_t)num_read) {
      status = pjson_feed(&tokenizer, tokenizer.token_start, (uintptr_t)(buf + num_read) - (uintptr_t)tokenizer.token_start);
      goto Resume;
    }

    // Otherwise, read the next chunk of data and resume parsing normally.
  }
}
