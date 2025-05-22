#ifndef __TOPLEVEL_VALUE_PARSER_H__
#define __TOPLEVEL_VALUE_PARSER_H__

#include <assert.h>
#include <string.h>
#include "pjson.h"

/* Parser definition */

typedef struct {
  pjson_parser base; // base struct MUST be the first member!

  pjson_parser_context context;
  bool context_pushed;

  pjson_token token;
} tlv_parser;

/* Parser context stack implementation */

static pjson_parsing_status tlv_parser_push_context(tlv_parser *parser) {
  if (parser->context_pushed) {
    return PJSON_STATUS_MAX_DEPTH_EXCEEDED;
  }
  parser->context_pushed = true;
  return PJSON_STATUS_SUCCESS;
}

static pjson_parser_context *tlv_parser_peek_context(tlv_parser *parser, bool previous) {
  (void)previous;
  assert(parser->context_pushed);
  return &parser->context;
}

static void tlv_parser_pop_context(tlv_parser *parser) {
  assert(parser->context_pushed);
  parser->context_pushed = false;
}

/* Parser implementation */

static pjson_parsing_status parse_toplevel_value(tlv_parser *parser, pjson_parser_context *context, const pjson_token *token) {
  (void)context;
  switch (token->type) {
    case PJSON_TOKEN_NULL:
    case PJSON_TOKEN_FALSE:
    case PJSON_TOKEN_TRUE:
    case PJSON_TOKEN_NUMBER:
    case PJSON_TOKEN_STRING: {
      parser->token = *token;
      parser->token.start = (uint8_t *)pjson_malloc(token->length);
      if (!parser->token.start) {
        return PJSON_STATUS_OUT_OF_MEMORY;
      }
      memcpy((uint8_t *)parser->token.start, token->start, token->length);
      return PJSON_STATUS_SUCCESS;
    }
    default:
      return PJSON_STATUS_USER_ERROR;
  }
}

/* Parser init */

static void tlv_parser_init(tlv_parser *parser) {
  // `pjson_parser_init` will push the top-level context onto the stack, so the stack MUST be initialized beforehand!
  parser->context_pushed = false;
  memset(&parser->context, 0, sizeof(parser->context));
  memset(&parser->token, 0, sizeof(parser->token));

  pjson_parser_init(&parser->base, false,
    (pjson_parser_push_context)&tlv_parser_push_context,
    (pjson_parser_peek_context)&tlv_parser_peek_context,
    (pjson_parser_pop_context)&tlv_parser_pop_context);

  tlv_parser_peek_context(parser, false)->on_value =
    (pjson_parser_context_callback)&parse_toplevel_value;
}

#endif // __TOPLEVEL_VALUE_PARSER_H__
