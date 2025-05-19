#ifndef __STATS_PARSER_H__
#define __STATS_PARSER_H__

#include "pjson.h"

#ifndef STATS_PARSER_MAX_DEPTH
#define STATS_PARSER_MAX_DEPTH (100)
#endif // STATS_PARSER_MAX_DEPTH

typedef struct {
  pjson_parser_context base; // base struct MUST be the first member!
  size_t counter;
} stats_parser_context;

typedef struct {
  pjson_parser base; // base struct MUST be the first member!

  // For simplicity's sake, we use a fixed-size buffer for storing the context stack.
  // In real life, when maximum depth of the parsed JSON is not known beforehand,
  // you may want to use a dynamically allocated data structure for this purpose.
  stats_parser_context context_stack[STATS_PARSER_MAX_DEPTH];
  size_t context_stack_current_index;

  pjson_token_type toplevel_datatype;
  size_t max_depth;
  size_t max_array_item_count;
  size_t max_object_property_count;
  size_t datatype_counts[PJSON_TOKEN_EOS - PJSON_TOKEN_NULL];
  size_t key_count;
} stats_parser;

/* Parser context stack implementation */

static pjson_parsing_status stats_parser_push_context(stats_parser *parser) {
  size_t next_index = parser->context_stack_current_index + 1u;
  if (next_index >= pjson_countof(parser->context_stack)) {
    return PJSON_STATUS_MAX_DEPTH_EXCEEDED;
  }
  parser->context_stack_current_index = next_index;
  return PJSON_STATUS_SUCCESS;
}

static stats_parser_context *stats_parser_peek_context(stats_parser *parser, bool previous) {
  assert(parser->context_stack_current_index - (size_t)previous < pjson_countof(parser->context_stack));
  return &parser->context_stack[parser->context_stack_current_index - (size_t)previous];
}

static void stats_parser_pop_context(stats_parser *parser) {
  assert(0 < parser->context_stack_current_index && parser->context_stack_current_index < pjson_countof(parser->context_stack));
  parser->context_stack_current_index--;
}

/* Parser callbacks */

static pjson_parsing_status stats_parser_on_value_core(stats_parser *parser, stats_parser_context *context, const pjson_token *token);

static pjson_parsing_status stats_parser_on_value_at_toplevel(stats_parser *parser, stats_parser_context *context, const pjson_token *token) {
  // For arrays and objects it is called twice: once when beginning and once when finishing parsing the value.
  // So in the case of an array or object, we will need to look for PJSON_TOKEN_CLOSE_BRACKET or PJSON_TOKEN_CLOSE_BRACE
  // when identifying data types.
  parser->toplevel_datatype = token->type;

  return stats_parser_on_value_core(parser, context, token);
}

static pjson_parsing_status stats_parser_on_value_in_array_or_object(stats_parser *parser, stats_parser_context *context, const pjson_token *token) {
  // Count items or properties in the current array or object.
  context->counter++;

  return stats_parser_on_value_core(parser, context, token);
}

static pjson_parsing_status stats_parser_on_object_property_name(stats_parser *parser, stats_parser_context *context, const pjson_token *token) {
  (void)context;
  (void)token;

  // Count property keys.
  parser->key_count++;

  return PJSON_STATUS_SUCCESS;
}

static pjson_parsing_status stats_parser_on_value_core(stats_parser *parser, stats_parser_context *context, const pjson_token *token) {
  (void)context;
  stats_parser_context *child_context;

  switch (token->type) {
    case PJSON_TOKEN_NULL:
    case PJSON_TOKEN_FALSE:
    case PJSON_TOKEN_TRUE:
    case PJSON_TOKEN_NUMBER:
    case PJSON_TOKEN_STRING:
    RecordDataTypeOccurrence:
      assert((size_t)token->type - (size_t)PJSON_TOKEN_NULL < pjson_countof(parser->datatype_counts));
      parser->datatype_counts[token->type - PJSON_TOKEN_NULL]++;
      break;

    case PJSON_TOKEN_OPEN_BRACKET:
    case PJSON_TOKEN_OPEN_BRACE:
      // When an array or object is beginning (or ending), the new context is already (or still) on the stack,
      // however the `context` parameter points to the original context. You can obtain the new context from the stack.
      child_context = stats_parser_peek_context(parser, false);
      // Don't forget to initialize your additional state in the context struct!
      // pjson only initializes the parts which it's aware of.
      child_context->counter = 0;
      child_context->base.on_value = (pjson_parser_context_callback)&stats_parser_on_value_in_array_or_object;
      child_context->base.on_object_property_name = (pjson_parser_context_callback)&stats_parser_on_object_property_name;

      // Calculate maximum depth.
      if (parser->max_depth < parser->context_stack_current_index) {
        parser->max_depth = parser->context_stack_current_index;
      }
      break;

    case PJSON_TOKEN_CLOSE_BRACKET:
      // Calculate maximum number of items in arrays.
      child_context = stats_parser_peek_context(parser, false);
      if (parser->max_array_item_count < child_context->counter) {
        parser->max_array_item_count = child_context->counter;
      }
      goto RecordDataTypeOccurrence;

    case PJSON_TOKEN_CLOSE_BRACE:
      // Calculate maximum number of properties in objects.
      child_context = stats_parser_peek_context(parser, false);
      if (parser->max_array_item_count < child_context->counter) {
        parser->max_object_property_count = child_context->counter;
      }
      goto RecordDataTypeOccurrence;

    default:
      break;
  }

  // We need to return PJSON_STATUS_SUCCESS to continue parsing.
  return PJSON_STATUS_SUCCESS;
}

/* Init */

static void stats_parser_init(stats_parser *parser, bool is_lazy) {
  // Don't forget to initialize your additional state in the parser struct!
  // `pjson_parser_init` only initializes the part which it's aware of.
  memset(((uint8_t *)parser) + sizeof(parser->base), 0, sizeof(*parser) - sizeof(parser->base));
  parser->context_stack_current_index = (size_t)-1;

  pjson_parser_init(&parser->base, is_lazy,
    // The pointer casts below are safe only because `stats_parser` is defined with its first member being the
    // `pjson_parser` "base" structure. The callbacks are called with the pointer to the base struct, so in the
    // case of a different memory layout, you're better off using a compatible callback signature here and restoring
    // the pointer to your actual context and parser structs in the callbacks.
    (pjson_parser_push_context)&stats_parser_push_context,
    (pjson_parser_peek_context)&stats_parser_peek_context,
    (pjson_parser_pop_context)&stats_parser_pop_context);

  // `pjson_parser_init` pushes the top-level context onto the stack but unless we set a callback to observe the tokens,
  // the parser will just consume them and validate the syntax of the JSON. We need to set an `on_value`
  // (or `on_object_property_name`) callback to hook into the parsing process if we want do anything useful.
  stats_parser_peek_context(parser, false)->base.on_value =
    // The same considerations apply for casting this function pointer as above.
    (pjson_parser_context_callback)&stats_parser_on_value_at_toplevel;
}

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static void stats_parser_reset(stats_parser *parser, bool is_lazy) {
  memset(((uint8_t *)parser) + sizeof(parser->base), 0, sizeof(*parser) - sizeof(parser->base));
  parser->context_stack_current_index = (size_t)-1;

  pjson_parser_reset(&parser->base, is_lazy);

  stats_parser_peek_context(parser, false)->base.on_value =
    (pjson_parser_context_callback)&stats_parser_on_value_at_toplevel;
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#endif // __STATS_PARSER_H__
