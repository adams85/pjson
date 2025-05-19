#ifndef __DATASTRUCT_PARSER_H__
#define __DATASTRUCT_PARSER_H__

#include <assert.h>
#include "pjson.h"
#include "vec.h"

/* Data structure definition */

const char PROPERTY_NAME_ID[] = "id";
const char PROPERTY_NAME_NAME[] = "name";
const char PROPERTY_NAME_RATING[] = "rating";

typedef struct {
  int32_t id;
  uint8_t *name;
  double rating;
} item_t;

typedef VEC_PRE_ALIGN struct { vec_define_fields(item_t) } vec_item_t VEC_POST_ALIGN;

void free_items(vec_item_t *items) {
  if (!items) return;
  vec_size_t i;
  item_t *item;
  vec_foreach_ptr(items, item, i) {
    if (item->name) pjson_free(item->name);
  }
  vec_deinit(items);
}

/* Parser and parser context definition */

typedef struct {
  pjson_parser_context base; // base struct MUST be the first member!

  union {
    vec_item_t *items_ptr; // for top-level context
    vec_item_t items; // items array context
    struct {
      item_t *ptr;
      void *current_member;
    } item;
  } data; // item object context
} ds_parser_context;

typedef struct {
  pjson_parser base; // base struct MUST be the first member!

  ds_parser_context context_stack[3];
  size_t context_stack_current_index;
} ds_parser;

/* Parser context stack implementation */

static pjson_parsing_status ds_parser_push_context(ds_parser *parser) {
  size_t next_index = parser->context_stack_current_index + 1u;
  if (next_index >= pjson_countof(parser->context_stack)) {
    return PJSON_STATUS_MAX_DEPTH_EXCEEDED;
  }
  parser->context_stack_current_index = next_index;
  return PJSON_STATUS_SUCCESS;
}

static ds_parser_context *ds_parser_peek_context(ds_parser *parser, bool previous) {
  assert(parser->context_stack_current_index - (size_t)previous < pjson_countof(parser->context_stack));
  return &parser->context_stack[parser->context_stack_current_index - (size_t)previous];
}

static void ds_parser_pop_context(ds_parser *parser) {
  assert(0 < parser->context_stack_current_index && parser->context_stack_current_index < pjson_countof(parser->context_stack));
  parser->context_stack_current_index--;
}

/* Parser implementation */

static pjson_parsing_status parse_string_property_value(ds_parser *parser, ds_parser_context *context, const pjson_token *token) {
  (void)parser;
  uint8_t **value_ptr = (uint8_t **)context->data.item.current_member;
  *value_ptr = (uint8_t *)pjson_malloc(token->unescaped_length + 1);
  if (!*value_ptr) return PJSON_STATUS_OUT_OF_MEMORY;
  pjson_parse_string(*value_ptr, token->unescaped_length, token->start, token->length);
  (*value_ptr)[token->unescaped_length] = 0;
  return PJSON_STATUS_SUCCESS;
}

static pjson_parsing_status parse_int32_property_value(ds_parser *parser, ds_parser_context *context, const pjson_token *token) {
  (void)parser;
  int32_t *value_ptr = (int32_t *)context->data.item.current_member;
  if (!pjson_parse_int32(value_ptr, token->start, token->length)) return PJSON_STATUS_USER_ERROR;
  return PJSON_STATUS_SUCCESS;
}

static pjson_parsing_status parse_double_property_value(ds_parser *parser, ds_parser_context *context, const pjson_token *token) {
  (void)parser;
  double *value_ptr = (double *)context->data.item.current_member;
  if (!pjson_parse_double(value_ptr, token->start, token->length)) return PJSON_STATUS_USER_ERROR;
  return PJSON_STATUS_SUCCESS;
}

static const char *get_property_name(const pjson_token *token) {
  if (token->unescaped_length == token->length - 2) {
    return (const char *)token->start + 1;
  }

  uint8_t *property_name = (uint8_t *)pjson_malloc(token->unescaped_length);
  if (!property_name) return NULL;
  pjson_parse_string(property_name, token->unescaped_length, token->start, token->length);
  return (const char *)property_name;
}

static void free_property_name(const char *property_name, const pjson_token *token) {
  if ((uint8_t *)property_name != token->start + 1) {
    pjson_free((char *)property_name);
  }
}

static pjson_parsing_status parse_item_property_name(ds_parser *parser, ds_parser_context *context, const pjson_token *token) {
  (void)parser;
  const char *property_name = get_property_name(token);
  if (!property_name) return PJSON_STATUS_OUT_OF_MEMORY;

  if (strncmp(property_name, PROPERTY_NAME_ID, token->unescaped_length) == 0) {
    context->data.item.current_member = &context->data.item.ptr->id;
    context->base.on_value = (pjson_parser_context_callback)&parse_int32_property_value;
  }
  else if (strncmp(property_name, PROPERTY_NAME_NAME, token->unescaped_length) == 0) {
    context->data.item.current_member = &context->data.item.ptr->name;
    context->base.on_value = (pjson_parser_context_callback)&parse_string_property_value;
  }
  else if (strncmp(property_name, PROPERTY_NAME_RATING, token->unescaped_length) == 0) {
    context->data.item.current_member = &context->data.item.ptr->rating;
    context->base.on_value = (pjson_parser_context_callback)&parse_double_property_value;
  }
  else {
    free_property_name(property_name, token);
    return PJSON_STATUS_USER_ERROR;
  }

  free_property_name(property_name, token);
  return PJSON_STATUS_SUCCESS;
}

static pjson_parsing_status parse_item(ds_parser *parser, ds_parser_context *context, const pjson_token *token) {
  (void)parser;
  if (token->type == PJSON_TOKEN_OPEN_BRACE) { // item starts
    vec_expand_(vec_unpack_(&context->data.items)), context->data.items.length++;
    ds_parser_context *child_context = ds_parser_peek_context(parser, false);
    child_context->data.item.ptr = &vec_last(&context->data.items);
    memset(child_context->data.item.ptr, 0, sizeof(item_t));
    child_context->base.on_object_property_name = (pjson_parser_context_callback)&parse_item_property_name;
  }
  else if (token->type == PJSON_TOKEN_CLOSE_BRACE) { // item ends
  }
  else {
    return PJSON_STATUS_USER_ERROR; // data not matching the expected schema
  }

  return PJSON_STATUS_SUCCESS;
}

static pjson_parsing_status parse_items(ds_parser *parser, ds_parser_context *context, const pjson_token *token) {
  if (token->type == PJSON_TOKEN_OPEN_BRACKET) { // array of items starts
    ds_parser_context *child_context = ds_parser_peek_context(parser, false);
    vec_init(&child_context->data.items);
    context->data.items_ptr = &child_context->data.items;
    child_context->base.on_value = (pjson_parser_context_callback)&parse_item;
  }
  else if (token->type == PJSON_TOKEN_CLOSE_BRACKET) { // array of items ends
  }
  else {
    return PJSON_STATUS_USER_ERROR; // data not matching the expected schema
  }

  return PJSON_STATUS_SUCCESS;
}

/* Parser init */

static void ds_parser_init(ds_parser *parser) {
  // `pjson_parser_init` will push the top-level context onto the stack, so the stack MUST be initialized beforehand!
  parser->context_stack_current_index = (size_t)-1;
  memset(parser->context_stack, 0, sizeof(parser->context_stack));

  pjson_parser_init(&parser->base, false,
    (pjson_parser_push_context)&ds_parser_push_context,
    (pjson_parser_peek_context)&ds_parser_peek_context,
    (pjson_parser_pop_context)&ds_parser_pop_context);

  ds_parser_peek_context(parser, false)->base.on_value =
    (pjson_parser_context_callback)&parse_items;
}

#endif // __DATASTRUCT_PARSER_H__
