#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "unity_fixture.h"
#include "pjson.h"
#include "stats_parser.h"

TEST_GROUP(feed_fuzzy);

TEST_SETUP(feed_fuzzy) {}

TEST_TEAR_DOWN(feed_fuzzy) {}

typedef struct {
  pjson_token_type type;
  size_t pos;
} token_info;

static pjson_parsing_status parse_file_using_random_size_chunks(stats_parser *parser, char *file_path, token_info *token_info) {
  stats_parser_init(parser, false);

  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser->base.base);

  pjson_parsing_status status;
  char buf[128];
  size_t num_to_read, num_read;
  FILE *file;

  file = fopen(file_path, "rb");
  TEST_ASSERT_NOT_NULL(file);

  for (;;) {
    memset(buf, 0, sizeof(buf)); // although it wouldn't be necessary, zero the buffer to make it easier to investigate potential errors

    num_to_read = (rand() % (sizeof(buf) - 4)) + 4; // get a random number between 4 and sizeof(buf) - 1
    num_read = fread(buf, 1, num_to_read, file);
    if (num_read <= 0) {
      TEST_ASSERT_EQUAL(0, num_read);
      break;
    }

    status = pjson_feed(&tokenizer, (uint8_t *)buf, num_read);
    if (status != PJSON_STATUS_DATA_NEEDED) break;
  }

  status = pjson_close(&tokenizer);

  fclose(file);

  if (token_info) {
    token_info->type = tokenizer.token_type;
    token_info->pos = tokenizer.token_start_index;
  }
  return status;
}

TEST(feed_fuzzy, test_parse_formatted_1mb) {
  stats_parser parser;

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, parse_file_using_random_size_chunks(&parser, "test/data/formatted_1mb.json", NULL));

  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACKET, parser.toplevel_datatype);
  TEST_ASSERT_EQUAL(4, parser.max_depth);
  TEST_ASSERT_EQUAL(1550, parser.max_array_item_count);
  TEST_ASSERT_EQUAL(24, parser.max_object_property_count);
  TEST_ASSERT_EQUAL(21700, parser.key_count);

  TEST_ASSERT_EQUAL(0, parser.datatype_counts[PJSON_TOKEN_NULL - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(381, parser.datatype_counts[PJSON_TOKEN_FALSE - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(394, parser.datatype_counts[PJSON_TOKEN_TRUE - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(5425, parser.datatype_counts[PJSON_TOKEN_NUMBER - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(19375, parser.datatype_counts[PJSON_TOKEN_STRING - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(1551, parser.datatype_counts[PJSON_TOKEN_CLOSE_BRACKET - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(3100, parser.datatype_counts[PJSON_TOKEN_CLOSE_BRACE - PJSON_TOKEN_NULL]);
}

TEST(feed_fuzzy, test_parse_minified_1mb) {
  stats_parser parser;

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, parse_file_using_random_size_chunks(&parser, "test/data/minified_1mb.json", NULL));

  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACKET, parser.toplevel_datatype);
  TEST_ASSERT_EQUAL(4, parser.max_depth);
  TEST_ASSERT_EQUAL(2000, parser.max_array_item_count);
  TEST_ASSERT_EQUAL(24, parser.max_object_property_count);
  TEST_ASSERT_EQUAL(28000, parser.key_count);

  TEST_ASSERT_EQUAL(0, parser.datatype_counts[PJSON_TOKEN_NULL - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(492, parser.datatype_counts[PJSON_TOKEN_FALSE - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(508, parser.datatype_counts[PJSON_TOKEN_TRUE - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(7000, parser.datatype_counts[PJSON_TOKEN_NUMBER - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(25000, parser.datatype_counts[PJSON_TOKEN_STRING - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(2001, parser.datatype_counts[PJSON_TOKEN_CLOSE_BRACKET - PJSON_TOKEN_NULL]);
  TEST_ASSERT_EQUAL(4000, parser.datatype_counts[PJSON_TOKEN_CLOSE_BRACE - PJSON_TOKEN_NULL]);
}

TEST(feed_fuzzy, test_parse_invalid_binary_data) {
  stats_parser parser;
  token_info token_info;

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, parse_file_using_random_size_chunks(&parser, "test/data/invalid_binary_data.json", &token_info));
  TEST_ASSERT_EQUAL(PJSON_TOKEN_ERROR, token_info.type);
  TEST_ASSERT_EQUAL(47, token_info.pos);
}

TEST(feed_fuzzy, test_parse_invalid_missing_colon) {
  stats_parser parser;
  token_info token_info;

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, parse_file_using_random_size_chunks(&parser, "test/data/invalid_missing_colon.json", &token_info));
  TEST_ASSERT_EQUAL(PJSON_TOKEN_STRING, token_info.type);
  TEST_ASSERT_EQUAL(22275, token_info.pos);
}

TEST(feed_fuzzy, test_parse_invalid_unterminated_string) {
  stats_parser parser;
  token_info token_info;

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, parse_file_using_random_size_chunks(&parser, "test/data/invalid_unterminated_string.json", &token_info));
  TEST_ASSERT_EQUAL(PJSON_TOKEN_ERROR, token_info.type);
  TEST_ASSERT_EQUAL(30029, token_info.pos);
}

TEST_GROUP_RUNNER(feed_fuzzy) {
  RUN_TEST_CASE(feed_fuzzy, test_parse_formatted_1mb);
  RUN_TEST_CASE(feed_fuzzy, test_parse_minified_1mb);
  RUN_TEST_CASE(feed_fuzzy, test_parse_invalid_binary_data);
  RUN_TEST_CASE(feed_fuzzy, test_parse_invalid_missing_colon);
  RUN_TEST_CASE(feed_fuzzy, test_parse_invalid_unterminated_string);
}
