#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "pjson.h"

#define DEMO_PARSER_MAX_DEPTH (100)

typedef struct {
  pjson_parser_context base;
  size_t counter;
} demo_parser_context;

typedef struct {
  pjson_parser base;
  // For simplicity's sake, we use a fixed-size buffer for storing the context stack.
  // In real life, when maximum depth of the parsed JSON is not known beforehand,
  // you may want to use a dynamically allocated data structure for this purpose.
  demo_parser_context context_stack[DEMO_PARSER_MAX_DEPTH];
  size_t context_stack_current_index;

  pjson_token_type toplevel_datatype;
  size_t max_depth;
  size_t max_array_item_count;
  size_t max_object_property_count;
  size_t datatype_counts[PJSON_TOKEN_EOS - PJSON_TOKEN_NULL];
  size_t key_count;
} demo_parser;

/* Parser context stack implementation */

static pjson_parsing_status demo_parser_push_context(demo_parser *parser) {
  size_t next_index = parser->context_stack_current_index + 1u;
  if (next_index >= pjson_countof(parser->context_stack)) {
    return PJSON_STATUS_MAX_DEPTH_EXCEEDED;
  }

  parser->context_stack_current_index = next_index;
  return PJSON_STATUS_SUCCESS;
}

static demo_parser_context *demo_parser_peek_context(demo_parser *parser, bool previous) {
  assert(parser->context_stack_current_index - (size_t)previous < pjson_countof(parser->context_stack));
  return &parser->context_stack[parser->context_stack_current_index - (size_t)previous];
}

static void demo_parser_pop_context(demo_parser *parser) {
  assert(0 < parser->context_stack_current_index && parser->context_stack_current_index < pjson_countof(parser->context_stack));
  parser->context_stack_current_index--;
}

/* Parser callbacks */

static pjson_parsing_status demo_parser_on_value_core(demo_parser *parser, demo_parser_context *context, const pjson_token *token);

static pjson_parsing_status demo_parser_on_value_at_toplevel(demo_parser *parser, demo_parser_context *context, const pjson_token *token) {
  // For arrays and objects it is called twice: once when beginning and once when finishing parsing the value.
  // So in the case of an array or object, we will need to look for PJSON_TOKEN_CLOSE_BRACKET or PJSON_TOKEN_CLOSE_BRACE
  // when identifying data types.
  parser->toplevel_datatype = token->type;

  return demo_parser_on_value_core(parser, context, token);
}

static pjson_parsing_status demo_parser_on_value_in_array_or_object(demo_parser *parser, demo_parser_context *context, const pjson_token *token) {
  // Count items or properties in the current array or object.
  context->counter++;

  return demo_parser_on_value_core(parser, context, token);
}

static pjson_parsing_status demo_parser_on_object_property_name(demo_parser *parser, demo_parser_context *context, const pjson_token *token) {
  (void)context;
  (void)token;

  // Count property keys.
  parser->key_count++;

  return PJSON_STATUS_SUCCESS;
}

static pjson_parsing_status demo_parser_on_value_core(demo_parser *parser, demo_parser_context *context, const pjson_token *token) {
  (void)context;
  demo_parser_context *child_context;

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
      child_context = demo_parser_peek_context(parser, false);
      // Don't forget to initialize your additional state in the context struct!
      // pjson only initializes the parts which it's aware of.
      child_context->counter = 0;
      child_context->base.on_value = (pjson_parser_context_callback)&demo_parser_on_value_in_array_or_object;
      child_context->base.on_object_property_name = (pjson_parser_context_callback)&demo_parser_on_object_property_name;

      // Calculate maximum depth.
      if (parser->max_depth < parser->context_stack_current_index) {
        parser->max_depth = parser->context_stack_current_index;
      }
      break;

    case PJSON_TOKEN_CLOSE_BRACKET:
      // Calculate maximum number of items in arrays.
      child_context = demo_parser_peek_context(parser, false);
      if (parser->max_array_item_count < child_context->counter) {
        parser->max_array_item_count = child_context->counter;
      }
      goto RecordDataTypeOccurrence;

    case PJSON_TOKEN_CLOSE_BRACE:
      // Calculate maximum number of properties in objects.
      child_context = demo_parser_peek_context(parser, false);
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

  demo_parser parser;
  // Don't forget to initialize your additional state in the parser struct!
  // pjson only initializes the parts which it's aware of.
  memset(((uint8_t *)&parser) + sizeof(parser.base), 0, sizeof(parser) - sizeof(parser.base));
  parser.context_stack_current_index = (size_t)-1;

  pjson_parser_init(&parser.base, true,
    // The pointer casts below are safe only because `demo_parser` is defined with its first member being the
    // `pjson_parser` "base" structure. The callbacks are called with the pointer to the base struct, so in the
    // case of a different memory layout, you're better off using a compatible callback signature here and restoring
    // the pointer to your actual context and parser structs in the callbacks.
    (pjson_parser_push_context)&demo_parser_push_context,
    (pjson_parser_peek_context)&demo_parser_peek_context,
    (pjson_parser_pop_context)&demo_parser_pop_context);

  // `pjson_parser_init` pushes the top-level context onto the stack but unless we set a callback to observe the tokens,
  // the parser will just consume them and validate the syntax of the JSON. We need to set an `on_value`
  // (or `on_object_property_name`) callback to hook into the parsing process if we want do anything meaningful.
  demo_parser_peek_context(&parser, false)->base.on_value =
    // The same considerations apply for casting this function pointer as above.
    (pjson_parser_context_callback)&demo_parser_on_value_at_toplevel;

  pjson_parsing_status status = -1; // unnecessary assignment, just for keeping the compiler happy
  bool value_found = false;

  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser.base.base);

  char buf[128];
  int num_read;

  // This example also shows how to parse continuous stream of multiple JSON values.

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
          printf("Maximum depth of %d exceeded.\n", DEMO_PARSER_MAX_DEPTH);
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
    memset(((uint8_t *)&parser) + sizeof(parser.base), 0, sizeof(parser) - sizeof(parser.base));
    parser.context_stack_current_index = (size_t)-1;
    pjson_parser_reset(&parser.base, true);
    demo_parser_peek_context(&parser, false)->base.on_value =
      (pjson_parser_context_callback)&demo_parser_on_value_at_toplevel;

    // If the current data hasn't been consumed entirely, consume the rest.
    if ((uintptr_t)tokenizer.token_start - (uintptr_t)buf < (uintptr_t)num_read) {
      status = pjson_feed(&tokenizer, tokenizer.token_start, (uintptr_t)(buf + num_read) - (uintptr_t)tokenizer.token_start);
      goto Resume;
    }

    // Otherwise, read the next chunk of data and resume parsing normally.
  }
}
