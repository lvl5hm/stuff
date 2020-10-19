#ifndef LVL5_CONTEXT

#include <stdio.h>
#include "lvl5_types.h"
#include <stdlib.h>
#include <string.h>

struct Arena {
  byte *data;
  Mem_Size size;
  Mem_Size capacity;
};

enum class Alloc_Op {
  ALLOC,
  FREE,
  REALLOC,
  FREE_ALL
};

// REMEMBER: aligned_alloc

//                        op, alloc_size, allocator_data, old_ptr, align
typedef void *(*Allocator)(Alloc_Op, Mem_Size, void *, void *, Mem_Size);

struct Context {
  Arena *scratch;
  void *allocator_data;
  Allocator allocator;
};

void *heap_allocator(Alloc_Op op, Mem_Size size, void *allocator_data, void *old_ptr, Mem_Size align = 4*8) {
  void *result = nullptr;

  switch (op) {
    case Alloc_Op::ALLOC: {
        result = _aligned_malloc(size, align);
    } break;
    case Alloc_Op::REALLOC: {
        result = _aligned_realloc(old_ptr, size, align);
    } break;
    case Alloc_Op::FREE: {
        _aligned_free(old_ptr);
    } break;
    case Alloc_Op::FREE_ALL: {

    } break;
  }

  return result;
}


#define __DEFAULT_SCRATCH_CAPACITY kilobytes(40)
globalvar thread_local Arena __global_default_scratch;
globalvar thread_local Context __global_context_stack[16];
globalvar thread_local u32 __global_context_count = 0;

void init_default_context() {
  __global_default_scratch = {
    .data = (byte *)_aligned_malloc(__DEFAULT_SCRATCH_CAPACITY, 16),
    .capacity = __DEFAULT_SCRATCH_CAPACITY,
  };

  __global_context_stack[__global_context_count++] = {
    .allocator = heap_allocator,
    .allocator_data = nullptr,
    .scratch = &__global_default_scratch,
  };
}

struct Context_Scope {
  Context_Scope() {}
  Context_Scope(const Context_Scope&) = default;	
  ~Context_Scope() { __global_context_count--; }
};

Context *get_context() {
  assert(__global_context_count > 0);
  Context *result = __global_context_stack + __global_context_count - 1;
  return result;
}

void *arena_allocator(Alloc_Op op, Mem_Size size, void *allocator_data, void *old_ptr, Mem_Size align = 4*8) {
  void *result = nullptr;
  Arena *arena = (Arena *)allocator_data;

  switch (op) {
    case Alloc_Op::ALLOC: {
      Mem_Size aligned_size = size;
      if (aligned_size % align != 0) {
          aligned_size += align - aligned_size % align;
      }

      Mem_Size ptr = (Mem_Size)arena->data + arena->size;
      if (ptr % align != 0) {
          Mem_Size added_align = align - ptr % align;
          ptr += added_align;
          arena->size += added_align;
      }
      result = (void *)ptr;
      arena->size += aligned_size;
    } break;
    case Alloc_Op::REALLOC: {
      // TODO: to realloc on an arena, we need the old block size
      // Mem_Size aligned_size = size;
      // if (aligned_size % align != 0) {
      //     aligned_size += align - aligned_size % align;
      // }

      // Mem_Size ptr = (Mem_Size)arena->data + arena->size;
      // if (ptr % align != 0) {
      //     Mem_Size added_align = align - ptr % align;
      //     ptr += added_align;
      //     arena->size += added_align;
      // }
      // result = (void *)ptr;
      // arena->size += aligned_size;
    } break;
    case Alloc_Op::FREE: {

    } break;
    case Alloc_Op::FREE_ALL: {
      arena->size = 0;
    } break;
  }
  return result;
}

void *scratch_allocator(Alloc_Op op, Mem_Size size, void *allocator_data, void *old_ptr, Mem_Size align = 4*8) {
  Context *ctx = get_context();
  void *result = arena_allocator(op, size, ctx->scratch, old_ptr, align);
  return result;
}

void *memalloc(Mem_Size size, Mem_Size align = 4*8) {
  Context *ctx = get_context();
  void *result = ctx->allocator(Alloc_Op::ALLOC, size, ctx->allocator_data, nullptr, align);
  return result;
}

void *memrealloc(void *old_ptr, Mem_Size size, Mem_Size align = 4*8) {
  Context *ctx = get_context();
  void *result = ctx->allocator(Alloc_Op::REALLOC, size, ctx->allocator_data, nullptr, align);
  return result;
}

void memfree(void *old_ptr) {
  Context *ctx = get_context();
  assert(old_ptr);
  ctx->allocator(Alloc_Op::FREE, 0, ctx->allocator_data, old_ptr, 0);
}

Context *__push_blank_context() {
  assert(__global_context_count < array_count(__global_context_stack));

  Context *old = get_context();
  Context *ctx = __global_context_stack + __global_context_count;
  *ctx = *old;
  __global_context_stack[__global_context_count++] = *ctx;
  return ctx;
}

Context_Scope push_context(Allocator allocator, void *allocator_data = nullptr) {
  Context *ctx = __push_blank_context();
  ctx->allocator = allocator;
  ctx->allocator_data = allocator_data;
  return {};
}

Context_Scope push_context(Arena *arena) {
  Context *ctx = __push_blank_context();
  ctx->allocator = arena_allocator;
  ctx->allocator_data = arena;
  return {};
}

Context_Scope push_scratch_context() {
  Context *ctx = __push_blank_context();
  ctx->allocator = scratch_allocator;
  return {};
}


struct sb_Header {
  void *data;
  u32 count;
  u32 capacity;

  Allocator allocator;
  void *allocator_data;
};

#define sb_make(T, capacity) (T *)(__sb_make(sizeof(T), capacity))
#define sb_count(arr) (((sb_Header *)(arr) - 1)->count)
#define sb_push(arr, element) (__sb_maybe_grow((void **)&(arr), sizeof(element)), (arr)[sb_count(arr) - 1] = element)

void *__sb_make(Mem_Size element_size, u32 capacity) {
  Context *ctx = get_context();

  sb_Header *array_memory = (sb_Header *)memalloc(sizeof(sb_Header) + element_size*capacity);

  sb_Header header = {
    .data = array_memory + 1,
    .capacity = capacity,
    .count = 0,
    .allocator = ctx->allocator,
    .allocator_data = ctx->allocator_data,
  };
  *array_memory = header;

  return header.data;
}

void __sb_maybe_grow(void **arr_ptr, Mem_Size element_size) {
  void *arr = *arr_ptr;
  assert(arr);
  sb_Header *header = (sb_Header *)arr - 1;
  assert(header->allocator);
  assert(header->capacity);

  if (header->count == header->capacity) {
    header->capacity *= 2;
    sb_Header *array_memory = (sb_Header *)memalloc(sizeof(sb_Header) + element_size*header->capacity);
    memcpy(array_memory + 1, header->data, header->count*element_size);
    header->data = array_memory + 1;
    *array_memory = *header;
    memfree(header);
    header = array_memory;
  }

  header->count++;
  *arr_ptr = header->data;
}

#define LVL5_CONTEXT
#endif