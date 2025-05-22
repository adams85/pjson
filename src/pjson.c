/*
3-Clause BSD Non-AI License

Copyright (c) 2024 Adam Simon. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The source code, and any modifications made to it may not be used for the
   purpose of training or improving machine learning algorithms, including but
   not limited to artificial intelligence, natural language processing, or
   data mining. This condition applies to any derivatives, modifications, or
   updates based on the Software code. Any usage of the source code in an
   AI-training dataset is considered a breach of this License.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef PJSON_NO_LOCALE
#include <locale.h>
#endif

#include "pjson.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define UTF8_INVALID_CODEPOINT_REPLACEMENT (0xFFFD)

static inline uint8_t hex_digit_value(uint8_t ch);
static inline int16_t utf8_cont_payload(uint8_t ch);
static inline size_t utf8_byte_size(int32_t cp);
static inline bool utf16_is_high_surrogate(uint16_t ch);
static inline bool utf16_is_low_surrogate(uint16_t ch);
static inline int32_t utf16_to_code_point(uint16_t high_surrogate, uint16_t low_surrogate);

/* Tokenizer */

// The negative range of state values is reserved for storing final states
// (i.e. EOS and the error states defined by pjson_parsing_status).
// Note for maintainers: state values must not be changed as parsing logic relies on them (see comments below).

#define STATE_EOS (-1)
#define STATE_BETWEEN_TOKENS (0)
#define STATE_IN_KEYWORD (1)
#define STATE_IN_STRING (2) 
#define STATE_IN_STRING_EXPECT_ESCAPE (3)
#define STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_1_OF_4 (4)
#define STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_2_OF_4 (5) // value must be equal to STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_1_OF_4 + 1
#define STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_3_OF_4 (6) // value must be equal to STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_2_OF_4 + 1
#define STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_4_OF_4 (7) // value must be equal to STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_3_OF_4 + 1
#define STATE_IN_STRING_MAYBE_LOW_SURROGATE_ESCAPE (8)
#define STATE_IN_STRING_EXPECT_ESCAPE_MAYBE_LOW_SURROGATE (9)
#define STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_2 (10) // value is used for indexing into FEED_UTF8_SEQUENCE_BYTE_LOOKUP
#define STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3 (11) // value is used for indexing into FEED_UTF8_SEQUENCE_BYTE_LOOKUP
#define STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_3 (12) // value is used for indexing into FEED_UTF8_SEQUENCE_BYTE_LOOKUP and must be equal to STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3 + 1
#define STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_4 (13) // value is used for indexing into FEED_UTF8_SEQUENCE_BYTE_LOOKUP
#define STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_4 (14) // value is used for indexing into FEED_UTF8_SEQUENCE_BYTE_LOOKUP and must be equal to STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_4 + 1
#define STATE_IN_STRING_EXPECT_UTF8_BYTE_4_OF_4 (15) // value is used for indexing into FEED_UTF8_SEQUENCE_BYTE_LOOKUP and must be equal to STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_4 + 1
#define STATE_IN_NUMBER_EXPECT_INTEGER_PART (16)
#define STATE_IN_NUMBER_EXPECT_FRACTIONAL_PART (17)
#define STATE_IN_NUMBER_EXPECT_EXPONENT (18)
#define STATE_IN_NUMBER_EXPECT_EXPONENT_DIGITS (19)
#define STATE_IN_NUMBER_INTEGER_PART (20)
#define STATE_IN_NUMBER_FRACTIONAL_PART (21)
#define STATE_IN_NUMBER_EXPONENT_DIGITS (22)
#define STATE_IN_NUMBER_MAYBE_DECIMAL_SEPARATOR_OR_EXPONENT (23)

// Note for maintainers: lookup indices must be in sync with pjson_token_type values
// (i.e. the index of a keyword must be equal to `keyword_token_type - PJSON_TOKEN_NULL`)!

static const char *KEYWORD_LOOKUP[] = { "null", "false", "true" };

typedef bool (*feed_string_utf8_sequence_byte)(pjson_tokenizer *tokenizer, uint8_t ch);

static bool pjson_feed_string_utf8_intermediate_byte(pjson_tokenizer *tokenizer, uint8_t ch);
static bool pjson_feed_string_utf8_byte_2_of_2(pjson_tokenizer *tokenizer, uint8_t ch);
static bool pjson_feed_string_utf8_byte_3_of_3(pjson_tokenizer *tokenizer, uint8_t ch);
static bool pjson_feed_string_utf8_byte_4_of_4(pjson_tokenizer *tokenizer, uint8_t ch);

// Note for maintainers: lookup indices must be in sync with STATE_* values
// (i.e. the index of a function must be equal to `tokenizer->state - pjson_feed_string_utf8_sequence_byte_2_of_2`)!

static const feed_string_utf8_sequence_byte FEED_UTF8_SEQUENCE_BYTE_LOOKUP[] = {
  &pjson_feed_string_utf8_byte_2_of_2,
  &pjson_feed_string_utf8_intermediate_byte,
  &pjson_feed_string_utf8_byte_3_of_3,
  &pjson_feed_string_utf8_intermediate_byte,
  &pjson_feed_string_utf8_intermediate_byte,
  &pjson_feed_string_utf8_byte_4_of_4,
};

static pjson_parsing_status pjson_null_parser_eat_subsequent(pjson_parser_base *parser, const pjson_token *token) {
  (void)parser;
  return token->type != PJSON_TOKEN_EOS ? PJSON_STATUS_DATA_NEEDED : PJSON_STATUS_COMPLETED;
}

static pjson_parsing_status pjson_null_parser_eat_first(pjson_parser_base *parser, const pjson_token *token) {
  (void)parser;

  if (token->type == PJSON_TOKEN_EOS) return PJSON_STATUS_NO_TOKENS_FOUND;

  parser->eat = &pjson_null_parser_eat_subsequent;
  return PJSON_STATUS_DATA_NEEDED;
}

void pjson_init(pjson_tokenizer *tokenizer, pjson_parser_base *parser) {
  assert(tokenizer);

  memset(tokenizer, 0, sizeof(*tokenizer));

  tokenizer->token_start_index = (size_t)-1;

  if (!parser) {
    static pjson_parser_base null_parser = { .eat = &pjson_null_parser_eat_first };
    parser = &null_parser;
  }
  tokenizer->parser = (pjson_parser_base *)parser;

  tokenizer->buf = tokenizer->fixed_size_buf;
  tokenizer->buf_capacity = pjson_countof(tokenizer->fixed_size_buf);
}

static pjson_parsing_status pjson_report_error(pjson_tokenizer *tokenizer, pjson_parsing_status status, pjson_token_type type, size_t start_index) {
  assert(status < 0);
  tokenizer->token_type = type;
  tokenizer->token_start_index = start_index;
  tokenizer->token_start = NULL;
  return status;
}

static inline void pjson_start_token(pjson_tokenizer *tokenizer, pjson_token_type type, size_t start_index, const uint8_t *start) {
  tokenizer->token_type = type;
  tokenizer->token_start_index = start_index;
  tokenizer->token_start = start;
}

static const uint8_t *pjson_push_data_into_internal_buffer(pjson_tokenizer *tokenizer, const uint8_t *start, const uint8_t *end) {
  size_t count = end - start, new_length = tokenizer->buf_length + count;
  if (new_length < tokenizer->buf_length) return NULL; // handle unsigned overflow

  if (new_length > tokenizer->buf_capacity) {
    uint8_t *buf;
    size_t new_capacity = max(new_length, tokenizer->buf_capacity + (tokenizer->buf_capacity >> 1));
    for (;;) {
      if (tokenizer->buf == tokenizer->fixed_size_buf) {
        buf = pjson_malloc(new_capacity);
        if (buf) {
          memcpy(buf, tokenizer->fixed_size_buf, tokenizer->buf_length);
          break;
        }
      }
      else {
        buf = pjson_realloc(tokenizer->buf, new_capacity);
        if (buf) break;
      }

      if (new_capacity > new_length) new_capacity = new_length; // retry allocation with new_length
      else return NULL;
    }
    tokenizer->buf = buf;
    tokenizer->buf_capacity = new_capacity;
  }

  memcpy(tokenizer->buf + tokenizer->buf_length, start, count);
  tokenizer->buf_length = new_length;
  return tokenizer->buf + new_length;
}

static inline const uint8_t *pjson_ensure_token_data(pjson_tokenizer *tokenizer, const uint8_t *data, const uint8_t *end) {
  if (tokenizer->token_start != tokenizer->buf) {
    assert(data <= tokenizer->token_start && tokenizer->token_start < end);
    return end;
  }

  end = pjson_push_data_into_internal_buffer(tokenizer, data, end);
  tokenizer->token_start = tokenizer->buf; // this assignment is necessary in every case (buffer may have been reallocated!)
  return end;
}

static pjson_parsing_status pjson_finish_token(pjson_tokenizer *tokenizer, const uint8_t *data, const uint8_t *end) {
  end = pjson_ensure_token_data(tokenizer, data, end);
  if (!end) {
    return PJSON_STATUS_OUT_OF_MEMORY;
  }

  pjson_token token;
  token.type = tokenizer->token_type;
  token.start_index = tokenizer->token_start_index;
  token.start = tokenizer->token_start;
  token.length = end - token.start;
  token.unescaped_length = tokenizer->unescaped_length;
  assert(token.unescaped_length <= token.length);

  pjson_parser_base *parser = tokenizer->parser;
  pjson_parsing_status status = parser->eat(parser, &token);
  tokenizer->buf_length = 0;
  return status;
}

static pjson_parsing_status pjson_emit_punctuator(pjson_tokenizer *tokenizer, pjson_token_type type, const uint8_t *p) {
  pjson_token token;
  // All JSON punctuators are 1 byte long, so incomplete token handling can be skipped.
  token.type = type;
  token.start_index = tokenizer->index;
  token.start = p;
  token.length = token.unescaped_length = 1;

  pjson_parser_base *parser = tokenizer->parser;
  return parser->eat(parser, &token);
}

static pjson_parsing_status pjson_emit_eos(pjson_tokenizer *tokenizer) {
  pjson_token token;
  token.type = PJSON_TOKEN_EOS;
  token.start_index = tokenizer->index;
  token.start = NULL;
  token.length = token.unescaped_length = 0;

  pjson_parser_base *parser = tokenizer->parser;
  return parser->eat(parser, &token);
}

pjson_parsing_status pjson_feed(pjson_tokenizer *tokenizer, const uint8_t *data, size_t length) {
  assert(tokenizer);
  assert(data || length == 0);

  pjson_parsing_status status;
  size_t tmp;

  const uint8_t *p, *const data_end = data + length;
  assert((uintptr_t)data <= (uintptr_t)data_end); // check for unsigned overflow

  for (p = data; p < data_end; p++, tokenizer->index++) {
    uint8_t ch = *p, ch2;
    switch (tokenizer->state) {
      case STATE_BETWEEN_TOKENS: {
        switch (ch) {
          case '\x20': case '\t': case '\r': case '\n':
            continue;

          case '"':
            pjson_start_token(tokenizer, PJSON_TOKEN_STRING, tokenizer->index, p);
            tokenizer->unescaped_length = 0;
            tokenizer->state = STATE_IN_STRING;
            continue;

          case ':':
            tokenizer->token_type = PJSON_TOKEN_COLON;
            goto EmitPunctuator;

          case ',':
            tokenizer->token_type = PJSON_TOKEN_COMMA;
            goto EmitPunctuator;

          case '[':
            tokenizer->token_type = PJSON_TOKEN_OPEN_BRACKET;
            goto EmitPunctuator;

          case ']':
            tokenizer->token_type = PJSON_TOKEN_CLOSE_BRACKET;
            goto EmitPunctuator;

          case '{':
            tokenizer->token_type = PJSON_TOKEN_OPEN_BRACE;
            goto EmitPunctuator;

          case '}':
            tokenizer->token_type = PJSON_TOKEN_CLOSE_BRACE;
            goto EmitPunctuator;

          case '-':
            pjson_start_token(tokenizer, PJSON_TOKEN_NUMBER, tokenizer->index, p);
            tokenizer->state = STATE_IN_NUMBER_EXPECT_INTEGER_PART;
            continue;

          case '0':
            pjson_start_token(tokenizer, PJSON_TOKEN_NUMBER, tokenizer->index, p);
            tokenizer->state = STATE_IN_NUMBER_MAYBE_DECIMAL_SEPARATOR_OR_EXPONENT;
            continue;

          case '1': case '2': case '3': case '4': case '5': case '6': case '7':  case '8':  case '9':
            pjson_start_token(tokenizer, PJSON_TOKEN_NUMBER, tokenizer->index, p);
            tokenizer->state = STATE_IN_NUMBER_INTEGER_PART;
            continue;

          case 'f':
            pjson_start_token(tokenizer, PJSON_TOKEN_FALSE, tokenizer->index, p);
            tokenizer->state = STATE_IN_KEYWORD;
            continue;

          case 't':
            pjson_start_token(tokenizer, PJSON_TOKEN_TRUE, tokenizer->index, p);
            tokenizer->state = STATE_IN_KEYWORD;
            continue;

          case 'n':
            pjson_start_token(tokenizer, PJSON_TOKEN_NULL, tokenizer->index, p);
            tokenizer->state = STATE_IN_KEYWORD;
            continue;
        }

        goto UnexpectedCharacter;
      }

      /* Keywords */

      case STATE_IN_KEYWORD: {
        tmp = tokenizer->token_type - PJSON_TOKEN_NULL;
        assert(tmp < (pjson_countof(KEYWORD_LOOKUP)));
        const uint8_t *const keyword = (uint8_t *)KEYWORD_LOOKUP[tmp];

        tmp = tokenizer->index - tokenizer->token_start_index;
        assert(tmp <= (tokenizer->token_type == PJSON_TOKEN_FALSE ? 5 : 4));

        ch2 = keyword[tmp];
        if (ch == ch2) continue;
        else if (ch2 == 0) {
          switch (ch) {
            case '\x20': case '\t': case '\r': case '\n': goto FinishKeywordOrNumberAndHandleWhitespace;
            case ':': tmp = PJSON_TOKEN_COLON; goto FinishKeywordOrNumberAndHandlePunctuator;
            case ',': tmp = PJSON_TOKEN_COMMA; goto FinishKeywordOrNumberAndHandlePunctuator;
            case '[': tmp = PJSON_TOKEN_OPEN_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
            case ']': tmp = PJSON_TOKEN_CLOSE_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
            case '{': tmp = PJSON_TOKEN_OPEN_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
            case '}': tmp = PJSON_TOKEN_CLOSE_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
          }
        }

        goto InvalidToken;
      }

      /* String */

      case STATE_IN_STRING: {
      ExpectStringCharacter:
        if ((ch & 0x80) == 0) {
          if (ch == '"') {
            status = pjson_finish_token(tokenizer, data, p + 1);
            if (status != PJSON_STATUS_DATA_NEEDED) {
              if (status == PJSON_STATUS_COMPLETED) {
                p++, tokenizer->index++;
                goto Completed;
              }
              else goto UnexpectedTokenOrOtherError;
            }
            tokenizer->state = STATE_BETWEEN_TOKENS;
          }
          else if (ch == '\\') {
            tokenizer->state = STATE_IN_STRING_EXPECT_ESCAPE;
          }
          else if (ch >= 0x20) {
            tokenizer->unescaped_length++;
          }
          else goto InvalidToken;
        }
        else {
          if ((ch & 0xE0) == 0xC0) {
            tokenizer->state = STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_2;
          }
          else if ((ch & 0xF0) == 0xE0) {
            tokenizer->state = STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3;
          }
          else if ((ch & 0xF8) == 0xF0) {
            tokenizer->state = STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_4;
          }
          else goto Utf8Error;

          tokenizer->string_state.utf8_sequence_buf[0] = ch;
        }
        continue;
      }

      case STATE_IN_STRING_EXPECT_ESCAPE: {
      ExpectStringEscapeCharacter:
        switch (ch) {
          case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
            tokenizer->unescaped_length++;
            tokenizer->state = STATE_IN_STRING;
            continue;
          case 'u':
            tokenizer->state = STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_1_OF_4;
            continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_1_OF_4:
      case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_2_OF_4:
      case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_3_OF_4: {
        if (isxdigit(ch)) {
          tokenizer->string_state.utf16_surrogate_pair[0] = tokenizer->string_state.utf16_surrogate_pair[0] << 4 | hex_digit_value(ch);
          tokenizer->state++;
          continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_4_OF_4: {
        if (isxdigit(ch)) {
          tmp = tokenizer->string_state.utf16_surrogate_pair[0] << 4 | hex_digit_value(ch);
          if (utf16_is_high_surrogate((uint16_t)tmp)) {
            if (tokenizer->string_state.utf16_surrogate_pair[1] != 0) { // two consecutive high surrogates (invalid encoding but JSON allows it; will be replaced with U+FFFD)
              tokenizer->unescaped_length += utf8_byte_size(UTF8_INVALID_CODEPOINT_REPLACEMENT);
            }

            tokenizer->string_state.utf16_surrogate_pair[0] = 0;
            tokenizer->string_state.utf16_surrogate_pair[1] = (uint16_t)tmp;
            tokenizer->state = STATE_IN_STRING_MAYBE_LOW_SURROGATE_ESCAPE;
            continue;
          }
          else if (tokenizer->string_state.utf16_surrogate_pair[1] != 0) {
            if (utf16_is_low_surrogate((uint16_t)tmp)) { // low surrogate following a high surrogate
              tokenizer->unescaped_length += utf8_byte_size(utf16_to_code_point(tokenizer->string_state.utf16_surrogate_pair[1], (uint16_t)tmp));
            }
            else { // lone high surrogate
              tokenizer->unescaped_length += utf8_byte_size(UTF8_INVALID_CODEPOINT_REPLACEMENT);
              tokenizer->unescaped_length += utf8_byte_size((int32_t)tmp);
            }
          }
          else {
            tokenizer->unescaped_length += utf8_byte_size(!utf16_is_low_surrogate((uint16_t)tmp)
              ? (int32_t)tmp
              : UTF8_INVALID_CODEPOINT_REPLACEMENT); // lone low surrogate
          }

          tokenizer->string_state.utf16_surrogate_pair[0] = tokenizer->string_state.utf16_surrogate_pair[1] = 0;
          tokenizer->state = STATE_IN_STRING;
          continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_STRING_MAYBE_LOW_SURROGATE_ESCAPE: {
        if (ch == '\\') {
          tokenizer->state = STATE_IN_STRING_EXPECT_ESCAPE_MAYBE_LOW_SURROGATE;
          continue;
        }

        tokenizer->unescaped_length += utf8_byte_size(UTF8_INVALID_CODEPOINT_REPLACEMENT); // lone high surrogate
        tokenizer->string_state.utf16_surrogate_pair[0] = tokenizer->string_state.utf16_surrogate_pair[1] = 0;
        tokenizer->state = STATE_IN_STRING;
        goto ExpectStringCharacter;
      }

      case STATE_IN_STRING_EXPECT_ESCAPE_MAYBE_LOW_SURROGATE: {
        if (ch == 'u') {
          tokenizer->state = STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_1_OF_4;
          continue;
        }

        tokenizer->unescaped_length += utf8_byte_size(UTF8_INVALID_CODEPOINT_REPLACEMENT); // lone high surrogate
        tokenizer->string_state.utf16_surrogate_pair[0] = tokenizer->string_state.utf16_surrogate_pair[1] = 0;
        tokenizer->state = STATE_IN_STRING_EXPECT_ESCAPE;
        goto ExpectStringEscapeCharacter;
      }

      case STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_2:
      case STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3:
      case STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_3:
      case STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_4:
      case STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_4:
      case STATE_IN_STRING_EXPECT_UTF8_BYTE_4_OF_4: {
        tmp = tokenizer->state - STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_2;
        assert(0 < pjson_countof(FEED_UTF8_SEQUENCE_BYTE_LOOKUP));
        if (!FEED_UTF8_SEQUENCE_BYTE_LOOKUP[tmp](tokenizer, ch)) goto Utf8Error;
        continue;
      }

      /* Number */

      case STATE_IN_NUMBER_EXPECT_INTEGER_PART: {
        if (ch == '0') {
          tokenizer->state = STATE_IN_NUMBER_MAYBE_DECIMAL_SEPARATOR_OR_EXPONENT;
          continue;
        }

        if (isdigit(ch)) {
          tokenizer->state = STATE_IN_NUMBER_INTEGER_PART;
          continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_INTEGER_PART: {
        if (isdigit(ch)) {
          continue;
        }

      MaybeDecimalSeparatorOrExponent:
        switch (ch) {
          case '.':
            tokenizer->state = STATE_IN_NUMBER_EXPECT_FRACTIONAL_PART;
            continue;

          case 'e': case 'E':
            tokenizer->state = STATE_IN_NUMBER_EXPECT_EXPONENT;
            continue;

          case '\x20': case '\t': case '\r': case '\n': goto FinishKeywordOrNumberAndHandleWhitespace;
          case ':': tmp = PJSON_TOKEN_COLON; goto FinishKeywordOrNumberAndHandlePunctuator;
          case ',': tmp = PJSON_TOKEN_COMMA; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '[': tmp = PJSON_TOKEN_OPEN_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
          case ']': tmp = PJSON_TOKEN_CLOSE_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '{': tmp = PJSON_TOKEN_OPEN_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '}': tmp = PJSON_TOKEN_CLOSE_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_EXPECT_FRACTIONAL_PART: {
        if (isdigit(ch)) {
          tokenizer->state = STATE_IN_NUMBER_FRACTIONAL_PART;
          continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_FRACTIONAL_PART: {
        if (isdigit(ch)) {
          continue;
        }

        switch (ch) {
          case 'e': case 'E':
            tokenizer->state = STATE_IN_NUMBER_EXPECT_EXPONENT;
            continue;

          case '\x20': case '\t': case '\r': case '\n': goto FinishKeywordOrNumberAndHandleWhitespace;
          case ':': tmp = PJSON_TOKEN_COLON; goto FinishKeywordOrNumberAndHandlePunctuator;
          case ',': tmp = PJSON_TOKEN_COMMA; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '[': tmp = PJSON_TOKEN_OPEN_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
          case ']': tmp = PJSON_TOKEN_CLOSE_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '{': tmp = PJSON_TOKEN_OPEN_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '}': tmp = PJSON_TOKEN_CLOSE_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_EXPECT_EXPONENT: {
        if (ch == '+' || ch == '-') {
          tokenizer->state = STATE_IN_NUMBER_EXPECT_EXPONENT_DIGITS;
          continue;
        }

        if (isdigit(ch)) {
          tokenizer->state = STATE_IN_NUMBER_EXPONENT_DIGITS;
          continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_EXPECT_EXPONENT_DIGITS: {
        if (isdigit(ch)) {
          tokenizer->state = STATE_IN_NUMBER_EXPONENT_DIGITS;
          continue;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_EXPONENT_DIGITS: {
        if (isdigit(ch)) {
          tokenizer->state = STATE_IN_NUMBER_EXPONENT_DIGITS;
          continue;
        }

        switch (ch) {
          case '\x20': case '\t': case '\r': case '\n': goto FinishKeywordOrNumberAndHandleWhitespace;
          case ':': tmp = PJSON_TOKEN_COLON; goto FinishKeywordOrNumberAndHandlePunctuator;
          case ',': tmp = PJSON_TOKEN_COMMA; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '[': tmp = PJSON_TOKEN_OPEN_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
          case ']': tmp = PJSON_TOKEN_CLOSE_BRACKET; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '{': tmp = PJSON_TOKEN_OPEN_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
          case '}': tmp = PJSON_TOKEN_CLOSE_BRACE; goto FinishKeywordOrNumberAndHandlePunctuator;
        }

        goto InvalidToken;
      }

      case STATE_IN_NUMBER_MAYBE_DECIMAL_SEPARATOR_OR_EXPONENT: goto MaybeDecimalSeparatorOrExponent;

      default:
        assert(tokenizer->state < 0);
        return tokenizer->state;
    }

    /* Common logic */

  FinishKeywordOrNumberAndHandleWhitespace:
    {
      tokenizer->unescaped_length = tokenizer->index - tokenizer->token_start_index;
      status = pjson_finish_token(tokenizer, data, p);
      if (status != PJSON_STATUS_DATA_NEEDED) {
        if (status == PJSON_STATUS_COMPLETED) goto Completed;
        else goto UnexpectedTokenOrOtherError;
      }

      tokenizer->state = STATE_BETWEEN_TOKENS;
      continue;
    }

  FinishKeywordOrNumberAndHandlePunctuator:
    {
      tokenizer->unescaped_length = tokenizer->index - tokenizer->token_start_index;
      status = pjson_finish_token(tokenizer, data, p);
      if (status != PJSON_STATUS_DATA_NEEDED) {
        if (status == PJSON_STATUS_COMPLETED) goto Completed;
        else goto UnexpectedTokenOrOtherError;
      }
      tokenizer->token_type = (pjson_token_type)tmp;

    EmitPunctuator:
      status = pjson_emit_punctuator(tokenizer, tokenizer->token_type, p);
      if (status != PJSON_STATUS_DATA_NEEDED) {
        if (status == PJSON_STATUS_COMPLETED) {
          p++, tokenizer->index++;
          goto Completed;
        }
        tokenizer->token_start_index = tokenizer->index;
        goto UnexpectedTokenOrOtherError;
      }

      tokenizer->state = STATE_BETWEEN_TOKENS;
      continue;
    }

  Completed:
    {
      tokenizer->token_type = PJSON_TOKEN_NONE;
      tokenizer->token_start_index = tokenizer->index;
      tokenizer->token_start = p; // save the pointer to the start of the potential next JSON value for user
      tokenizer->state = STATE_BETWEEN_TOKENS;
      return PJSON_STATUS_COMPLETED;
    }
  }

  /* Buffer consumed */

  if (tokenizer->state != STATE_BETWEEN_TOKENS) {
    if (tokenizer->state < 0) {
      return tokenizer->state;
    }

    // Token may be incomplete. Save what is received so far into the internal buffer.
    p = pjson_ensure_token_data(tokenizer, data, data_end);
    if (p == data_end) {
      if (!pjson_push_data_into_internal_buffer(tokenizer, tokenizer->token_start, data_end)) goto OutOfMemory;
      tokenizer->token_start = tokenizer->buf;
    }
    else {
      if (!p) goto OutOfMemory;
    }
  }
  else {
    tokenizer->token_type = PJSON_TOKEN_NONE;
    tokenizer->token_start_index = tokenizer->index;
    tokenizer->token_start = NULL;
  }

  return PJSON_STATUS_DATA_NEEDED;

  /* Error cases */

Utf8Error:
  status = pjson_report_error(tokenizer, PJSON_STATUS_UTF8_ERROR, PJSON_TOKEN_ERROR, tokenizer->index);
  return tokenizer->state = status; // save the status code for cases when pjson_feed is called again

UnexpectedCharacter:
  status = pjson_report_error(tokenizer, PJSON_STATUS_SYNTAX_ERROR, PJSON_TOKEN_ERROR, tokenizer->index);
  return tokenizer->state = status; // save the status code for cases when pjson_feed is called again

InvalidToken:
  status = pjson_report_error(tokenizer, PJSON_STATUS_SYNTAX_ERROR, PJSON_TOKEN_ERROR, tokenizer->token_start_index);
  return tokenizer->state = status; // save the status code for cases when pjson_feed is called again

OutOfMemory:
  status = PJSON_STATUS_OUT_OF_MEMORY;
  goto UnexpectedTokenOrOtherError;

UnexpectedTokenOrOtherError:
  status = pjson_report_error(tokenizer, status > 0 ? PJSON_STATUS_NONCOMPLIANT_PARSER : status, tokenizer->token_type, tokenizer->token_start_index);
  return tokenizer->state = status; // save the status code for cases when pjson_feed is called again
}

pjson_parsing_status pjson_close(pjson_tokenizer *tokenizer) {
  assert(tokenizer);

  pjson_parsing_status status;
  size_t index;

  switch (tokenizer->state) {
    case STATE_BETWEEN_TOKENS:
      goto EmitEOS;

    case STATE_IN_KEYWORD: {
      index = tokenizer->token_type - PJSON_TOKEN_NULL;
      assert(index < (pjson_countof(KEYWORD_LOOKUP)));
      const uint8_t *const keyword = (uint8_t *)KEYWORD_LOOKUP[index];

      index = tokenizer->index - tokenizer->token_start_index;
      assert(index <= (tokenizer->token_type == PJSON_TOKEN_FALSE ? 5 : 4));

      if (keyword[index] != 0) goto InvalidToken;

      status = pjson_finish_token(tokenizer, NULL, NULL);
      if (status != PJSON_STATUS_DATA_NEEDED && status != PJSON_STATUS_COMPLETED) goto UnexpectedTokenOrOtherError;
      goto EmitEOS;
    }

    case STATE_IN_STRING:
    case STATE_IN_STRING_EXPECT_ESCAPE:
    case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_1_OF_4:
    case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_2_OF_4:
    case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_3_OF_4:
    case STATE_IN_STRING_EXPECT_UTF16_ESCAPE_DIGIT_4_OF_4:
    case STATE_IN_STRING_MAYBE_LOW_SURROGATE_ESCAPE:
    case STATE_IN_STRING_EXPECT_ESCAPE_MAYBE_LOW_SURROGATE:
    case STATE_IN_NUMBER_EXPECT_INTEGER_PART:
    case STATE_IN_NUMBER_EXPECT_FRACTIONAL_PART:
    case STATE_IN_NUMBER_EXPECT_EXPONENT:
    case STATE_IN_NUMBER_EXPECT_EXPONENT_DIGITS:
      goto InvalidToken;

    case STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_2:
    case STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3:
    case STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_3:
    case STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_4:
    case STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_4:
    case STATE_IN_STRING_EXPECT_UTF8_BYTE_4_OF_4: {
      tokenizer->index -=
        tokenizer->state == STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_2
        || tokenizer->state == STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3
        || tokenizer->state == STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_4
        ? 1
        : tokenizer->state == STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_3
        || tokenizer->state == STATE_IN_STRING_EXPECT_UTF8_BYTE_3_OF_4
        ? 2
        : 3;
      goto Utf8Error;
    }

    case STATE_IN_NUMBER_INTEGER_PART:
    case STATE_IN_NUMBER_FRACTIONAL_PART:
    case STATE_IN_NUMBER_EXPONENT_DIGITS:
    case STATE_IN_NUMBER_MAYBE_DECIMAL_SEPARATOR_OR_EXPONENT:
      status = pjson_finish_token(tokenizer, NULL, NULL);
      if (status != PJSON_STATUS_DATA_NEEDED && status != PJSON_STATUS_COMPLETED) goto UnexpectedTokenOrOtherError;
      goto EmitEOS;

    default:
      assert(tokenizer->state < 0);
      status = tokenizer->state;
      goto CleanUp;
  }

Utf8Error:
  status = pjson_report_error(tokenizer, PJSON_STATUS_UTF8_ERROR, PJSON_TOKEN_ERROR, tokenizer->index);
  tokenizer->state = status; // save the status code for cases when pjson_feed is called again
  goto CleanUp;

InvalidToken:
  status = pjson_report_error(tokenizer, PJSON_STATUS_SYNTAX_ERROR, PJSON_TOKEN_ERROR, tokenizer->token_start_index);
  tokenizer->state = status; // save the status code for cases when pjson_feed is called again
  goto CleanUp;

UnexpectedTokenOrOtherError:
  status = pjson_report_error(tokenizer, status > 0 ? PJSON_STATUS_NONCOMPLIANT_PARSER : status, tokenizer->token_type, tokenizer->token_start_index);
  tokenizer->state = status; // save the status code for cases when pjson_feed/pjson_close is called again
  goto CleanUp;

EmitEOS:
  tokenizer->token_type = PJSON_TOKEN_EOS;
  status = pjson_emit_eos(tokenizer);
  if (status != PJSON_STATUS_COMPLETED) goto UnexpectedTokenOrOtherError;

  tokenizer->token_start_index = tokenizer->index;
  tokenizer->token_start = NULL;
  tokenizer->state = STATE_EOS;

CleanUp:
  if (tokenizer->buf) {
    if (tokenizer->buf != tokenizer->fixed_size_buf) {
      pjson_free(tokenizer->buf);
    }
    tokenizer->buf = NULL;
    tokenizer->buf_length = 0;
    tokenizer->buf_capacity = 0;
  }

  return status;
}

// UTF8 sequence validation is based on: https://www.json.org/JSON_checker/utf8_decode.c

static bool pjson_feed_string_utf8_intermediate_byte(pjson_tokenizer *tokenizer, uint8_t ch) {
  uint8_t *p = tokenizer->string_state.utf8_sequence_buf + ((tokenizer->state - STATE_IN_STRING_EXPECT_UTF8_BYTE_2_OF_3) & 1) + 1;
  *p = ch;
  tokenizer->state++;
  return true;
}

static bool pjson_feed_string_utf8_byte_2_of_2(pjson_tokenizer *tokenizer, uint8_t ch) {
  uint8_t *p = tokenizer->string_state.utf8_sequence_buf;
  const uint8_t ch0 = *p;
  const int16_t ch1 = utf8_cont_payload(ch);

  if (ch1 >= 0) {
    const uint32_t r = ((ch0 & 0x1F) << 6) | (uint8_t)ch1;
    if (r >= 0x80) {
      tokenizer->unescaped_length += 2;
      tokenizer->state = STATE_IN_STRING;
      return true;
    }
  }

  tokenizer->index--;
  return false;
}

static bool pjson_feed_string_utf8_byte_3_of_3(pjson_tokenizer *tokenizer, uint8_t ch) {
  uint8_t *p = tokenizer->string_state.utf8_sequence_buf;
  const uint8_t ch0 = *p;
  const int16_t ch1 = utf8_cont_payload(*(++p));
  const int16_t ch2 = utf8_cont_payload(ch);

  if ((ch1 | ch2) >= 0) {
    const uint32_t r = ((ch0 & 0x0F) << 12) | ((uint8_t)ch1 << 6) | (uint8_t)ch2;
    if (r >= 0x800 && (r < 0xD800 || r > 0xDFFF)) {
      tokenizer->unescaped_length += 3;
      tokenizer->state = STATE_IN_STRING;
      return true;
    }
  }

  tokenizer->index -= 2;
  return false;
}

static bool pjson_feed_string_utf8_byte_4_of_4(pjson_tokenizer *tokenizer, uint8_t ch) {
  uint8_t *p = tokenizer->string_state.utf8_sequence_buf;
  const uint8_t ch0 = *p;
  const int16_t ch1 = utf8_cont_payload(*(++p));
  const int16_t ch2 = utf8_cont_payload(*(++p));
  const int16_t ch3 = utf8_cont_payload(ch);

  if ((ch1 | ch2 | ch3) >= 0) {
    const uint32_t r = ((ch0 & 0x07) << 18) | ((uint8_t)ch1 << 12) | ((uint8_t)ch2 << 6) | (uint8_t)ch3;
    if (r >= 0x10000 && r <= 0x10FFFF) {
      tokenizer->unescaped_length += 4;
      tokenizer->state = STATE_IN_STRING;
      return true;
    }
  }

  tokenizer->index -= 3;
  return false;
}

/* Parser */

static pjson_parsing_status pjson_eat_array_element_or_end(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_array_element(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_array_element_separator_or_end(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_object_property_name_or_end(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_object_property_name(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_object_property_name_and_value_separator(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_object_property_value(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_object_property_separator_or_end(pjson_parser *parser, const pjson_token *token);
static pjson_parsing_status pjson_eat_eos(pjson_parser *parser, const pjson_token *token);

static inline pjson_parsing_status pjson_eat_value(pjson_parser *parser, const pjson_token *token,
  pjson_parser_eat primitive_value_next_eat,
  pjson_parser_eat complex_value_next_eat,
  pjson_parsing_status primitive_value_status,
  pjson_parsing_status eos_status) {

  pjson_parser_context *context;
  pjson_parsing_status status;
  pjson_parser_eat parser_next_eat;

  switch (token->type) {
    case PJSON_TOKEN_NULL:
    case PJSON_TOKEN_FALSE:
    case PJSON_TOKEN_TRUE:
    case PJSON_TOKEN_NUMBER:
    case PJSON_TOKEN_STRING: {
      context = (pjson_parser_context *)parser->peek_context(parser, false);
      assert(context);
      if (context->on_value
        && (status = context->on_value(parser, context, token)) != PJSON_STATUS_SUCCESS) {
        goto Error;
      }

      parser->base.eat = primitive_value_next_eat;
      return primitive_value_status;
    }

    case PJSON_TOKEN_OPEN_BRACKET:
      parser_next_eat = (pjson_parser_eat)&pjson_eat_array_element_or_end;
      goto BeginComplexValue;

    case PJSON_TOKEN_OPEN_BRACE:
      parser_next_eat = (pjson_parser_eat)&pjson_eat_object_property_name_or_end;
      goto BeginComplexValue;

    case PJSON_TOKEN_EOS:
      return eos_status;

    default:
      return PJSON_STATUS_SYNTAX_ERROR;
  }

BeginComplexValue:
  status = parser->push_context(parser);
  if (status != PJSON_STATUS_SUCCESS) goto Error;

  pjson_parser_context *new_context = (pjson_parser_context *)parser->peek_context(parser, false);
  memset(new_context, 0, sizeof(*new_context));

  context = (pjson_parser_context *)parser->peek_context(parser, true);
  assert(context);
  context->next_eat = complex_value_next_eat;

  if (context->on_value
    && (status = context->on_value(parser, context, token)) != PJSON_STATUS_SUCCESS) {
    goto Error;
  }

  parser->base.eat = parser_next_eat;
  return PJSON_STATUS_DATA_NEEDED;

Error:
  return status > 0 ? PJSON_STATUS_NONCOMPLIANT_PARSER : status;
}

static inline pjson_parsing_status pjson_end_complex_value(pjson_parser *parser, const pjson_token *token) {
  pjson_parser_context *context = (pjson_parser_context *)parser->peek_context(parser, true);
  assert(context);

  pjson_parsing_status status;
  if (context->on_value
    && (status = context->on_value(parser, context, token)) != PJSON_STATUS_SUCCESS) {
    return status > 0 ? PJSON_STATUS_NONCOMPLIANT_PARSER : status;
  }

  pjson_parser_eat next_eat = context->next_eat;
  context->next_eat = NULL;

  parser->pop_context(parser);

  if (next_eat) {
    parser->base.eat = next_eat;
    return PJSON_STATUS_DATA_NEEDED;
  }
  else {
    parser->base.eat = (pjson_parser_eat)&pjson_eat_eos;
    return PJSON_STATUS_COMPLETED;
  }
}

static pjson_parsing_status pjson_eat_toplevel_value_greedy(pjson_parser *parser, const pjson_token *token) {
  return pjson_eat_value(parser, token,
    (pjson_parser_eat)&pjson_eat_eos,
    (pjson_parser_eat)&pjson_eat_eos,
    PJSON_STATUS_DATA_NEEDED,
    PJSON_STATUS_NO_TOKENS_FOUND);
}

static pjson_parsing_status pjson_eat_toplevel_value_lazy(pjson_parser *parser, const pjson_token *token) {
  return pjson_eat_value(parser, token,
    (pjson_parser_eat)&pjson_eat_eos,
    NULL, // used to indicate lazy parsing to `pjson_end_complex_value`
    PJSON_STATUS_COMPLETED,
    PJSON_STATUS_NO_TOKENS_FOUND);
}

static pjson_parsing_status pjson_eat_array_element_or_end(pjson_parser *parser, const pjson_token *token) {
  return token->type != PJSON_TOKEN_CLOSE_BRACKET
    ? pjson_eat_array_element(parser, token)
    : pjson_end_complex_value(parser, token);
}

static pjson_parsing_status pjson_eat_array_element(pjson_parser *parser, const pjson_token *token) {
  return pjson_eat_value(parser, token,
    (pjson_parser_eat)&pjson_eat_array_element_separator_or_end,
    (pjson_parser_eat)&pjson_eat_array_element_separator_or_end,
    PJSON_STATUS_DATA_NEEDED,
    PJSON_STATUS_SYNTAX_ERROR);
}

static pjson_parsing_status pjson_eat_array_element_separator_or_end(pjson_parser *parser, const pjson_token *token) {
  switch (token->type) {
    case PJSON_TOKEN_COMMA: {
      parser->base.eat = (pjson_parser_eat)&pjson_eat_array_element;
      return PJSON_STATUS_DATA_NEEDED;
    }

    case PJSON_TOKEN_CLOSE_BRACKET:
      return pjson_end_complex_value(parser, token);

    default:
      return PJSON_STATUS_SYNTAX_ERROR;
  }
}

static pjson_parsing_status pjson_eat_object_property_name_or_end(pjson_parser *parser, const pjson_token *token) {
  return token->type != PJSON_TOKEN_CLOSE_BRACE
    ? pjson_eat_object_property_name(parser, token)
    : pjson_end_complex_value(parser, token);
}

static pjson_parsing_status pjson_eat_object_property_name(pjson_parser *parser, const pjson_token *token) {
  if (token->type == PJSON_TOKEN_STRING) {
    pjson_parser_context *current_context = (pjson_parser_context *)parser->peek_context(parser, false);
    assert(current_context);

    pjson_parsing_status status;
    if (current_context->on_object_property_name
      && (status = current_context->on_object_property_name(parser, current_context, token)) != PJSON_STATUS_SUCCESS) {
      return status > 0 ? PJSON_STATUS_NONCOMPLIANT_PARSER : status;
    }

    parser->base.eat = (pjson_parser_eat)&pjson_eat_object_property_name_and_value_separator;
    return PJSON_STATUS_DATA_NEEDED;
  }

  return PJSON_STATUS_SYNTAX_ERROR;
}

static pjson_parsing_status pjson_eat_object_property_name_and_value_separator(pjson_parser *parser, const pjson_token *token) {
  if (token->type == PJSON_TOKEN_COLON) {
    parser->base.eat = (pjson_parser_eat)&pjson_eat_object_property_value;
    return PJSON_STATUS_DATA_NEEDED;
  }

  return PJSON_STATUS_SYNTAX_ERROR;
}

static pjson_parsing_status pjson_eat_object_property_value(pjson_parser *parser, const pjson_token *token) {
  return pjson_eat_value(parser, token,
    (pjson_parser_eat)&pjson_eat_object_property_separator_or_end,
    (pjson_parser_eat)&pjson_eat_object_property_separator_or_end,
    PJSON_STATUS_DATA_NEEDED,
    PJSON_STATUS_SYNTAX_ERROR);
}

static pjson_parsing_status pjson_eat_object_property_separator_or_end(pjson_parser *parser, const pjson_token *token) {
  switch (token->type) {
    case PJSON_TOKEN_COMMA: {
      parser->base.eat = (pjson_parser_eat)&pjson_eat_object_property_name;
      return PJSON_STATUS_DATA_NEEDED;
    }

    case PJSON_TOKEN_CLOSE_BRACE:
      return pjson_end_complex_value(parser, token);

    default:
      return PJSON_STATUS_SYNTAX_ERROR;
  }
}

static pjson_parsing_status pjson_eat_eos(pjson_parser *parser, const pjson_token *token) {
  (void)parser;
  return token->type == PJSON_TOKEN_EOS ? PJSON_STATUS_COMPLETED : PJSON_STATUS_SYNTAX_ERROR;
}

void pjson_parser_init(pjson_parser *parser, bool is_lazy,
  pjson_parser_push_context push_context,
  pjson_parser_peek_context peek_context,
  pjson_parser_pop_context pop_context
) {
  assert(parser);
  assert(push_context);
  assert(peek_context);
  assert(pop_context);

  memset(parser, 0, sizeof(*parser));

  parser->push_context = push_context;
  parser->peek_context = peek_context;
  parser->pop_context = pop_context;

  pjson_parser_reset(parser, is_lazy);
}

void pjson_parser_reset(pjson_parser *parser, bool is_lazy) {
  assert(parser);

  parser->push_context(parser);
  pjson_parser_context *context = parser->peek_context(parser, false);
  assert(context);
  memset(context, 0, sizeof(*context));

  parser->base.eat = (pjson_parser_eat)(!is_lazy
    ? &pjson_eat_toplevel_value_greedy
    : &pjson_eat_toplevel_value_lazy);
}

/* Helpers */

static inline uint8_t hex_digit_value(uint8_t ch) {
  return (ch <= '9') ? ch - '0'
    : (ch <= 'F') ? ch - ('A' - 10)
    : ch - ('a' - 10);
}

static inline bool is_in_range(uint16_t cp, uint16_t min, uint16_t max) {
  assert(min <= max);
  return (uint32_t)(cp - (uint32_t)min) <= (uint32_t)(max - (uint32_t)min);
}

#define UTF8_ERROR (-1)

/**
 * Get the 6-bit payload of the next continuation byte.
 * Return UTF8_ERROR if it is not a contination byte.
 */
static inline int16_t utf8_cont_payload(uint8_t ch) {
  return ((ch & 0xC0) == 0x80) ? (ch & 0x3F) : UTF8_ERROR;
}

static inline size_t utf8_byte_size(int32_t cp) {
  return cp < 0x80 ? 1
    : cp < 0x800 ? 2
    : cp < 0x10000 ? 3
    : 4;
}

static bool utf8_encode_code_point(int32_t cp, uint8_t **dest, const uint8_t *dest_end) {
  assert(0 <= cp && cp <= 0x10FFFF);
  assert(dest);
  assert(*dest);
  assert(*dest < dest_end);

  if (cp < 0x80) {
    **dest = (uint8_t)cp;
  }
  else if (cp < 0x800) {
    if (*dest + 1 >= dest_end) return false;

    **dest = (uint8_t)(0xC0 | (cp >> 6));
    *(++*dest) = (uint8_t)(0x80 | (cp & 0x3F));
  }
  else if (cp <= 0xFFFF) {
    if (*dest + 2 >= dest_end) return false;

    if (0xD800 <= cp && cp <= 0xDFFF) {
      // Replace lone surrogates with U+FFFD
      cp = UTF8_INVALID_CODEPOINT_REPLACEMENT;
    }

    **dest = (uint8_t)(0xE0 | (cp >> 12));
    *(++*dest) = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
    *(++*dest) = (uint8_t)(0x80 | (cp & 0x3F));
  }
  else {
    if (*dest + 3 >= dest_end) return false;

    **dest = (uint8_t)(0xF0 | (cp >> 18));
    *(++*dest) = (uint8_t)(0x80 | ((cp >> 12) & 0x3F));
    *(++*dest) = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
    *(++*dest) = (uint8_t)(0x80 | (cp & 0x3F));
  }

  return true;
}

static inline bool utf16_is_high_surrogate(uint16_t ch) {
  return is_in_range(ch, 0xD800, 0xDBFF);
}

static inline bool utf16_is_low_surrogate(uint16_t ch) {
  return is_in_range(ch, 0xDC00, 0xDFFF);
}

static inline int32_t utf16_to_code_point(uint16_t high_surrogate, uint16_t low_surrogate) {
  return ((int32_t)high_surrogate << 10) + (int32_t)low_surrogate - 0x35FDC00;
}

static int32_t utf16_parse_char(const uint8_t *src) {
  assert(src);

  int32_t cp = 0;
  for (size_t i = 0; i < 4; i++) {
    uint8_t ch = src[i];
    if (!isxdigit(ch)) return -1;
    cp = cp << 4 | hex_digit_value(ch);
  }
  return cp;
}

bool pjson_parse_string(uint8_t *dest, size_t dest_size, const uint8_t *token_start, size_t token_length, bool replace_lone_surrogates) {
  assert(dest);
  assert(token_start);

  const uint8_t *token_end = token_start + token_length;
  assert((uintptr_t)token_start <= (uintptr_t)token_end); // check for unsigned overflow

  uint8_t *dest_end = dest + dest_size;
  assert((uintptr_t)dest <= (uintptr_t)dest_end); // check for unsigned overflow

  if (token_length < 2 || *token_start != '"' || *(token_end - 1) != '"') {
    return false;
  }

  for (token_start++, token_end--; token_start < token_end; token_start++, dest++) {
    if (dest >= dest_end) return false;

    int32_t cp = *token_start;

    if (cp == '\\') {
      if (++token_start >= token_end) return false;

      switch (*token_start) {
        case '"': cp = '"'; break;
        case '\\': cp = '\\'; break;
        case '/': cp = '/'; break;
        case 'b': cp = '\b'; break;
        case 'f': cp = '\f'; break;
        case 'n': cp = '\n'; break;
        case 'r': cp = '\r'; break;
        case 't': cp = '\t'; break;
        case 'u': {
          if (token_start + 4 >= token_end) return false;

          cp = utf16_parse_char(token_start + 1);
          if (cp < 0) return false;
          token_start += 4;

          if (utf16_is_high_surrogate((uint16_t)cp)) {
            if (token_start + 6 < token_end
              && *(token_start + 1) == '\\' && *(token_start + 2) == 'u') {
                int32_t cp2 = utf16_parse_char(token_start + 3);
                if (cp2 < 0) return false;

                if (utf16_is_low_surrogate((uint16_t)cp2)) {
                  token_start += 6;
                  cp = utf16_to_code_point((uint16_t)cp, (uint16_t)cp2);
                }
                else if (!replace_lone_surrogates) return false;
            }
            else if (!replace_lone_surrogates) return false;
          }
          else if (!replace_lone_surrogates && utf16_is_low_surrogate((uint16_t)cp)) {
            return false;
          }

          if (!utf8_encode_code_point(cp, &dest, dest_end)) return false;
          continue;
        }
      }
    }

    *dest = (uint8_t)cp;
  }

  return true;
}

static bool pjson_parse_uint32_core(uint32_t *num, const uint8_t *token_start, const uint8_t *token_end) {
  uint32_t value = 0;
  for (; token_start < token_end; token_start++) {
    uint8_t ch = *token_start;
    if (!isdigit(ch)) return false;

    ch = ch - '0';
    if (value <= UINT32_MAX / 10 - 1) {
      value = value * 10 + ch; // operation is safe, will not overflow.
    }
    else if (value <= UINT32_MAX / 10) {
      uint32_t tmp = value;
      value = value * 10 + ch; // operation may overflow
      if (value < tmp) return false;
    }
    else return false; // operation would guaranteedly overflow
  }

  *num = value;
  return true;
}

bool pjson_parse_uint32(uint32_t *num, const uint8_t *token_start, size_t token_length) {
  assert(num);
  assert(token_start);

  const uint8_t *token_end = token_start + token_length;
  assert((uintptr_t)token_start <= (uintptr_t)token_end); // check for unsigned overflow

  if (token_length == 0) return false;

  return pjson_parse_uint32_core(num, token_start, token_end);
}

bool pjson_parse_int32(int32_t *num, const uint8_t *token_start, size_t token_length) {
  assert(num);
  assert(token_start);

  const uint8_t *token_end = token_start + token_length;
  assert((uintptr_t)token_start <= (uintptr_t)token_end); // check for unsigned overflow

  if (token_length == 0) return false;

  bool is_negative = *token_start == '-';

  uint32_t tmp;
  if (!pjson_parse_uint32_core(&tmp, token_start + (uintptr_t)is_negative, token_end)) {
    return false;
  }

  if (tmp <= (uint32_t)INT32_MAX) *num = !is_negative ? (int32_t)tmp : -(int32_t)tmp;
  else if (is_negative && tmp == ((uint32_t)(INT32_MAX)) + 1) *num = INT32_MIN;
  else return false;

  return true;
}

static bool pjson_parse_uint64_core(uint64_t *num, const uint8_t *token_start, const uint8_t *token_end) {
  uint64_t value = 0;
  for (; token_start < token_end; token_start++) {
    uint8_t ch = *token_start;
    if (!isdigit(ch)) return false;

    ch = ch - '0';
    if (value <= UINT64_MAX / 10 - 1) {
      value = value * 10 + ch; // operation is safe, will not overflow.
    }
    else if (value <= UINT64_MAX / 10) {
      uint64_t tmp = value;
      value = value * 10 + ch; // operation may overflow
      if (value < tmp) return false;
    }
    else return false; // operation would guaranteedly overflow
  }

  *num = value;
  return true;
}

bool pjson_parse_uint64(uint64_t *num, const uint8_t *token_start, size_t token_length) {
  assert(num);
  assert(token_start);

  const uint8_t *token_end = token_start + token_length;
  assert((uintptr_t)token_start <= (uintptr_t)token_end); // check for unsigned overflow

  if (token_length == 0) return false;

  return pjson_parse_uint64_core(num, token_start, token_end);
}

bool pjson_parse_int64(int64_t *num, const uint8_t *token_start, size_t token_length) {
  assert(num);
  assert(token_start);

  const uint8_t *token_end = token_start + token_length;
  assert((uintptr_t)token_start <= (uintptr_t)token_end); // check for unsigned overflow

  if (token_length == 0) return false;

  bool is_negative = *token_start == '-';

  uint64_t tmp;
  if (!pjson_parse_uint64_core(&tmp, token_start + (uintptr_t)is_negative, token_end)) {
    return false;
  }

  if (tmp <= (uint64_t)INT64_MAX) *num = !is_negative ? (int64_t)tmp : -(int64_t)tmp;
  else if (is_negative && tmp == ((uint64_t)(INT64_MAX)) + 1) *num = INT64_MIN;
  else return false;

  return true;
}

#ifndef PJSON_NO_LOCALE
static inline char get_decimal_point() {
  struct lconv *lconv = localeconv();
  return lconv->decimal_point[0];
}

static void replace_decimal_point(char *str, const char *str_end, char decimal_point) {
  for (; str < str_end; str++) {
    if (*str == '.') {
      *str = decimal_point;
      return;
    }
  }
}
#endif

bool pjson_parse_float(float *num, const uint8_t *token_start, size_t token_length) {
  assert(num);
  assert(token_start);
  assert((uintptr_t)token_start <= (uintptr_t)(token_start + token_length)); // check for unsigned overflow

  if (token_length == 0) return false;

  char fixed_size_buf[16];
  char *buf;
  if (token_length < sizeof(fixed_size_buf)) {
    buf = fixed_size_buf;
  }
  else if (!(buf = pjson_malloc(token_length + 1))) {
    return false;
  }

  memcpy(buf, token_start, token_length);
#ifndef PJSON_NO_LOCALE
  uint8_t decimal_point = get_decimal_point();
  if (decimal_point != '.') {
    replace_decimal_point(buf, buf + token_length, decimal_point);
  }
#endif
  buf[token_length] = 0;

  char *endptr = NULL;
  errno = 0;
  float tmp = strtof(buf, &endptr);
  bool success = !errno && endptr == buf + token_length;

  if (buf != fixed_size_buf) pjson_free(buf);
  if (!success) return false;

  *num = tmp;
  return true;
}

bool pjson_parse_double(double *num, const uint8_t *token_start, size_t token_length) {
  assert(num);
  assert(token_start);
  assert((uintptr_t)token_start <= (uintptr_t)(token_start + token_length)); // check for unsigned overflow

  if (token_length == 0) return false;

  char fixed_size_buf[24];
  char *buf;
  if (token_length < sizeof(fixed_size_buf)) {
    buf = fixed_size_buf;
  }
  else if (!(buf = pjson_malloc(token_length + 1))) {
    return false;
  }

  memcpy(buf, token_start, token_length);
#ifndef PJSON_NO_LOCALE
  uint8_t decimal_point = get_decimal_point();
  if (decimal_point != '.') {
    replace_decimal_point(buf, buf + token_length, decimal_point);
  }
#endif
  buf[token_length] = 0;

  char *endptr = NULL;
  errno = 0;
  double tmp = strtod(buf, &endptr);
  bool success = !errno && endptr == buf + token_length;

  if (buf != fixed_size_buf) pjson_free(buf);
  if (!success) return false;

  *num = tmp;
  return true;
}
