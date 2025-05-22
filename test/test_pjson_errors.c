#include <assert.h>
#include <string.h>

#include "unity_fixture.h"
#include "pjson.h"

TEST_GROUP(errors);

TEST_SETUP(errors) {}

TEST_TEAR_DOWN(errors) {}

static pjson_parsing_status feed_string(pjson_tokenizer *tokenizer, const char *input) {
  return pjson_feed(tokenizer, (uint8_t *)input, strlen(input));
}

static pjson_parsing_status parse_string(const char *input, pjson_parsing_status *feed_status, size_t *error_index) {
  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, NULL);
  pjson_parsing_status status = feed_string(&tokenizer, input);
  if (feed_status) *feed_status = status;
  pjson_parsing_status close_status = pjson_close(&tokenizer);
  if (error_index) *error_index = tokenizer.token_start_index;
  return close_status;
}

/* UTF-8 encoding errors */

TEST(errors, test_parse_utf8_2_byte_sequence_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\337", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_utf8_2_byte_sequence_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\337\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_utf8_3_byte_sequence_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\357\277", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_utf8_3_byte_sequence_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\357\277\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_utf8_4_byte_sequence_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\360\220\200", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_utf8_4_byte_sequence_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\360\220\200\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_utf8_high_surrogate) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\355\237\277 \355\240\200\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(5, error_index);
}

TEST(errors, test_parse_utf8_low_surrogate) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\356\200\200 \355\277\277\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_UTF8_ERROR, close_status);
  TEST_ASSERT_EQUAL(5, error_index);
}

/* Escape sequence errors */

TEST(errors, test_parse_escape_sequence_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string(" \"\\", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_escape_sequence_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

TEST(errors, test_parse_invalid_escape_sequence_null) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\0\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

TEST(errors, test_parse_invalid_escape_sequence_verticaltab) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\v\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

TEST(errors, test_parse_invalid_escape_sequence_apostrophe) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\'\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

TEST(errors, test_parse_unicode_escape_sequence_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string(" \"\\uD80", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_unicode_escape_sequence_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\uD80\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

TEST(errors, test_parse_high_surrogate_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string(" \"\\uD800\\", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_high_surrogate_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\uD800\\\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

TEST(errors, test_parse_surrogate_pair_unterminated_input) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string(" \"\\uD800\\udc0", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_surrogate_pair_unterminated_string) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("\"\\uD800\\udc0\"", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(0, error_index);
}

/* Unexpected characters */

TEST(errors, test_parse_invalid_character_null) {
  pjson_parsing_status feed_status;
  size_t error_index;

  static char input[] = "[null, 0, \"0\", \0]";

  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, NULL);
  pjson_parsing_status status = pjson_feed(&tokenizer, (uint8_t *)input, sizeof(input) - 1);
  feed_status = status;
  pjson_parsing_status close_status = pjson_close(&tokenizer);
  error_index = tokenizer.token_start_index;

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(15, error_index);
}

TEST(errors, test_parse_invalid_character_in_keyword) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[nvll, 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_invalid_punctuator_directly_after_keyword) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[null; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_invalid_punctuator_after_keyword) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[null ; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(6, error_index);
}

TEST(errors, test_parse_invalid_punctuator_directly_after_zero) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[0; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_invalid_punctuator_after_zero) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[0 ; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(3, error_index);
}

TEST(errors, test_parse_invalid_punctuator_directly_after_integer) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[123; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_invalid_punctuator_after_integer) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[123 ; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(5, error_index);
}

TEST(errors, test_parse_invalid_punctuator_directly_after_fraction) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[0.1; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_invalid_punctuator_after_fraction) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[0.1 ; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(5, error_index);
}

TEST(errors, test_parse_invalid_punctuator_directly_after_exponent) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[0.1e1; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(1, error_index);
}

TEST(errors, test_parse_invalid_punctuator_after_exponent) {
  pjson_parsing_status feed_status;
  size_t error_index;
  pjson_parsing_status close_status = parse_string("[0.1e1 ; 0]", &feed_status, &error_index);

  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(7, error_index);
}

TEST_GROUP_RUNNER(errors) {
  RUN_TEST_CASE(errors, test_parse_utf8_2_byte_sequence_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_utf8_2_byte_sequence_unterminated_string);
  RUN_TEST_CASE(errors, test_parse_utf8_3_byte_sequence_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_utf8_3_byte_sequence_unterminated_string);
  RUN_TEST_CASE(errors, test_parse_utf8_4_byte_sequence_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_utf8_4_byte_sequence_unterminated_string);
  RUN_TEST_CASE(errors, test_parse_utf8_high_surrogate);
  RUN_TEST_CASE(errors, test_parse_utf8_low_surrogate);

  RUN_TEST_CASE(errors, test_parse_escape_sequence_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_escape_sequence_unterminated_string);
  RUN_TEST_CASE(errors, test_parse_invalid_escape_sequence_null);
  RUN_TEST_CASE(errors, test_parse_invalid_escape_sequence_verticaltab);
  RUN_TEST_CASE(errors, test_parse_invalid_escape_sequence_apostrophe);
  RUN_TEST_CASE(errors, test_parse_unicode_escape_sequence_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_unicode_escape_sequence_unterminated_string);
  RUN_TEST_CASE(errors, test_parse_high_surrogate_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_high_surrogate_unterminated_string);
  RUN_TEST_CASE(errors, test_parse_surrogate_pair_unterminated_input);
  RUN_TEST_CASE(errors, test_parse_surrogate_pair_unterminated_string);

  RUN_TEST_CASE(errors, test_parse_invalid_character_null);
  RUN_TEST_CASE(errors, test_parse_invalid_character_in_keyword);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_directly_after_keyword);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_after_keyword);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_directly_after_zero);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_after_zero);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_directly_after_integer);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_after_integer);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_directly_after_fraction);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_after_fraction);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_directly_after_exponent);
  RUN_TEST_CASE(errors, test_parse_invalid_punctuator_after_exponent);
}
