#pragma once

#include <assert.h>

/* Typedefs */
#include <stdint.h>

/* @todo */
#define ASSERT(cond, ...) assert(cond)
#define INVALID_CODE_PATH { bool COMBINE(invalid_code_path, __LINE__) = false; ASSERT(COMBINE(invalid_code_path, __LINE__)); }

#define CLASS_COPY_DISABLE(Type)\
	Type(const Type &) = delete;\
	Type &operator=(const Type &) = delete;

/* @todo Learn more what the 'move' is.  */
#define CLASS_MOVE_ALLOW(Type)\
	Type(Type &&) = default;\
	Type &operator=(Type &&) = default;

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))
#define ZERO_ARRAY(arr) memset(arr, 0, sizeof(arr)) // Static array, memset required (cstring)

#define COMBINE__(l, r) l##r
#define COMBINE_(l, r)  COMBINE__(l, r)
#define COMBINE(l, r)   COMBINE_(l, r)

#if 1 && defined(__glew_h__)
#define GL_CHECK(...) glGetError(); __VA_ARGS__; { auto COMBINE(error_code, __LINE__) = glGetError(); if(COMBINE(error_code, __LINE__) != GL_NO_ERROR) { fprintf(stderr, "[error] GL_CHECK: line:%d code:%d str:%s exp:%s\n", __LINE__, COMBINE(error_code, __LINE__), glewGetErrorString(COMBINE(error_code, __LINE__)), #__VA_ARGS__); } }
#else
#define GL_CHECK(...) __VA_ARGS__;
#endif

/* requires: stdio.h */
#define PRINT_INT(v) printf("\"%s\" = %d\n", #v, v);