#include <assert.h>
#include <string.h>

#include "unity_fixture.h"
#include "pjson.h"
#include "datastruct_parser.h"

TEST_GROUP(parse_datastruct);

TEST_SETUP(parse_datastruct) {}

TEST_TEAR_DOWN(parse_datastruct) {}

static pjson_parsing_status parse_string(ds_parser *parser, const char *input, pjson_parsing_status *feed_status) {
  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser->base.base);
  pjson_parsing_status status = -1; // unnecessary assignment, just for keeping the compiler happy

  char buf[16];

  for (size_t offset = 0, len = strlen(input); offset < len; offset += sizeof(buf)) {
    memset(buf, 0, sizeof(buf)); // although it wouldn't be necessary, zero the buffer to make it easier to investigate potential errors
    size_t block_length = len - offset;
    if (block_length > sizeof(buf)) block_length = sizeof(buf);
    memcpy(buf, input + offset, block_length);
    status = pjson_feed(&tokenizer, (uint8_t *)buf, block_length);
    if (status != PJSON_STATUS_DATA_NEEDED) break;
  }

  if (feed_status) *feed_status = status;
  return pjson_close(&tokenizer);
}

TEST(parse_datastruct, test_parse_valid_datastruct) {
  ds_parser parser;
  ds_parser_init(&parser);

  static char input[] = "[\n\
    { \"id\": -2147483648, \"name\": \"Alice\", \"rating\": 4.2 },\n\
    { \"i\\u0064\": 0, \"name\": \"B\\uD83D\\uDE00b\", \"rating\": 38e-1 },\n\
    { \"id\": 2147483647, \"name\": \"Charlie\", \"rating\": -0.5E0 }\n\
  ]";

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  vec_item_t *items = parser.context_stack[0].data.items_ptr;
  TEST_ASSERT_EQUAL(3, items->length);

  item_t *item = &items->data[0];
  TEST_ASSERT_EQUAL_INT32(-2147483647 - 1, item->id);
  TEST_ASSERT_EQUAL_STRING("Alice", item->name);
  TEST_ASSERT_EQUAL_DOUBLE(4.2, item->rating);

  item = &items->data[1];
  TEST_ASSERT_EQUAL_INT32(0, item->id);
  TEST_ASSERT_EQUAL_STRING("B\360\237\230\200b", item->name);
  TEST_ASSERT_EQUAL_DOUBLE(3.8, item->rating);

  item = &items->data[2];
  TEST_ASSERT_EQUAL_INT32(2147483647, item->id);
  TEST_ASSERT_EQUAL_STRING("Charlie", item->name);
  TEST_ASSERT_EQUAL_DOUBLE(-0.5, item->rating);

  free_items(items);
}

TEST(parse_datastruct, test_parse_invalid_datastruct) {
  ds_parser parser;
  ds_parser_init(&parser);

  static char input[] = "[\n\
    { \"id\": -2147483648, \"name\": \"Alice\", \"rating\": 4.2 },\n\
    { \"i\\u0064\": 0, \"name\": \"B\\uD83D\\uDE00b\", \"rating\": 38e-1 },\n\
    { \"id\": 2147483648, \"name\": \"Charlie\", \"rating\": -0.5E0 }\n\
  ]";

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_USER_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_USER_ERROR, close_status);

  vec_item_t *items = parser.context_stack[0].data.items_ptr;
  TEST_ASSERT_EQUAL(3, items->length);

  item_t *item = &items->data[0];
  TEST_ASSERT_EQUAL_INT32(-2147483647 - 1, item->id);
  TEST_ASSERT_EQUAL_STRING("Alice", item->name);
  TEST_ASSERT_EQUAL_DOUBLE(4.2, item->rating);

  item = &items->data[1];
  TEST_ASSERT_EQUAL_INT32(0, item->id);
  TEST_ASSERT_EQUAL_STRING("B\360\237\230\200b", item->name);
  TEST_ASSERT_EQUAL_DOUBLE(3.8, item->rating);

  item = &items->data[2];
  TEST_ASSERT_EQUAL_INT32(0, item->id);
  TEST_ASSERT_NULL(item->name);
  TEST_ASSERT_EQUAL_DOUBLE(0, item->rating);

  free_items(items);
}

TEST_GROUP_RUNNER(parse_datastruct) {
  RUN_TEST_CASE(parse_datastruct, test_parse_valid_datastruct);
  RUN_TEST_CASE(parse_datastruct, test_parse_invalid_datastruct);
}
