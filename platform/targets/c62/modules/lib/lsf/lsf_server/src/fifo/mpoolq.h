/*
Copyright (c) 2004-2006, Dan Muresan
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are 
met:

* Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright 
notice, this list of conditions and the following disclaimer in the 
documentation and/or other materials provided with the distribution.

* The names of the copyright holders may be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER 
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/// @file mpoolq.h
/// Memory pool queue (FIFO allocator).
/// @author Dan A. Muresan
/// @date July 2003 - March 2004
///
/// $Revision: 1.8 $
/// The user can allocate variable-sized blocks out of a pool (MPOOLQ_T); blocks in
/// use can be freed completely or partially, resized, or split in multiple
/// parts. Deallocation can be done in any order, but freed space can
/// only be reused when there is a match between the deallocation sequence
/// and the original allocation order. Thus out-of-order deallocation can
/// create temporary holes that
/// may waste valuable space and cause the pool to fill up prematurely.
/// The pool has primitive queue capabilities, so that no external queue
/// module is necessary if allocation/deallocation is done in FIFO order.

#ifndef __MPOOLQ_H
#define __MPOOLQ_H

#define MPOOLQ_REVISION "$Revision: 1.8 $"

#if 0  // REQUIRES
  #define STATIC_INLINE
  typedef int BOOL;
  #define TRUE 1
  #define FALSE 0
  #define NULL 0
  // Signed type. Dictates alignment and the max size of a block
  #define MPOOLQ_CELL_T int
  // Optional -- turns debugging on
  #define ASSERT_MSG( cond, msg ) ((cond) || (printf ("%s", msg), abort (), TRUE))
  #define MPOOLQ_T mpoolq  // Optional -- name the abstract data type
#endif

#include "circ_arr.h"

/// @name User units
// @{
  #ifndef MPOOLQ_USER_T
    /// Type with a width of 1 user unit.
    /// If not predefined, it defaults to @c char.
    typedef signed char MPOOLQ_USER_T;
  #endif
  /// Number of user units per cell
  #define MPOOLQ_UUS_PER_CELL ((MPOOLQ_CELL_T) (sizeof (MPOOLQ_CELL_T) / sizeof (MPOOLQ_USER_T)))
  #ifndef MPOOLQ_UU_TO_CELLS
    /// Converts user units to cells.
    #define MPOOLQ_UU_TO_CELLS( n ) (((n) + 1) / MPOOLQ_UUS_PER_CELL)
  #endif
  #ifndef MPOOLQ_CELLS_TO_UU
    /// Converts cells to user units.
    #define MPOOLQ_CELLS_TO_UU( n ) ((n) * MPOOLQ_UUS_PER_CELL)
  #endif
// @}

/// Memory pool data structure.
/// The pool uses as support a circular array of an underlying integer
/// type that guarantees alignment and that can also hold the maximum
/// block size.
/// The array elements are "cells", but the API interface specifies block
/// sizes in "user units" (UU), which may be different from cells.
///
/// The active part of the pool (the #head -> #tail range) is formatted as a
/// list of contiguous blocks; the inactive part (the #head -> #tail range)
/// is not maintained in any format. Each block has a header cell that stores
/// the block's size, with the convention
/// <code>block [0] = used ? UUs : -(cells + 1)</code>
/// where neither @c cells nor @c UUs includes the header itself.
///
/// mpoolq_alloc() returns monotonically increasing addresses (with wrap-around).
/// Holes can occur at end of pool or if blocks are freed out of order.
/// The deallocator compacts free blocks and may run in O(n) if out of order
/// deallocation is used.
typedef struct MPOOLQ_T {
  MPOOLQ_CELL_T *base,  ///< Base address of circular array
                *end;   ///< Circular array end
  union {
    volatile MPOOLQ_CELL_T * volatile w;          /// writeable version
    volatile MPOOLQ_CELL_T * volatile const r;    /// read-only version
    volatile MPOOLQ_CELL_T * const l;             /// non-volatile read-only version
  } __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) head,   ///< Ptr to beginning of allocated space
    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) tail;   ///< Ptr to beginning of free space
  MPOOLQ_CELL_T size;   ///< Circular array capacity
} MPOOLQ_T;

/// @name Block-related
// @{
  /// block size (in UUs) for a currently allocated block
  #define MPOOLQ_BLK_SIZE( blk ) \
    (((volatile MPOOLQ_CELL_T *) (blk)) [-1])
  /// block size (in cells)
  #define MPOOLQ_BLK_CELLS( blk ) \
    MPOOLQ_UU_TO_CELLS (MPOOLQ_BLK_SIZE (blk))
  #ifdef MACROS_TO_INLINES
  #endif
// @}

/// @name Debug primitves
/// Error-checking primitives. To enable them the user must pre-define
/// the macro <code>ASSERT_MSG( cond, msg_on_failure )</code>.
/// Otherwise, no error checking is performed. Error checking,
/// when enabled, changes the algorithmic complexity of the code.
// @{
  #ifdef ASSERT_MSG
  #define MPOOLQ_ASSERT_MSG( cond, msg ) ASSERT_MSG (cond, msg)
  #else
  #define MPOOLQ_ASSERT_MSG( cond, msg ) ((void) 0)
  #endif

  /// Checks whether @p ptr points to a location inside the circular array
  #define MPOOLQ_CHK_PTR( p, ptr ) \
    MPOOLQ_ASSERT_MSG ((p)->base <= (ptr) && (ptr) < (p)->end, \
                       "mpoolq: bad ptr")

  #ifdef ASSERT_MSG
    /// Checks that @p blk points to (what looks like) a valid allocated block
    STATIC_INLINE void mpoolq_chk_blk (MPOOLQ_T *p, volatile void *blk) {
      MPOOLQ_CELL_T len;

      MPOOLQ_CHK_PTR (p, ((MPOOLQ_CELL_T *) blk) - 1);
      len = MPOOLQ_BLK_CELLS (blk);
      assert: len 为 28/4 + N*32/sizeof(MPOOLQ_CELL_T)
      MPOOLQ_ASSERT_MSG (0 <= len, "mpoolq: blk size < 0");
      MPOOLQ_ASSERT_MSG (len + (MPOOLQ_CELL_T *) blk <= (p)->end,
                         "mpoolq blk size too big");
    }
  #else
    #define mpoolq_chk_blk( p, blk ) ((void) 0)
  #endif
  #define MPOOLQ_CHK_BLK( p, blk ) mpoolq_chk_blk ((MPOOLQ_T *) (p), blk)

  /// Simple invariant check. Checks that MPOOLQ_T::tail and MPOOLQ_T::head
  /// are within the allowable range and the active range does not start
  /// with a hole.
  #define MPOOLQ_CHK_INVAR( p ) ( \
    MPOOLQ_CHK_PTR (p, (p)->head.r), \
    MPOOLQ_CHK_PTR (p, (p)->tail.r) \
  )
// @}

/// @name Initialization
// @{
  #define MPOOLQ_INIT( p, b, cells ) (\
    (p)->tail.w = (p)->head.w = (MPOOLQ_CELL_T *) (b), \
    (p)->base = (MPOOLQ_CELL_T *) (b), \
    (p)->size = (cells), \
    (p)->end = (p)->base + (p)->size \
  )
  /// Declares a queue and the underlying circular array statically
  #define MPOOLQ_DECLARE( p, b, cells ) \
    MPOOLQ_CELL_T b [cells]; \
    MPOOLQ_T p = { base: (b), end: ((b) + (cells)), head: (b), tail: (b), size: (cells), max: (cells) };
  #ifdef MACROS_TO_INLINES
    STATIC_INLINE void mpoolq_init (MPOOLQ_T *p, MPOOLQ_CELL_T *base, MPOOLQ_CELL_T cells) {
      MPOOLQ_INIT (p, base, cells);
    }
  #endif
  /// Counts # of cells needed to hold @p n objects of size @p s (in UUs)
  #define MPOOLQ_REQ_CELLS( s, n ) \
    (1 + (MPOOLQ_UU_TO_CELLS (s) + 1) * n)
  /// Same as #MPOOLQ_REQ_CELLS but gives the result in UUs
// @}

/// @name Producers.
/// Allocator and related primitives.
// @{
  /// Creates a hole at the end of the pool.
  #define MPOOLQ_CREATE_HOLE( p, ctail ) ( \
    ((volatile MPOOLQ_CELL_T *) (ctail)) [0] = -((p)->end - (ctail)), \
    (p)->tail.w = (p)->base \
  )

  /// Allocates a block from the inactive part and advances MPOOLQ_T::tail.
  /// The block is allocated at MPOOLQ_T::tail if there is enough space
  /// there; if not, the allocator tries to leave a hole at the end of the
  /// pool, wrap around and allocate a block at the beginning.
  STATIC_INLINE volatile void *mpoolq_alloc (MPOOLQ_T *p, MPOOLQ_CELL_T UUs) {
    MPOOLQ_CELL_T cells = MPOOLQ_UU_TO_CELLS (UUs);
    // 从核间共享内存更新读取head
    xthal_dcache_region_invalidate((void *) &(p->head.r),
                                   IC_DCACHELINE_ROUNDUP_SIZE(sizeof(void *)));

    // synchronization & memory barrier. 期望: full system
#pragma flush_memory

    // 本端私有tail
    volatile MPOOLQ_CELL_T *tail_private = p->tail.w;

    volatile MPOOLQ_CELL_T *head = p->head.r,
                           *old_tail = p->tail.l;
    volatile MPOOLQ_CELL_T *end = old_tail + cells + 1;

    MPOOLQ_CHK_INVAR (p);
    if (end >= head) {
      if (head > old_tail)
        return NULL;
      else {
        if (! (end < p->end
               || (end == p->end && head != p->base)))
        {  // actually wrap around
          if ((end = p->base + cells + 1) >= head)
            return NULL;

          // 创建Hole: old_tail = MPOOLQ_CREATE_HOLE (p, old_tail);
          ((volatile MPOOLQ_CELL_T *) (old_tail)) [0] = -((p)->end - (old_tail));
          // 检查点: hole header记录 == N*(32/4) cells
          MPOOLQ_ASSERT_MSG(((-((p)->end - (old_tail))) % (IC_MAX_DCACHE_LINESIZE/sizeof(MPOOLQ_CELL_T))) == 0,
                            "mpoolq: hole size not aligned");
          // write-back hole header: old_tail[0] => 共享内存, 1 cache line
          xthal_dcache_region_writeback((void *) &(old_tail[0]),
                                        IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T)));

          // synchronization & memory barrier. 期望: full system and store
#pragma flush_memory
          // alloc端脱钩hole header: 1 cache line
          xthal_dcache_region_invalidate((void *) &(old_tail[0]),
                                         IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T)));

          // tail更新
          tail_private = (p)->base;
          // blk位置更新
          old_tail = tail_private;

          // 按新的tail计算分配区间结尾
          end = old_tail + cells + 1;
        }
      }
    }

    MPOOLQ_ASSERT_MSG (old_tail >= p->base && end <= p->end,
                       "mpoolq_alloc: internal error");
    *old_tail = UUs;
    // write-back blk header: old_tail[0] => 共享内存, 1 cache line
    xthal_dcache_region_writeback((void *) &(old_tail[0]),
                                  IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T)));
    // synchronization & memory barrier. 期望: full system and store
#pragma flush_memory
    // alloc端脱钩 blk header: 1 cache line
    xthal_dcache_region_invalidate((void *) &(old_tail[0]),
                                   IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T)));

    // synchronization & memory barrier. 期望: full system
#pragma flush_memory
    tail_private = CIRC_ARR_WRAP_END (end, p->base, p->end);
    (p)->tail.w = tail_private;
    // synchronization & memory barrier. 期望: full system and store
#pragma flush_memory
    // write-back tail => 共享内存, 1 cache line
    xthal_dcache_region_writeback((void *) &(p->tail.w),
                                  IC_DCACHELINE_ROUNDUP_SIZE(sizeof(void *)));
    // synchronization & memory barrier. 期望: full system and store
#pragma flush_memory
    // ->至此更新完整

    return (void *) (1 + old_tail);
  }
  #define MPOOLQ_ALLOC( p , UUs ) \
    mpoolq_alloc ((p), (UUs))

  /// Counts # of cells available
  STATIC_INLINE int mpoolq_avail_cells (const MPOOLQ_T *p) {
    volatile const MPOOLQ_CELL_T *head = p->head.r;
    return (MPOOLQ_CELL_T) CIRC_ARR_DS_AVAIL (head, (p)->tail.l, (p)->size);
  }
  #define MPOOLQ_AVAIL_CELLS( p ) \
    mpoolq_avail_cells (p)

  /// Counts # of contiguous cells available without wrapping (if any).
  /// If exactly zero cells are available at the end of the pool
  /// but more would be available after wrapping, this function leaves a one-cell
  /// hole at the end of the pool, wraps MPOOLQ_T::tail and counts again.
  /// @return zero if the pool is full; otherwise, non-zero number of cells
  /// @c n such that mpoolq_alloc() would succeed with an arg of
  /// <code>#MPOOLQ_UU_TO_CELLS (n)</code>.
  STATIC_INLINE int mpoolq_lavail_cells (MPOOLQ_T *p) {
    volatile MPOOLQ_CELL_T *head = p->head.r,
                           *tail = p->tail.l;

    if (tail + 1 == p->end) {
      if (head == p->base)
        return 0;
      else
        tail = MPOOLQ_CREATE_HOLE (p, tail);
    }
    return CIRC_ARR_DS_LAVAIL (p->base, p->end, head, tail, p->size);
  }
  #define MPOOLQ_LAVAIL_CELLS( p ) \
    mpoolq_lavail_cells (p);
// @}

/// @name Consumers
/// Memory pool consumers.
/// These functions free entire blocks or parts thereof.
// @{
  /// Skips holes until either the tail or an allocated block is reached.
  /// @param blk address of pointer to block header, will be modified
  /// to address of header of next allocated block (or tail if no more
  /// allocated blocks are found)
  /// @return TRUE if a subsequent allocated block could be found
  STATIC_INLINE BOOL mpoolq_next_blk (const MPOOLQ_T *p, volatile MPOOLQ_CELL_T **blk) {
    // 下一次从核间共享内存读取, 1 cache line
    // 由于cache line存在自动轮替, 在多次读取时可能会读到多次不同的值更新, 不影响逻辑
    xthal_dcache_region_invalidate((void *) &(p->tail.r),
                                   IC_DCACHELINE_ROUNDUP_SIZE(sizeof(void *)));
    // synchronization & memory barrier. 期望: system
#pragma flush_memory

    MPOOLQ_CELL_T n;

    for (;;) {
      MPOOLQ_CHK_PTR (p, *blk);
      if (*blk == p->tail.r)
        return FALSE;
      if ((n = - (*blk) [0]) <= 0) {  // found used block
        MPOOLQ_CHK_BLK (p, *blk + 1);
        return TRUE;
      }
      // 检查点: hole header记录 == N*(32/4) cells
      MPOOLQ_ASSERT_MSG((n % (IC_MAX_DCACHE_LINESIZE/sizeof(MPOOLQ_CELL_T))) == 0,
                        "mpoolq: hole size not aligned");
      // header中记录的n<0, 该blk是hole, 已free的blk也已置为hole
      // 先脱钩header的cacheline
      xthal_dcache_region_invalidate((void *) (*blk),
                                     IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T)));

      // 再推进私有head
      *blk = CIRC_ARR_WRAP_END (*blk + n, p->base, p->end);
    }
  }
  #define MPOOLQ_NEXT_BLK( p, blk ) \
    mpoolq_next_blk ((p), (blk))

  /// Checks whether the pool is empty.
  /// Frees holes as a side effect.
  STATIC_INLINE BOOL mpoolq_empty (MPOOLQ_T *p) {
    // head推进时, 使用本端私有变量
    volatile MPOOLQ_CELL_T *blk = (MPOOLQ_CELL_T *) p->head.l;
    BOOL success = MPOOLQ_NEXT_BLK (p, &blk);

    // 若head需要更新, 才write back以让alloc端可见.
    if (p->head.w != blk) {
      p->head.w = blk;

      // synchronization & memory barrier. 期望: store
#pragma flush_memory
      // write-back head => 共享内存, 1 cache line
      xthal_dcache_region_writeback((void *) &(p->head.w),
                                    IC_DCACHELINE_ROUNDUP_SIZE(sizeof(void *)));
      // synchronization & memory barrier. 期望: store
#pragma flush_memory
      // ->至此更新完整
    }

    return ! success;
  }
  #define MPOOLQ_EMPTY( p ) \
    mpoolq_empty (p)

  /// Counts # of cells in use
  STATIC_INLINE int mpoolq_used_cells (MPOOLQ_T *p) {
    const MPOOLQ_CELL_T *tail = (const MPOOLQ_CELL_T *) p->tail.r;

    return MPOOLQ_EMPTY (p) ? 0
             : CIRC_ARR_USED (p->head.l, tail, p->size);
  }
  #define MPOOLQ_USED_CELLS( p ) \
    mpoolq_used_cells (p)

  #define MPOOLQ_OLDEST( p ) \
    (MPOOLQ_EMPTY (p) ? NULL : (volatile MPOOLQ_CELL_T *) (p)->head.l + 1)
  #ifdef MACROS_TO_INLINES
    /// Finds the oldest block in the pool.
    /// @return ptr to oldest allocated blk or NULL
    STATIC_INLINE volatile void *mpoolq_oldest (MPOOLQ_T *p)
    { return MPOOLQ_OLDEST (p); }
  #endif

  /// Frees an entire block
  STATIC_INLINE void mpoolq_free (MPOOLQ_T *p, volatile void *blk) {
    MPOOLQ_CHK_BLK (p, blk);
    MPOOLQ_BLK_SIZE (blk) = - MPOOLQ_BLK_CELLS (blk) - 1;
    MPOOLQ_EMPTY (p);  // result ignored, but frees holes as a side effect
  }
  #define MPOOLQ_FREE( p, blk ) \
    mpoolq_free (p, blk)

  #define MPOOLQ_FREE_OLDEST( p ) \
    (MPOOLQ_EMPTY (p) ? FALSE : (MPOOLQ_FREE ((p), (p)->head.l + 1), TRUE))
  #ifdef MACROS_TO_INLINES
    /// Frees oldest allocated block (if any)
    /// @return TRUE if the pool was non-empty
    STATIC_INLINE BOOL mpoolq_free_oldest (MPOOLQ_T *p)
    { return MPOOLQ_FREE_OLDEST (p); }
  #endif

  /// Frees up to @p n oldest blocks.
  /// @return TRUE if @p n blocks could be freed
  STATIC_INLINE BOOL mpoolq_drop (MPOOLQ_T *p, int n) {
    for (; n-- != 0; )
      if (! MPOOLQ_FREE_OLDEST (p)) return FALSE;
    return TRUE;
  }
  #define MPOOLQ_DROP( p, n ) mpoolq_drop (p, n)

  #define MPOOLQ_CLEAR( p ) ((p)->head.w = (MPOOLQ_CELL_T *) (p)->tail.r)
  #ifdef MACROS_TO_INLINES
    /// Frees all blocks
    STATIC_INLINE void mpoolq_clear (MPOOLQ_T *p)
    { MPOOLQ_CLEAR (p); }
  #endif

  /// Splits a used block.
  /// One cell is lost for the header of the 2nd part.
  /// @param head_UUs size of 1st sub-block to be created
  /// @return addr of 2nd sub-block.
  STATIC_INLINE volatile void *mpoolq_split_blk (volatile void *blk, MPOOLQ_CELL_T head_UUs) {
    volatile MPOOLQ_CELL_T *split = ((MPOOLQ_CELL_T *) blk) + MPOOLQ_UU_TO_CELLS (head_UUs) + 1;

    MPOOLQ_ASSERT_MSG (head_UUs >= 0, "mpoolq_split_blk: negative head size");
    MPOOLQ_BLK_SIZE (split) = MPOOLQ_BLK_SIZE (blk) - head_UUs - MPOOLQ_CELLS_TO_UU (1);
    MPOOLQ_ASSERT_MSG (MPOOLQ_BLK_SIZE (split) >= 0, "mpoolq_split_blk: head too large");
    MPOOLQ_BLK_SIZE (blk) = head_UUs;
    return (void *) split;
  }
  #define MPOOLQ_SPLIT_BLK( blk, head_UUs ) \
    mpoolq_split_blk (blk, head_UUs)

  /// Frees a chunk at the beginning of a block.
  /// @param delta_UUs amount to free, MUST cover an integer # of cells
  /// @return new block start or NULL if entire block was freed
  STATIC_INLINE volatile void *mpoolq_free_bot (MPOOLQ_T *p, volatile void *blk, MPOOLQ_CELL_T delta_UUs) {
    MPOOLQ_CELL_T old_size = MPOOLQ_BLK_SIZE (blk);

    if (old_size == delta_UUs) {  // free entire block
      MPOOLQ_FREE (p, blk);
      return NULL;
    } else {
      volatile MPOOLQ_CELL_T *result = mpoolq_split_blk (blk, delta_UUs - MPOOLQ_CELLS_TO_UU (1));
      MPOOLQ_FREE (p, blk);
      return result;
    }
  }
  #define MPOOLQ_FREE_BOT( p, blk, delta_UUs ) \
    mpoolq_free_bot (p, blk, delta_UUs)

  /// Frees part of a block. Splits the original block and creates a hole.
  /// @param blk    blk addr
  /// @param offset position of part in blk.
  /// <P> @p offset and @p size are in UUs and MUST cover integer # of cells!
  ///     @p size must cover at least<UL>
  ///       <LI> 2 cells if part is in middle of blk
  ///       <LI> 1 cell if part is at head or tail of blk
  ///       <LI> no requirements if part covers entire blk
  ///     </UL>
  STATIC_INLINE void mpoolq_free_part (MPOOLQ_T *p, volatile void *blk, MPOOLQ_CELL_T offset, MPOOLQ_CELL_T size) {
    volatile MPOOLQ_CELL_T *part = (offset > 0) ? mpoolq_split_blk (blk, offset) : (MPOOLQ_CELL_T *) blk;
    if (MPOOLQ_BLK_SIZE (part) > size)
      mpoolq_split_blk (part, size - MPOOLQ_CELLS_TO_UU ((offset > 0) ? 2 : 1));
    MPOOLQ_FREE (p, part);
  }
  #define MPOOLQ_FREE_PART( p, blk, offset, size ) \
    mpoolq_free_part (p, blk, offset, size)
// @}

/// @name Non-concurrent
/// Primitives that do not fit either the Producer or Consumer paradigm.
/// They cannot be used concurrently with other threads.
// @{
  /// Checks that @p blk is the tail of the queue
  #define MPOOLQ_CHK_TAIL_BLK( p, blk ) ( \
    MPOOLQ_CHK_BLK (p, blk), \
    MPOOLQ_ASSERT_MSG ( \
      CIRC_ARR_WRAP_END ((MPOOLQ_CELL_T *) (blk) + MPOOLQ_BLK_CELLS (blk), \
                         (p)->base, (p)->end) \
        == (p)->tail.l, \
      "block not tail") \
  )
  #define MPOOLQ_RELINQ_TAIL( p, blk ) ( \
    MPOOLQ_CHK_TAIL_BLK (p, (MPOOLQ_CELL_T *) blk), \
    (p)->tail.w = ((MPOOLQ_CELL_T *) (blk)) - 1 \
  )
  #ifdef MACROS_TO_INLINES
    /// Relinquishes tail block.
    /// Use ONLY when @p blk points to the last allocated block.
    /// This primitive modifies both used block headers and the
    /// MPOOLQ_T::tail pointer and thus breaks both producer and
    /// consumer paradigms.
    /// @param blk addr of tail block
    STATIC_INLINE void mpoolq_relinq_tail (MPOOLQ_T *p, volatile void *blk)
    { MPOOLQ_RELINQ_TAIL (p, blk); }
  #endif

  /// Changes size of tail block. Same problems as mpoolq_relinq_tail().
  /// @param new_size new size of tail block in UUs (any non-negative number)
  /// @return success
  STATIC_INLINE BOOL mpoolq_realloc_tail (MPOOLQ_T *p, volatile void *blk, MPOOLQ_CELL_T new_size) {
    if (new_size == 0) {
      MPOOLQ_RELINQ_TAIL (p, blk);
      return TRUE;
    }
    MPOOLQ_ASSERT_MSG (new_size > 0, "mpoolq_realloc_tail: negative size");
    MPOOLQ_CHK_TAIL_BLK (p, blk);
    if (CIRC_ARR_DS_LAVAIL (p->base, p->end, p->head.l, (MPOOLQ_CELL_T *) blk, p->size)
        < MPOOLQ_UU_TO_CELLS (new_size))
      return FALSE;
    MPOOLQ_BLK_SIZE (blk) = new_size;
    p->tail.w = CIRC_ARR_WRAP_END (((volatile MPOOLQ_CELL_T *) blk) + MPOOLQ_BLK_CELLS (blk), p->base, p->end);
    return TRUE;
  }
  #define MPOOLQ_REALLOC_TAIL( p, blk, sz ) \
    mpoolq_realloc_tail (p, blk, sz)

  #define MPOOLQ_FREE_TAIL( p, blk, d ) \
    MPOOLQ_REALLOC_TAIL ((p), (blk), MPOOLQ_BLK_SIZE (blk) - (d))
  #ifdef MACROS_TO_INLINES
    /// Shrinks tail block by specified amount.
    STATIC_INLINE void mpoolq_free_tail (MPOOLQ_T *p, MPOOLQ_CELL_T *blk, MPOOLQ_CELL_T delta_UUs) {
      MPOOLQ_FREE_TAIL (p, blk, delta_UUs);
    }
  #endif
// @}

#ifdef ASSERT_MSG
  /// Comprehensive consistency check. Calls #MPOOLQ_CHK_INVAR and
  /// also checks that block headers are not corrupted and that @p blk
  /// is either @c NULL or the address of an allocated block.
  /// Not a true invariant check because it modifies data structure.
  STATIC_INLINE void mpoolq_chk_hdr (MPOOLQ_T *p, volatile void *blk) {
    volatile MPOOLQ_CELL_T *ptr;
    BOOL more, found = blk == NULL;

    MPOOLQ_CHK_INVAR (p);
    if (! MPOOLQ_EMPTY (p))
      for (ptr = MPOOLQ_OLDEST (p), more = TRUE;
           more;
           more = mpoolq_next_blk (p, &ptr), ++ptr)
        if (ptr == blk)
          found = TRUE;

    ASSERT_MSG (found, "mpoolq: address not a block pointer");
  }
#else
  #define mpoolq_chk_hdr( p, blk ) ((void) 0)
#endif
#define MPOOLQ_CHK_HDR( p, blk ) mpoolq_chk_hdr (p, blk)

#endif
