/*
 * Copyright (c) 2018 Peter Nelson (peter@peterdn.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "unity_fixture.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4210)
#endif // _MSC_VER

int main(void) {
  unsigned int rand_seed = (unsigned int)time(NULL);
  srand(rand_seed);
  TEST_PRINTF("Random seed: %u\n\n", rand_seed);

  UNITY_BEGIN();
  RUN_TEST_GROUP(basics);
  RUN_TEST_GROUP(errors);
  RUN_TEST_GROUP(feed_fuzzy);
  RUN_TEST_GROUP(parse_datastruct);
  RUN_TEST_GROUP(value_helpers);
  return UNITY_END();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
