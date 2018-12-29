/**********************************************************************
  Copyright(c) 2011-2016 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions 
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/

#ifndef _IGZIP_H
#define _IGZIP_H

/**
 * @file igzip_lib.h
 *
 * @brief This file defines the igzip compression and decompression interface, a
 * high performance deflate compression interface for storage applications.
 *
 * Deflate is a widely used compression standard that can be used standalone, it
 * also forms the basis of gzip and zlib compression formats. Igzip supports the
 * following flush features:
 *
 * - No Flush: The default method where no special flush is performed.
 *
 * - Sync flush: whereby isal_deflate() finishes the current deflate block at
 *   the end of each input buffer. The deflate block is byte aligned by
 *   appending an empty stored block.
 *
 * - Full flush: whereby isal_deflate() finishes and aligns the deflate block as
 *   in sync flush but also ensures that subsequent block's history does not
 *   look back beyond this point and new blocks are fully independent.
 *
 * Igzip also supports compression levels from ISAL_DEF_MIN_LEVEL to
 * ISAL_DEF_MAX_LEVEL.
 *
 * Igzip contains some behaviour configurable at compile time. These
 * configureable options are:
 *
 * - IGZIP_HIST_SIZE - Defines the window size. The default value is 32K (note K
 *   represents 1024), but 8K is also supported. Powers of 2 which are at most
 *   32K may also work.
 *
 * - LONGER_HUFFTABLES - Defines whether to use a larger hufftables structure
 *   which may increase performance with smaller IGZIP_HIST_SIZE values. By
 *   default this optoin is not defined. This define sets IGZIP_HIST_SIZE to be
 *   8 if IGZIP_HIST_SIZE > 8K.
 *
 *   As an example, to compile gzip with an 8K window size, in a terminal run
 *   @verbatim gmake D="-D IGZIP_HIST_SIZE=8*1024" @endverbatim on Linux and
 *   FreeBSD, or with @verbatim nmake -f Makefile.nmake D="-D
 *   IGZIP_HIST_SIZE=8*1024" @endverbatim on Windows.
 *
 */
#include <stdint.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Deflate Compression Standard Defines */
/******************************************************************************/
#define IGZIP_K  1024
#define ISAL_DEF_MAX_HDR_SIZE 328
#define ISAL_DEF_MAX_CODE_LEN 15
#define ISAL_DEF_HIST_SIZE (32*IGZIP_K)

#define ISAL_DEF_LIT_SYMBOLS 257
#define ISAL_DEF_LEN_SYMBOLS 29
#define ISAL_DEF_DIST_SYMBOLS 30
#define ISAL_DEF_LIT_LEN_SYMBOLS (ISAL_DEF_LIT_SYMBOLS + ISAL_DEF_LEN_SYMBOLS)

#define ISAL_LOOK_AHEAD (18 * 16)	/* Max repeat length, rounded up to 32 byte boundary */

/******************************************************************************/
/* Deflate Implemenation Specific Defines */
/******************************************************************************/
/* Note IGZIP_HIST_SIZE must be a power of two */
#ifndef IGZIP_HIST_SIZE
#define IGZIP_HIST_SIZE ISAL_DEF_HIST_SIZE
#endif

#if (IGZIP_HIST_SIZE > ISAL_DEF_HIST_SIZE)
#undef IGZIP_HIST_SIZE
#define IGZIP_HIST_SIZE ISAL_DEF_HIST_SIZE
#endif

#ifdef LONGER_HUFFTABLE
#if (IGZIP_HIST_SIZE > 8 * IGZIP_K)
#undef IGZIP_HIST_SIZE
#define IGZIP_HIST_SIZE (8 * IGZIP_K)
#endif
#endif

#define ISAL_LIMIT_HASH_UPDATE

#ifndef IGZIP_HASH_SIZE
#define IGZIP_HASH_SIZE  (8 * IGZIP_K)
#endif

#ifdef LONGER_HUFFTABLE
enum {IGZIP_DIST_TABLE_SIZE = 8*1024};

/* DECODE_OFFSET is dist code index corresponding to DIST_TABLE_SIZE + 1 */
enum { IGZIP_DECODE_OFFSET = 26 };
#else
enum {IGZIP_DIST_TABLE_SIZE = 2};
/* DECODE_OFFSET is dist code index corresponding to DIST_TABLE_SIZE + 1 */
enum { IGZIP_DECODE_OFFSET = 0 };
#endif
enum {IGZIP_LEN_TABLE_SIZE = 256};
enum {IGZIP_LIT_TABLE_SIZE = ISAL_DEF_LIT_SYMBOLS};

#define IGZIP_HUFFTABLE_CUSTOM 0
#define IGZIP_HUFFTABLE_DEFAULT 1
#define IGZIP_HUFFTABLE_STATIC 2

/* Flush Flags */
#define NO_FLUSH	0	/* Default */
#define SYNC_FLUSH	1
#define FULL_FLUSH	2
#define FINISH_FLUSH	0	/* Deprecated */

/* Gzip Flags */
#define IGZIP_DEFLATE	0	/* Default */
#define IGZIP_GZIP	1
#define IGZIP_GZIP_NO_HDR	2

/* Compression Return values */
#define COMP_OK 0
#define INVALID_FLUSH -7
#define INVALID_PARAM -8
#define STATELESS_OVERFLOW -1
#define ISAL_INVALID_OPERATION -9
#define ISAL_INVALID_LEVEL -4	/* Invalid Compression level set */

/**
 *  @enum isal_zstate_state
 *  @brief Compression State please note ZSTATE_TRL only applies for GZIP compression
 */


/* When the state is set to ZSTATE_NEW_HDR or TMP_ZSTATE_NEW_HEADER, the
 * hufftable being used for compression may be swapped
 */
enum isal_zstate_state {
	ZSTATE_NEW_HDR,  //!< Header to be written
	ZSTATE_HDR,	//!< Header state
	ZSTATE_CREATE_HDR, //!< Header to be created
	ZSTATE_BODY,	//!< Body state
	ZSTATE_FLUSH_READ_BUFFER, //!< Flush buffer
	ZSTATE_FLUSH_ICF_BUFFER,
	ZSTATE_SYNC_FLUSH, //!< Write sync flush block
	ZSTATE_FLUSH_WRITE_BUFFER, //!< Flush bitbuf
	ZSTATE_TRL,	//!< Trailer state
	ZSTATE_END,	//!< End state
	ZSTATE_TMP_NEW_HDR, //!< Temporary Header to be written
	ZSTATE_TMP_HDR,	//!< Temporary Header state
	ZSTATE_TMP_CREATE_HDR, //!< Temporary Header to be created state
	ZSTATE_TMP_BODY,	//!< Temporary Body state
	ZSTATE_TMP_FLUSH_READ_BUFFER, //!< Flush buffer
	ZSTATE_TMP_FLUSH_ICF_BUFFER,
	ZSTATE_TMP_SYNC_FLUSH, //!< Write sync flush block
	ZSTATE_TMP_FLUSH_WRITE_BUFFER, //!< Flush bitbuf
	ZSTATE_TMP_TRL,	//!< Temporary Trailer state
	ZSTATE_TMP_END	//!< Temporary End state
};

/* Offset used to switch between TMP states and non-tmp states */
#define ZSTATE_TMP_OFFSET ZSTATE_TMP_HDR - ZSTATE_HDR

/******************************************************************************/
/* Inflate Implementation Specific Defines */
/******************************************************************************/
#define ISAL_DECODE_LONG_BITS 12
#define ISAL_DECODE_SHORT_BITS 10

/* Current state of decompression */
enum isal_block_state {
	ISAL_BLOCK_NEW_HDR,	/* Just starting a new block */
	ISAL_BLOCK_HDR,		/* In the middle of reading in a block header */
	ISAL_BLOCK_TYPE0,	/* Decoding a type 0 block */
	ISAL_BLOCK_CODED,	/* Decoding a huffman coded block */
	ISAL_BLOCK_INPUT_DONE,	/* Decompression of input is completed */
	ISAL_BLOCK_FINISH	/* Decompression of input is completed and all data has been flushed to output */
};

/* Inflate Return values */
#define ISAL_DECOMP_OK 0	/* No errors encountered while decompressing */
#define ISAL_END_INPUT 1	/* End of input reached */
#define ISAL_OUT_OVERFLOW 2	/* End of output reached */
#define ISAL_INVALID_BLOCK -1	/* Invalid deflate block found */
#define ISAL_INVALID_SYMBOL -2	/* Invalid deflate symbol found */
#define ISAL_INVALID_LOOKBACK -3	/* Invalid lookback distance found */

/******************************************************************************/
/* Compression structures */
/******************************************************************************/
/** @brief Holds histogram of deflate symbols*/
struct isal_huff_histogram {
	uint64_t lit_len_histogram[ISAL_DEF_LIT_LEN_SYMBOLS]; //!< Histogram of Literal/Len symbols seen
	uint64_t dist_histogram[ISAL_DEF_DIST_SYMBOLS]; //!< Histogram of Distance Symbols seen
	uint16_t hash_table[IGZIP_HASH_SIZE]; //!< Tmp space used as a hash table
};

struct isal_mod_hist {
    uint32_t d_hist[30];
    uint32_t ll_hist[513];
};

#define ISAL_DEF_MIN_LEVEL 0
#define ISAL_DEF_MAX_LEVEL 1

/* Defines used set level data sizes */
#define ISAL_DEF_LVL0_REQ 0
#define ISAL_DEF_LVL1_REQ 4 * IGZIP_K /* has to be at least sizeof(struct level_2_buf) */
#define ISAL_DEF_LVL1_TOKEN_SIZE 4

/* Data sizes for level specific data options */
#define ISAL_DEF_LVL0_MIN ISAL_DEF_LVL0_REQ
#define ISAL_DEF_LVL0_SMALL ISAL_DEF_LVL0_REQ
#define ISAL_DEF_LVL0_MEDIUM ISAL_DEF_LVL0_REQ
#define ISAL_DEF_LVL0_LARGE ISAL_DEF_LVL0_REQ
#define ISAL_DEF_LVL0_EXTRA_LARGE ISAL_DEF_LVL0_REQ
#define ISAL_DEF_LVL0_DEFAULT ISAL_DEF_LVL0_REQ

#define ISAL_DEF_LVL1_MIN (ISAL_DEF_LVL1_REQ + ISAL_DEF_LVL1_TOKEN_SIZE * 1 * IGZIP_K)
#define ISAL_DEF_LVL1_SMALL (ISAL_DEF_LVL1_REQ + ISAL_DEF_LVL1_TOKEN_SIZE * 16 * IGZIP_K)
#define ISAL_DEF_LVL1_MEDIUM (ISAL_DEF_LVL1_REQ + ISAL_DEF_LVL1_TOKEN_SIZE * 32 * IGZIP_K)
#define ISAL_DEF_LVL1_LARGE (ISAL_DEF_LVL1_REQ + ISAL_DEF_LVL1_TOKEN_SIZE * 64 * IGZIP_K)
#define ISAL_DEF_LVL1_EXTRA_LARGE (ISAL_DEF_LVL1_REQ + ISAL_DEF_LVL1_TOKEN_SIZE * 128 * IGZIP_K)
#define ISAL_DEF_LVL1_DEFAULT ISAL_DEF_LVL1_LARGE

/** @brief Holds Bit Buffer information*/
struct BitBuf2 {
	uint64_t m_bits;	//!< bits in the bit buffer
	uint32_t m_bit_count;	//!< number of valid bits in the bit buffer
	uint8_t *m_out_buf;	//!< current index of buffer to write to
	uint8_t *m_out_end;	//!< end of buffer to write to
	uint8_t *m_out_start;	//!< start of buffer to write to
};

/* Variable prefixes:
 * b_ : Measured wrt the start of the buffer
 * f_ : Measured wrt the start of the file (aka file_start)
 */

/** @brief Holds the internal state information for input and output compression streams*/
struct isal_zstate {
	uint32_t b_bytes_valid;	//!< number of bytes of valid data in buffer
	uint32_t b_bytes_processed;	//!< keeps track of the number of bytes processed in isal_zstate.buffer
	uint8_t *file_start;	//!< pointer to where file would logically start
	uint32_t crc;		//!< Current crc
	struct BitBuf2 bitbuf;	//!< Bit Buffer
	enum isal_zstate_state state;	//!< Current state in processing the data stream
	uint32_t count;	//!< used for partial header/trailer writes
	uint8_t tmp_out_buff[16];	//!< temporary array
	uint32_t tmp_out_start;	//!< temporary variable
	uint32_t tmp_out_end;	//!< temporary variable
	uint32_t has_eob;	//!< keeps track of eob on the last deflate block
	uint32_t has_eob_hdr;	//!< keeps track of eob hdr (with BFINAL set)
	uint32_t has_hist;	//!< flag to track if there is match history

	struct isal_mod_hist hist;

	DECLARE_ALIGNED(uint8_t buffer[2 * IGZIP_HIST_SIZE + ISAL_LOOK_AHEAD], 32);	//!< Internal buffer
	DECLARE_ALIGNED(uint16_t head[IGZIP_HASH_SIZE], 16);	//!< Hash array

};

/** @brief Holds the huffman tree used to huffman encode the input stream **/
struct isal_hufftables {

	uint8_t deflate_hdr[ISAL_DEF_MAX_HDR_SIZE]; //!< deflate huffman tree header
	uint32_t deflate_hdr_count; //!< Number of whole bytes in deflate_huff_hdr
	uint32_t deflate_hdr_extra_bits; //!< Number of bits in the partial byte in header
	uint32_t dist_table[IGZIP_DIST_TABLE_SIZE]; //!< bits 4:0 are the code length, bits 31:5 are the code
	uint32_t len_table[IGZIP_LEN_TABLE_SIZE]; //!< bits 4:0 are the code length, bits 31:5 are the code
	uint16_t lit_table[IGZIP_LIT_TABLE_SIZE]; //!< literal code
	uint8_t lit_table_sizes[IGZIP_LIT_TABLE_SIZE]; //!< literal code length
	uint16_t dcodes[30 - IGZIP_DECODE_OFFSET]; //!< distance code
	uint8_t dcodes_sizes[30 - IGZIP_DECODE_OFFSET]; //!< distance code length

};

/** @brief Holds stream information*/
struct isal_zstream {
	uint8_t *next_in;	//!< Next input byte
	uint32_t avail_in;	//!< number of bytes available at next_in
	uint32_t total_in;	//!< total number of bytes read so far

	uint8_t *next_out;	//!< Next output byte
	uint32_t avail_out;	//!< number of bytes available at next_out
	uint32_t total_out;	//!< total number of bytes written so far

	struct isal_hufftables *hufftables; //!< Huffman encoding used when compressing
	uint32_t level; //!< Compression level to use
	uint32_t level_buf_size; //!< Size of level_buf
	uint8_t * level_buf; //!< User allocated buffer required for different compression levels
	uint32_t end_of_stream;	//!< non-zero if this is the last input buffer
	uint32_t flush;	//!< Flush type can be NO_FLUSH, SYNC_FLUSH or FULL_FLUSH
	uint32_t gzip_flag; //!< Indicate if gzip compression is to be performed

	struct isal_zstate internal_state;	//!< Internal state for this stream
};

/******************************************************************************/
/* Inflate structures */
/******************************************************************************/
/*
 * Inflate_huff_code data structures are used to store a Huffman code for fast
 * lookup. It works by performing a lookup in small_code_lookup that hopefully
 * yields the correct symbol. Otherwise a lookup into long_code_lookup is
 * performed to find the correct symbol. The details of how this works follows:
 *
 * Let i be some index into small_code_lookup and let e be the associated
 * element.  Bit 15 in e is a flag. If bit 15 is not set, then index i contains
 * a Huffman code for a symbol which has length at most DECODE_LOOKUP_SIZE. Bits
 * 0 through 8 are the symbol associated with that code and bits 9 through 12 of
 * e represent the number of bits in the code. If bit 15 is set, the i
 * corresponds to the first DECODE_LOOKUP_SIZE bits of a Huffman code which has
 * length longer than DECODE_LOOKUP_SIZE. In this case, bits 0 through 8
 * represent an offset into long_code_lookup table and bits 9 through 12
 * represent the maximum length of a Huffman code starting with the bits in the
 * index i. The offset into long_code_lookup is for an array associated with all
 * codes which start with the bits in i.
 *
 * The elements of long_code_lookup are in the same format as small_code_lookup,
 * except bit 15 is never set. Let i be a number made up of DECODE_LOOKUP_SIZE
 * bits.  Then all Huffman codes which start with DECODE_LOOKUP_SIZE bits are
 * stored in an array starting at index h in long_code_lookup. This index h is
 * stored in bits 0 through 9 at index i in small_code_lookup. The index j is an
 * index of this array if the number of bits contained in j and i is the number
 * of bits in the longest huff_code starting with the bits of i. The symbol
 * stored at index j is the symbol whose huffcode can be found in (j <<
 * DECODE_LOOKUP_SIZE) | i. Note these arrays will be stored sorted in order of
 * maximum Huffman code length.
 *
 * The following are explanations for sizes of the tables:
 *
 * Since small_code_lookup is a lookup on DECODE_LOOKUP_SIZE bits, it must have
 * size 2^DECODE_LOOKUP_SIZE.
 *
 * Since deflate Huffman are stored such that the code size and the code value
 * form an increasing function, At most 2^(15 - DECODE_LOOKUP_SIZE) - 1 elements
 * of long_code_lookup duplicate an existing symbol. Since there are at most 285
 * - DECODE_LOOKUP_SIZE possible symbols contained in long_code lookup. Rounding
 * this to the nearest 16 byte boundary yields the size of long_code_lookup of
 * 288 + 2^(15 - DECODE_LOOKUP_SIZE).
 *
 * Note that DECODE_LOOKUP_SIZE can be any length even though the offset in
 * small_lookup_code is 9 bits long because the increasing relationship between
 * code length and code value forces the maximum offset to be less than 288.
 */

/* Large lookup table for decoding huffman codes */
struct inflate_huff_code_large {
	uint16_t short_code_lookup[1 << (ISAL_DECODE_LONG_BITS)];
	uint16_t long_code_lookup[288 + (1 << (15 - ISAL_DECODE_LONG_BITS))];
};

/* Small lookup table for decoding huffman codes */
struct inflate_huff_code_small {
	uint16_t short_code_lookup[1 << (ISAL_DECODE_SHORT_BITS)];
	uint16_t long_code_lookup[32 + (1 << (15 - ISAL_DECODE_SHORT_BITS))];
};

/** @brief Holds decompression state information*/
struct inflate_state {
	uint8_t *next_out;	//!< Next output Byte
	uint32_t avail_out;	//!< Number of bytes available at next_out 
	uint32_t total_out;	//!< Total bytes written out so far
	uint8_t *next_in;	//!< Next input byte
	uint64_t read_in;	//!< Bits buffered to handle unaligned streams
	uint32_t avail_in;	//!< Number of bytes available at next_in
	int32_t read_in_length;	//!< Bits in read_in
	struct inflate_huff_code_large lit_huff_code;	//!< Structure for decoding lit/len symbols
	struct inflate_huff_code_small dist_huff_code;	//!< Structure for decoding dist symbols
	enum isal_block_state block_state;	//!< Current decompression state
	uint32_t bfinal;	//!< Flag identifying final block
	uint32_t crc_flag;	//!< Flag identifying whether to track of crc
	uint32_t crc;		//!< Contains crc of output if crc_flag is set
	int32_t type0_block_len;	//!< Length left to read of type 0 block when outbuffer overflow occured
	int32_t copy_overflow_length; 	//!< Length left to copy when outbuffer overflow occured
	int32_t copy_overflow_distance;	//!< Lookback distance when outbuffer overlow occured
	int32_t tmp_in_size;	//!< Number of bytes in tmp_in_buffer
	int32_t tmp_out_valid;	//!< Number of bytes in tmp_out_buffer
	int32_t tmp_out_processed;	//!< Number of bytes processed in tmp_out_buffer
	uint8_t tmp_in_buffer[ISAL_DEF_MAX_HDR_SIZE];	//!< Temporary buffer containing data from the input stream
	uint8_t tmp_out_buffer[2 * ISAL_DEF_HIST_SIZE + ISAL_LOOK_AHEAD]; 	//!< Temporary buffer containing data from the output stream
};

/******************************************************************************/
/* Compression functions */
/******************************************************************************/
/**
 * @brief Updates histograms to include the symbols found in the input
 * stream. Since this function only updates the histograms, it can be called on
 * multiple streams to get a histogram better representing the desired data
 * set. When first using histogram it must be initialized by zeroing the
 * structure.
 *
 * @param in_stream: Input stream of data.
 * @param length: The length of start_stream.
 * @param histogram: The returned histogram of lit/len/dist symbols.
 */
void isal_update_histogram(uint8_t * in_stream, int length, struct isal_huff_histogram * histogram);


/**
 * @brief Creates a custom huffman code for the given histograms in which
 *  every literal and repeat length is assigned a code and all possible lookback
 *  distances are assigned a code.
 *
 * @param hufftables: the output structure containing the huffman code
 * @param histogram: histogram containing frequency of literal symbols,
 *        repeat lengths and lookback distances
 * @returns Returns a non zero value if an invalid huffman code was created.
 */
int isal_create_hufftables(struct isal_hufftables * hufftables,
			struct isal_huff_histogram * histogram);

/**
 * @brief Creates a custom huffman code for the given histograms like
 * isal_create_hufftables() except literals with 0 frequency in the histogram
 * are not assigned a code
 *
 * @param hufftables: the output structure containing the huffman code
 * @param histogram: histogram containing frequency of literal symbols,
 *        repeat lengths and lookback distances
 * @returns Returns a non zero value if an invalid huffman code was created.
 */
int isal_create_hufftables_subset(struct isal_hufftables * hufftables,
				struct isal_huff_histogram * histogram);

/**
 * @brief Initialize compression stream data structure
 *
 * @param stream Structure holding state information on the compression streams.
 * @returns none
 */
void isal_deflate_init(struct isal_zstream *stream);

/**
 * @brief Set stream to use a new Huffman code
 *
 * Sets the Huffman code to be used in compression before compression start or
 * after the sucessful completion of a SYNC_FLUSH or FULL_FLUSH. If type has
 * value IGZIP_HUFFTABLE_DEFAULT, the stream is set to use the default Huffman
 * code. If type has value IGZIP_HUFFTABLE_STATIC, the stream is set to use the
 * deflate standard static Huffman code, or if type has value
 * IGZIP_HUFFTABLE_CUSTOM, the stream is set to sue the isal_hufftables
 * structure input to isal_deflate_set_hufftables.
 *
 * @param stream: Structure holding state information on the compression stream.
 * @param hufftables: new huffman code to use if type is set to
 * IGZIP_HUFFTABLE_CUSTOM.
 * @param type: Flag specifying what hufftable to use.
 *
 * @returns Returns INVALID_OPERATION if the stream was unmodified. This may be
 * due to the stream being in a state where changing the huffman code is not
 * allowed or an invalid input is provided.
 */
int isal_deflate_set_hufftables(struct isal_zstream *stream,
				struct isal_hufftables *hufftables, int type);

/**
 * @brief Initialize compression stream data structure
 *
 * @param stream Structure holding state information on the compression streams.
 * @returns none
 */
void isal_deflate_stateless_init(struct isal_zstream *stream);


/**
 * @brief Fast data (deflate) compression for storage applications.
 *
 * The call to isal_deflate() will take data from the input buffer (updating
 * next_in, avail_in and write a compressed stream to the output buffer
 * (updating next_out and avail_out). The function returns when either the input
 * buffer is empty or the output buffer is full.
 *
 * On entry to isal_deflate(), next_in points to an input buffer and avail_in
 * indicates the length of that buffer. Similarly next_out points to an empty
 * output buffer and avail_out indicates the size of that buffer.
 *
 * The fields total_in and total_out start at 0 and are updated by
 * isal_deflate(). These reflect the total number of bytes read or written so far.
 *
 * When the last input buffer is passed in, signaled by setting the
 * end_of_stream, the routine will complete compression at the end of the input
 * buffer, as long as the output buffer is big enough.
 *
 * The compression level can be set by setting level to any value between
 * ISAL_DEF_MIN_LEVEL and ISAL_DEF_MAX_LEVEL. When the compression level is
 * ISAL_DEF_MIN_LEVEL, hufftables can be set to a table trained for the the
 * specific data type being compressed to achieve better compression. When a
 * higher compression level is desired, a larger generic memory buffer needs to
 * be supplied by setting level_buf and level_buf_size to represent the chunk of
 * memory. For level x, the suggest size for this buffer this buffer is
 * ISAL_DEFL_LVLx_DEFAULT. The defines ISAL_DEFL_LVLx_MIN, ISAL_DEFL_LVLx_SMALL,
 * ISAL_DEFL_LVLx_MEDIUM, ISAL_DEFL_LVLx_LARGE, and ISAL_DEFL_LVLx_EXTRA_LARGE
 * are also provided as other suggested sizes.
 *
 * The equivalent of the zlib FLUSH_SYNC operation is currently supported.
 * Flush types can be NO_FLUSH, SYNC_FLUSH or FULL_FLUSH. Default flush type is
 * NO_FLUSH. A SYNC_ OR FULL_ flush will byte align the deflate block by
 * appending an empty stored block once all input has been compressed, including
 * the buffered input. Checking that the out_buffer is not empty or that
 * internal_state.state = ZSTATE_NEW_HDR is sufficient to guarantee all input
 * has been flushed. Additionally FULL_FLUSH will ensure look back history does
 * not include previous blocks so new blocks are fully independent. Switching
 * between flush types is supported.
 *
 * If the gzip_flag is set to IGZIP_GZIP, a generic gzip header and the gzip
 * trailer are written around the deflate compressed data. If gzip_flag is set
 * to IGZIP_GZIP_NO_HDR, then only the gzip trailer is written.
 *
 * @param  stream Structure holding state information on the compression streams.
 * @return COMP_OK (if everything is ok),
 *         INVALID_FLUSH (if an invalid FLUSH is selected),
 *         ISAL_INVALID_LEVEL (if an invalid compression level is selected).
 */
int isal_deflate(struct isal_zstream *stream);


/**
 * @brief Fast data (deflate) stateless compression for storage applications.
 *
 * Stateless (one shot) compression routine with a similar interface to
 * isal_deflate() but operates on entire input buffer at one time. Parameter
 * avail_out must be large enough to fit the entire compressed output. Max
 * expansion is limited to the input size plus the header size of a stored/raw
 * block.
 *
 * When the compression level is set to 1, unlike in isal_deflate(), level_buf
 * may be optionally set depending on what what permormance is desired.
 *
 * For stateless the flush types NO_FLUSH and FULL_FLUSH are supported.
 * FULL_FLUSH will byte align the output deflate block so additional blocks can
 * be easily appended.
 *
 * If the gzip_flag is set to IGZIP_GZIP, a generic gzip header and the gzip
 * trailer are written around the deflate compressed data. If gzip_flag is set
 * to IGZIP_GZIP_NO_HDR, then only the gzip trailer is written.
 *
 * @param  stream Structure holding state information on the compression streams.
 * @return COMP_OK (if everything is ok),
 *         INVALID_FLUSH (if an invalid FLUSH is selected),
 *         ISAL_INVALID_LEVEL (if an invalid compression level is selected),
 *         STATELESS_OVERFLOW (if output buffer will not fit output).
 */
int isal_deflate_stateless(struct isal_zstream *stream);


/******************************************************************************/
/* Inflate functions */
/******************************************************************************/
/**
 * @brief Initialize decompression state data structure
 *
 * @param state Structure holding state information on the compression streams.
 * @returns none
 */
void isal_inflate_init(struct inflate_state *state);

/**
 * @brief Fast data (deflate) decompression for storage applications.
 *
 * On entry to isal_inflate(), next_in points to an input buffer and avail_in
 * indicates the length of that buffer. Similarly next_out points to an empty
 * output buffer and avail_out indicates the size of that buffer.
 *
 * The field total_out starts at 0 and is updated by isal_inflate(). This
 * reflects the total number of bytes written so far.
 *
 * The call to isal_inflate() will take data from the input buffer (updating
 * next_in, avail_in and write a decompressed stream to the output buffer
 * (updating next_out and avail_out). The function returns when the input buffer
 * is empty, the output buffer is full or invalid data is found. The current
 * state of the decompression on exit can be read from state->block-state. If
 * the crc_flag is set, the gzip crc of the output is stored in state->crc.
 *
 * @param  state Structure holding state information on the compression streams.
 * @return ISAL_DECOMP_OK (if everything is ok),
 *         ISAL_END_INPUT (if all input was decompressed),
 *         ISAL_OUT_OVERFLOW (if output buffer ran out of space),
 *         ISAL_INVALID_BLOCK,
 *         ISAL_INVALID_SYMBOL,
 *         ISAL_INVALID_LOOKBACK.
 */
int isal_inflate(struct inflate_state *state);

/**
 * @brief Fast data (deflate) stateless decompression for storage applications.
 *
 * Stateless (one shot) decompression routine with a similar interface to
 * isal_inflate() but operates on entire input buffer at one time. Parameter
 * avail_out must be large enough to fit the entire decompressed output.
 *
 * @param  state Structure holding state information on the compression streams.
 * @return ISAL_DECOMP_OK (if everything is ok),
 *         ISAL_END_INPUT (if all input was decompressed),
 *         ISAL_OUT_OVERFLOW (if output buffer ran out of space),
 *         ISAL_INVALID_BLOCK,
 *         ISAL_INVALID_SYMBOL,
 *         ISAL_INVALID_LOOKBACK.
 */
int isal_inflate_stateless(struct inflate_state *state);

#ifdef __cplusplus
}
#endif
#endif	/* ifndef _IGZIP_H */