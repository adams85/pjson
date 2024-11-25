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

#ifndef __PJSON_CONFIG_H__
#define __PJSON_CONFIG_H__

// Define any signature decoration for vector APIs
#define PJSON_API(name) name

// Define PJSON_NO_LOCALE if localeconv (locale.h) is not available.
// #define PJSON_NO_LOCALE

#ifndef PJSON_INTERNAL_BUFFER_FIXED_SIZE
#define PJSON_INTERNAL_BUFFER_FIXED_SIZE (256)
#endif

#if !defined(pjson_malloc) && !defined(pjson_realloc) && !defined(pjson_free)
#define pjson_malloc malloc
#define pjson_realloc realloc
#define pjson_free free
#else
#if !defined(pjson_malloc) || !defined(pjson_realloc) || !defined(pjson_free)
#error "Incomplete memory management override. Define either all of pjson_malloc, pjson_realloc and pjson_realloc or none of them."
#endif
#endif

#endif // __PJSON_CONFIG_H__