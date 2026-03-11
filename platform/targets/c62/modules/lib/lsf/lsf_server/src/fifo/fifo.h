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

/// @file fifo.h
/// Basic FIFO implementation.
/// @author Dan A. Muresan
/// @date July - September 2003
///
/// $Revision: 1.7 $
/// Note: producers insert (add) elements to tail of queue;
/// consumers remove elements from head of queue

#define BASIC_FIFO_REVISION "$Revision: 1.7 $"

#if 0  // REQUIRES
  #define STATIC_INLINE
  typedef int BOOL;
  #define TRUE 1
  #define FALSE 0
  typedef int FIFO_SIZE_T;  // optional: some signed type
  typedef int FIFO_CELL_T;
  #define FIFO_T myq        // optional, name of queue type
  #define FIFO_P2_SIZE      // optimize power-of-2 lengths
  #define FIFO_NO_DS        // no dummy slot convention
#endif

#ifndef FIFO_SIZE_T
/// Some signed type.
/// Defaults to @c int
#define FIFO_SIZE_T int
#endif

#include "circ_arr.h"

#ifndef BASIC_FIFO_T
#define BASIC_FIFO_T FIFO_T
#else
#define FIFO_T BASIC_FIFO_T
#endif

/// Basic queue with dummy slot convention. Slots are stored in a circular
/// array. A #head and #tail pointer keep track of the active
/// slots. The queue is empty when <code>#head == #tail</code> and full when
/// a single slot (the dummy slot) remains unused.
///
/// In unframed mode, <code>#frame_size == 0</code>. In framed mode, the
/// circular array covers an integer number of fixed-size "frames" (#size is
/// evenly divisible by #frame_size), while some slots
/// (<code>#max - #size</code> of them) may be left unused.
typedef struct {
  FIFO_CELL_T *base, ///< circular array base addr
              *end;  ///< circular array end
  FIFO_CELL_T volatile *next_frame;  ///< start of next frame in a "framed" queue
#ifdef FIFO_NO_DS
  union {
    volatile FIFO_CELL_T * w;          /// writeable version
    volatile FIFO_CELL_T * const r;    /// read-only version
    volatile FIFO_CELL_T * const l;    /// read-only version
  } __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) head,  ///< head
    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) tail;  ///< tail
#else
  union {
    volatile FIFO_CELL_T * volatile w;          /// writeable version
    volatile FIFO_CELL_T * volatile const r;    /// read-only version
    volatile FIFO_CELL_T * const l;             /// non-volatile read-only version
  } __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) head,  ///< head
    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) tail;  ///< tail
#endif

#ifdef FIFO_NO_DS
  // shared head/tail index
  union {
    volatile FIFO_SIZE_T w;          /// writeable version
    volatile FIFO_SIZE_T const r;    /// read-only version
    FIFO_SIZE_T const l;             /// non-volatile read-only version
  } __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) head_index,  ///< head index
    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) tail_index;  ///< tail index

  FIFO_SIZE_T index_size;  ///< index boundary.
#endif

  FIFO_SIZE_T size,  ///< number of slots currently allocated.
              max,   ///< capacity of underlying circular array
              mask,  ///< size - 1 (if size is a power of 2)
              frame_size;  ///< frame size (0 for unframed mode)
} FIFO_T;

/// @name Initialization
// @{
  /// Declares and statically initializes a queue.
  #define FIFO_DECLARE( q, b, sz ) \
    FIFO_CELL_T b [sz]; \
    FIFO_T q = { head: { w: (b) }, tail: { w: (b) }, base: (b), end: (b) + (sz), \
                 size: (sz), max: (sz), mask: (sz) - 1, \
                 next_frame: NULL, frame_size: 0 }

#ifdef FIFO_NO_DS
  #define FIFO_INIT( q, b, sz ) (\
    (q)->tail.w = (q)->head.w = (b), \
    (q)->base = (b), \
    (q)->end = (b) + (sz), \
    (q)->size = (q)->max = (sz), (q)->mask = (sz) - 1, \
    (q)->head_index.w = 0, \
    (q)->tail_index.w = 0, \
    (q)->index_size = (q)->size + (q)->size, \
    (q)->frame_size = 0, (q)->next_frame = NULL \
  )
#else
  #define FIFO_INIT( q, b, sz ) (\
    (q)->tail.w = (q)->head.w = (b), \
    (q)->base = (b), \
    (q)->end = (b) + (sz), \
    (q)->size = (q)->max = (sz), (q)->mask = (sz) - 1, \
    (q)->frame_size = 0, (q)->next_frame = NULL \
  )
#endif
  /// Configures queue for framed-insertion mode
  /// @param fs frame size (zero for no framing)
  #define FIFO_INIT_FRAME( q, fs ) ( \
    (q)->frame_size = fs, \
    (q)->next_frame = (fs) == 0 ? 0 : (q)->base + fs \
  )
  #ifdef MACROS_TO_INLINES
    STATIC_INLINE void fifo_init (FIFO_T *q, FIFO_CELL_T *base, FIFO_SIZE_T size)
    { FIFO_INIT (q, base, size); }
    STATIC_INLINE void fifo_init_frame (FIFO_T *q, FIFO_SIZE_T fs)
    { FIFO_INIT_FRAME (q, fs); }
  #endif
// @}

/// @name Producers
/// Producer primitives. These functions add/insert elements.
/// Insertion methods are of two kinds<UL>
  /// <LI> copying insertions enqueue elements one at a time or
  /// in blocks. One-by-one insertions operate in framed or unframed mode.
  /// In framed mode, the fifo_ins_frame_chk() keeps track of frame
  /// boundaries and notifies the caller once a new frame has been filled.
  /// <LI> direct access insertion -- unused slots beyond the tail (tail) of
  /// the queue are modified directly, then the tail pointer is updated. The
  /// user is responsible for not crossing over the dummy slot.
/// </UL>
// @{
  /// Checks whether the queue is full.
  STATIC_INLINE BOOL fifo_full (const FIFO_T *q) {
#ifdef FIFO_NO_DS
    volatile const FIFO_SIZE_T head_index = q->head_index.r;
    return CIRC_ARR_NO_DS_FULL (head_index, q->tail_index.l, q->index_size, q->size);
#else
    volatile const FIFO_CELL_T *head = q->head.r;

    #ifdef FIFO_P2_SIZE
    return CIRC_ARR_P2_DS_FULL (head, q->tail.l, q->mask);
    #else
    return CIRC_ARR_DS_FULL (head, q->tail.l, q->size);
    #endif
#endif
  }
  #define FIFO_FULL( q ) \
    fifo_full (q)

  /// Counts available slots.
  STATIC_INLINE FIFO_SIZE_T fifo_avail (const FIFO_T *q) {
#ifdef FIFO_NO_DS
    const volatile FIFO_SIZE_T head_index = q->head_index.r;
    return CIRC_ARR_NO_DS_AVAIL (head_index, q->tail_index.l, q->index_size, q->size);
#else
    const FIFO_CELL_T *head = (const FIFO_CELL_T *) q->head.r;

    #ifdef FIFO_P2_SIZE
      return CIRC_ARR_P2_DS_AVAIL (head, (q)->tail.l, (q)->mask);
    #else
      return CIRC_ARR_DS_AVAIL (head, (q)->tail.l, (q)->size);
    #endif
#endif
  }
  #define FIFO_AVAIL( q ) \
    fifo_avail (q)

  /// Counts slots available without wrapping.
  STATIC_INLINE FIFO_SIZE_T fifo_lavail (const FIFO_T *q) {
#ifdef FIFO_NO_DS
    const volatile FIFO_SIZE_T head_index = q->head_index.r;
    return CIRC_ARR_NO_DS_LAVAIL (head_index, q->tail_index.l, q->index_size, q->size);
#else
    const volatile FIFO_CELL_T *head = q->head.r;

    #ifdef FIFO_P2_SIZE
      return CIRC_ARR_P2_DS_LAVAIL ((q)->base, (q)->end, head, (q)->tail.l, (q)->mask);
    #else
      return CIRC_ARR_DS_LAVAIL ((q)->base, (q)->end, head, (q)->tail.l, (q)->size);
    #endif
#endif
  }
  #define FIFO_LAVAIL( q ) \
    fifo_lavail (q)

  #define FIFO_INS( q, x ) (\
    *((q)->tail.l) = (x), \
    (q)->tail.w = CIRC_ARR_ADV_PTR ((q)->tail.l, (q)->base, (q)->end) \
  )
  #define FIFO_INS_CHK( q, x ) \
    (FIFO_FULL (q) ? FALSE : (FIFO_INS (q, x), TRUE))
  #ifdef MACROS_TO_INLINES
    /// Inserts without checking for overflow.
    STATIC_INLINE void fifo_ins (FIFO_T *q, FIFO_CELL_T x)
    { FIFO_INS (q, x); }
    /// Inserts, checking for overflow first.
    /// @return success (no overflow)
    STATIC_INLINE BOOL fifo_ins_chk (FIFO_T *q, FIFO_CELL_T x)
    { return FIFO_INS_CHK (q, x); }
  #endif

  // slot operations
#ifdef FIFO_NO_DS
  #define FIFO_SWALLOW_SLOTS( q, delta ) (\
    ((q)->tail_index.w = CIRC_ARR_WRAP_FWD ((q)->tail_index.l + delta, (q)->index_size, (q)->index_size)), \
    ((q)->tail.w = (q)->base + (CIRC_ARR_WRAP_FWD ((q)->tail_index.l, (q)->size, (q)->size))) \
  )
#else
  #define FIFO_SWALLOW_SLOTS( q, delta ) (\
    ((q)->tail.w = CIRC_ARR_WRAP_FWD ((q)->tail.l + delta, (q)->end, (q)->size)) \
  )
#endif

  // block operations
#ifdef FIFO_NO_DS
  #define FIFO_SWALLOW( q, delta ) \
    FIFO_SWALLOW_SLOTS( q, delta )
#else
  #define FIFO_SWALLOW( q, delta ) \
    ((q)->tail.w = CIRC_ARR_WRAP_END ((q)->tail.l + delta, (q)->base, (q)->end))
#endif

  #define FIFO_INS_BLK( q, ptr, len ) (\
    memcpy ((FIFO_CELL_T *) (q)->tail.l, ptr, (len) * sizeof (FIFO_CELL_T)), \
    FIFO_SWALLOW (q, len) \
  )
  #define FIFO_INS_BLK_CHK( q, ptr, len ) \
    (FIFO_LAVAIL (q) >= (len) ? (FIFO_INS_BLK (q, ptr, len), TRUE) : FALSE)
  #ifdef MACROS_TO_INLINES
    /// Completes a direct-access insertion.
    /// Moves tail pointer up by the specified amount, "swallowing" slots
    /// on tail of queue and maybe wrap.
    STATIC_INLINE void fifo_swallow_slots (FIFO_T *q, FIFO_SIZE_T delta)
    { FIFO_SWALLOW_SLOTS (q, delta); }

    /// Completes a direct-access insertion.
    /// Moves tail pointer up by the specified amount, "swallowing" slots
    /// on tail of queue.
    STATIC_INLINE void fifo_swallow (FIFO_T *q, FIFO_SIZE_T delta)
    { FIFO_SWALLOW (q, delta); }
    /// Copies slots from an array
    STATIC_INLINE void fifo_ins_blk (FIFO_T *q, FIFO_CELL_T *ptr, FIFO_SIZE_T len)
    { FIFO_INS_BLK (q, ptr, len); }
    /// Copies slots from an array checking for overflow.
    /// If not all slots would fit without wrap-around, no slots are copied.
    /// @return success
    STATIC_INLINE BOOL fifo_ins_blk_chk (FIFO_T *q, FIFO_CELL_T *ptr, FIFO_SIZE_T len)
    { return FIFO_INS_BLK_CHK (q, ptr, len); }
  #endif

  /// Inserts and checks current frame. If full, advances frame pointer.
  /// No overflow check.
  /// @return current frame full
  STATIC_INLINE BOOL fifo_ins_frame_chk (FIFO_T *q, FIFO_CELL_T x) {
    volatile FIFO_CELL_T *ptr = q->tail.l;

    *ptr++ = x;
    if (ptr == q->next_frame) {
      if (ptr == q->end) ptr = q->base;
      q->tail.w = ptr;
      q->next_frame = ptr + q->frame_size;
      return TRUE;
    } else {
      q->tail.w = ptr;
      return FALSE;
    }
  }
  #define FIFO_INS_FRAME_CHK( q, x ) \
    fifo_ins_frame_chk (q, x)
// @}

/// @name Consumers
/// Consumer primitives. These functions remove elements from queue.
// @{
  /// Checks whether the queue is empty
  STATIC_INLINE BOOL fifo_empty (const FIFO_T *q) {
#ifdef FIFO_NO_DS
    volatile const FIFO_SIZE_T tail_index = q->tail_index.r;
    return CIRC_ARR_EMPTY (q->head_index.l, tail_index);
#else
    volatile const FIFO_CELL_T *tail = q->tail.r;
    return CIRC_ARR_EMPTY (q->head.l, tail);
#endif
  }
  #define FIFO_EMPTY( q ) \
    fifo_empty (q)

  STATIC_INLINE FIFO_SIZE_T fifo_len (const FIFO_T *q) {
#ifdef FIFO_NO_DS
    volatile FIFO_SIZE_T tail_index = q->tail_index.r;
    return CIRC_ARR_NO_DS_USED (q->head_index.l, tail_index, q->index_size);
#else
    volatile FIFO_CELL_T *tail = q->tail.r;

    #ifdef FIFO_P2_SIZE
    return CIRC_ARR_P2_USED ((q)->head.l, tail, (q)->mask);
    #else
    return CIRC_ARR_USED ((q)->head.l, tail, (q)->size);
    #endif
#endif
  }
  #define FIFO_LEN( q ) \
    fifo_len (q)
  #define FIFO_USED( q ) \
    FIFO_LEN (q)

  STATIC_INLINE FIFO_SIZE_T fifo_lused (const FIFO_T *q) {
#ifdef FIFO_NO_DS
    volatile FIFO_SIZE_T tail_index = q->tail_index.r;
    return CIRC_ARR_NO_DS_LUSED (q->head_index.l, tail_index, q->index_size, q->size);
#else
    volatile FIFO_CELL_T *tail = q->tail.r;
    return CIRC_ARR_LUSED ((q)->head.l, tail, (q)->end);
#endif
  }
  #define FIFO_LUSED( q ) \
    fifo_lused (q)

  #define FIFO_PEEK( q ) (*((q)->head.l))
  #ifdef MACROS_TO_INLINES
    /// Reads an element from the queue without removing it.
    /// Does not check for underflow
    STATIC_INLINE FIFO_CELL_T fifo_peek (const FIFO_T *q) {
      return FIFO_PEEK (q);
    }
  #endif
  /// Removes slots from queue, without checking for underflow
  STATIC_INLINE FIFO_CELL_T fifo_read (FIFO_T *q) {
    volatile FIFO_CELL_T *head = q->head.l;
    q->head.w = CIRC_ARR_ADV_PTR (head, q->base, q->end);
    return *head;
  }
  #define FIFO_READ( q ) \
    fifo_read (q)

// Wrap around, n <= q->size
#ifdef FIFO_NO_DS
  #define FIFO_DROP( q, n ) (\
    ((q)->head_index.w = CIRC_ARR_WRAP_FWD ((q)->head_index.l + n, (q)->index_size, (q)->index_size)), \
    ((q)->head.w = (q)->base + (CIRC_ARR_WRAP_FWD ((q)->head_index.l, (q)->size, (q)->size))) \
  )
#else
  #define FIFO_DROP( q, n ) (\
    ((q)->head.w = CIRC_ARR_WRAP_FWD ((q)->head.l + n, (q)->end, (q)->size)) \
  )
#endif

  #define FIFO_CLEAR( q ) \
    ((q)->head.w = (FIFO_CELL_T *) (q)->tail.r)
  #define FIFO_RESET( q ) \
    ((q)->tail.w = (q)->head.w = (q)->base)
// No wrap around
  #define FIFO_READ_BLK( q, ptr, len ) ( \
    memcpy (ptr, (FIFO_CELL_T *) (q)->head.l, len * sizeof (FIFO_CELL_T)), \
    FIFO_DROP (q, len) \
  )
  #ifdef MACROS_TO_INLINES
    /// Skips several slots
    STATIC_INLINE void fifo_drop (FIFO_T *q, FIFO_SIZE_T n)
    { FIFO_DROP (q, n); }
    /// Empties queue
    STATIC_INLINE void fifo_clear (FIFO_T *q)
    { FIFO_CLEAR (q); }
    /// Reset queue
    STATIC_INLINE void fifo_reset (FIFO_T *q)
    { FIFO_RESET (q); }
    /// Reads several slots into an array
    STATIC_INLINE void fifo_read_blk (FIFO_T *q, FIFO_CELL_T *ptr, FIFO_SIZE_T len)
    { FIFO_READ_BLK (q, ptr, len); }
  #endif
// @}
