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

/// @file circ_arr.h
/// Primitive operations on circular arrays.
/// @author Dan A. Muresan
/// @date July - September 2003

#ifndef __CIRC_ARR_H
#define __CIRC_ARR_H

#ifndef STATIC_INLINE
  /// Qualifier for inline functions declared in FifoEmbed headers.
  /// Defaults to 'static inline' if not predefined.
  /// If the C compiler does not support 'inline', define to 'static'
  #define STATIC_INLINE static inline
#endif

/// @name Wrapping macros
/// Macros that wrap pointers in circular arrays
// @{
  /// wraps a ptr that may have reached the boundary
  #define CIRC_ARR_WRAP_END( ptr, base, end ) \
    ((ptr) == (end) ? (base) : (ptr))

  /// wraps a ptr that may have reached or gone beyond boundary
  #define CIRC_ARR_WRAP_FWD( ptr, end, sz ) \
    ((ptr) >= (end) ? (ptr) - (sz) : (ptr))

  /// reverse-wraps a pointer that has underflowed below the base address
  #define CIRC_ARR_WRAP_REV( ptr, base, sz ) \
    ((ptr) < (base) ? (ptr) + (sz) : (ptr))

  /// adjusts for both overflow and underflow
  #define CIRC_ARR_WRAP( ptr, base, end, sz ) \
    ((ptr) < (base) ? (ptr) + (sz) : ((ptr) >= (end) ? (ptr) - (sz) : (ptr)))

  /// advances a ptr, wraps around if necessary
  #define CIRC_ARR_ADV_PTR( ptr, base, end ) \
    CIRC_ARR_WRAP_END (ptr + 1, base, end)
// @}

///@name Counting macros
// @{
  /// checks whether whether the circular array is empty
  #define CIRC_ARR_EMPTY( head, tail ) \
    ((head) == (tail))

  /// counts used slots
  #define CIRC_ARR_NO_DS_USED( head_index, tail_index, index_sz ) \
    (CIRC_ARR_WRAP((tail_index)-(head_index), 0, index_sz, index_sz))

  #define CIRC_ARR_USED( head, tail, sz ) \
    CIRC_ARR_WRAP_REV ((tail) - (head), 0, sz)

  /// counts the number of contiguous slots available starting at head under No dummy slot convention
  STATIC_INLINE int CIRC_ARR_NO_DS_LUSED(int head_index, int tail_index, int index_sz, int sz) {
    // total data slots = (((tail_index) - (head_index) + (index_sz)) % (index_sz))
    // gap between head and end:
    //    when right half circle, head to half end;
    //    when left half circle, head to whole end.
    // contiguous data slots from head = min(total data slots, gap between head and end)

    int data_slots = CIRC_ARR_WRAP(tail_index - head_index, 0, index_sz, index_sz);
    int gap_head2end = (head_index < sz) ? (sz - head_index) : (index_sz - head_index);

    return ((data_slots <= gap_head2end) ? data_slots : gap_head2end);
  }

  /// counts the number of contiguous slots available starting at @p head
  #define CIRC_ARR_LUSED( head, tail, end ) \
    ((tail) >= (head) ? (tail) - (head) : (end) - (head))

  // data slots = (CIRC_ARR_WRAP((tail_index)-(head_index), 0, index_sz, index_sz))
  /// checks for fullness under NO dummy slot convention
  #define CIRC_ARR_NO_DS_FULL( head_index, tail_index, index_sz, sz ) \
    ((CIRC_ARR_WRAP((tail_index)-(head_index), 0, index_sz, index_sz)) == (sz))

  /// checks for fullness under dummy slot convention
  #define CIRC_ARR_DS_FULL( head, tail, sz ) \
    (((tail) + 1 - (head) == 0) || ((tail) + 1 - (head) == (sz)))

  /// counts available slots (excluding dummy slot) under No dummy slot convention
  #define CIRC_ARR_NO_DS_AVAIL( head_index, tail_index, index_sz, sz ) \
    (sz - (CIRC_ARR_WRAP((tail_index) - (head_index), 0, index_sz, index_sz)))

  /// counts available contiguous slots (no wrap-around) under No dummy slot convention
  STATIC_INLINE int CIRC_ARR_NO_DS_LAVAIL(int head_index, int tail_index, int index_sz, int sz) {
    // total data slots = (((tail_index) - (head_index) + (index_sz)) % (index_sz))
    // total empty slots = sz - total data slots
    // gap between tail and end:
    //    when right half circle, tail to half end;
    //    when left half circle, tail to whole end.
    // contiguous slots from tail = min(total empty slots, gap between tail and end)

    int empty_slots = sz - (CIRC_ARR_WRAP(tail_index - head_index, 0, index_sz, index_sz));
    int gap_tail2end = (tail_index < sz) ? (sz - tail_index) : (index_sz - tail_index);

    return ((empty_slots <= gap_tail2end) ? empty_slots : gap_tail2end);
  }

  /// counts available slots (excluding dummy slot)
  #define CIRC_ARR_DS_AVAIL( head, tail, sz ) \
    CIRC_ARR_WRAP_REV (head - 1 - tail, 0, sz)

  /// counts available contiguous slots (no wrap-around)
  #define CIRC_ARR_DS_LAVAIL( base, end, head, tail, sz ) \
    ((head) <= (tail) \
     ? (end) - (tail) - ((head) == (base) ? 1 : 0) \
     : CIRC_ARR_DS_AVAIL (head, tail, sz) \
    )
// @}

/// @name "Power of 2" macros
/// Special versions of counting and wrapping macros that assume @c size is
/// a power of 2 and <code>mask == size - 1</code>
// @{
  #define CIRC_ARR_P2_OFFSET( ptr, base, mask ) \
    (((ptr) - (base)) & (mask))

  #define CIRC_ARR_P2_WRAP( ptr, base, mask ) \
    ((base) + CIRC_ARR_P2_OFFSET (ptr, base, mask))

  #define CIRC_ARR_P2_ADV_PTR( ptr, base, mask ) \
    CIRC_ARR_P2_WRAP (ptr + 1, base, mask)

  #define CIRC_ARR_P2_USED( head, tail, mask ) \
    CIRC_ARR_P2_OFFSET (tail, head, mask)

  #define CIRC_ARR_P2_DS_FULL( head, tail, mask ) \
    (CIRC_ARR_P2_OFFSET (head, tail, mask) == 1)

  #define CIRC_ARR_P2_DS_AVAIL( head, tail, mask ) \
    CIRC_ARR_P2_OFFSET (tail - 1, head, mask)

  #define CIRC_ARR_P2_DS_LAVAIL( base, end, head, tail, mask ) \
    ((head) <= (tail) \
     ? (end) - (tail) - ((head) == (base) ? 1 : 0) \
     : CIRC_ARR_P2_DS_AVAIL (head, tail, mask) \
    )
// @}

#endif
