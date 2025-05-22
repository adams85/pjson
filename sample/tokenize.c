#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "pjson.h"

static const char *token_type_name(pjson_token_type token_type) {
  switch (token_type) {
    case PJSON_TOKEN_ERROR: return "error";
    case PJSON_TOKEN_NONE: return "<n/a>";
    case PJSON_TOKEN_NULL: return "null";
    case PJSON_TOKEN_FALSE: return "false";
    case PJSON_TOKEN_TRUE: return "true";
    case PJSON_TOKEN_NUMBER: return "number";
    case PJSON_TOKEN_STRING: return "string";
    case PJSON_TOKEN_OPEN_BRACKET: return "open_bracket";
    case PJSON_TOKEN_CLOSE_BRACKET: return "close_bracket";
    case PJSON_TOKEN_OPEN_BRACE: return "open_brace";
    case PJSON_TOKEN_CLOSE_BRACE: return "close_brace";
    case PJSON_TOKEN_COLON: return "colon";
    case PJSON_TOKEN_COMMA: return "comma";
    case PJSON_TOKEN_EOS: return "eos";
    default: return "<unknown>";
  }
}

static pjson_parsing_status on_subsequent_token(pjson_parser_base *parser, const pjson_token *token) {
  (void)parser;

  if (token->type == PJSON_TOKEN_EOS) return PJSON_STATUS_COMPLETED;

  char fixed_size_buf[128];
  char *buf;
  if (token->length < sizeof(fixed_size_buf)) {
    buf = fixed_size_buf;
  }
  else if (!(buf = pjson_malloc(token->length + 1))) {
    return PJSON_STATUS_OUT_OF_MEMORY;
  }
  memcpy(buf, token->start, token->length);
  buf[token->length] = 0;

  printf("/* type: %s | start_index: %zu | length: %zu | value: %s",
    token_type_name(token->type), token->start_index, token->length, buf);

  if (token->unescaped_length != token->length - (token->type == PJSON_TOKEN_STRING ? 2 : 0)) {
    if (!pjson_parse_string((uint8_t *)buf, token->length, token->start, token->length, true)) {
      return PJSON_STATUS_USER_ERROR;
    }
    buf[token->unescaped_length] = 0;

    printf(" | unescaped_length: %zu | unescaped value: %s", token->unescaped_length, buf);
  }

  puts(" */");

  if (buf != fixed_size_buf) pjson_free(buf);

  return PJSON_STATUS_DATA_NEEDED;
}

static pjson_parsing_status on_first_token(pjson_parser_base *parser, const pjson_token *token) {
  if (token->type == PJSON_TOKEN_EOS) return PJSON_STATUS_NO_TOKENS_FOUND;

  parser->eat = &on_subsequent_token;
  return on_subsequent_token(parser, token);
}

/* Command entry point */

int tokenize() {
  if (is_tty_stdout) {
#ifdef __WINDOWS__
    puts("Input JSON, press CTRL-Z in an empty line and finally ENTER.");
#else
    puts("Input JSON, then press Ctrl+D.\n");
#endif
  }

  pjson_parser_base parser;
  parser.eat = &on_first_token;

  pjson_parsing_status status;
  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser);

  char buf[128];
  int num_read;

  while ((num_read = read_from_stdin(buf)) > 0) {
    status = pjson_feed(&tokenizer, (uint8_t *)buf, num_read);
    if (status != PJSON_STATUS_DATA_NEEDED) break;
  }

  status = pjson_close(&tokenizer);

  if (num_read < 0) {
    pjson_close(&tokenizer);
    printf("Read error (%d).\n", num_read);
    return EXIT_FAILURE;
  }

  switch (status) {
    case PJSON_STATUS_COMPLETED:
      break;

    case PJSON_STATUS_NO_TOKENS_FOUND:
      puts("No tokens found.");
      return EXIT_FAILURE;

    case PJSON_STATUS_SYNTAX_ERROR:
      printf("Syntax error at position %zu.\n", tokenizer.index);
      return EXIT_FAILURE;

    case PJSON_STATUS_UTF8_ERROR:
      printf("UTF-8 encoding error at position %zu.\n", tokenizer.index);
      return EXIT_FAILURE;

    default:
      printf("Unexpected error (%d).\n", status);
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
