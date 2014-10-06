/* Copyright (c) 2014  Niklas Rosenstein
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. */

#ifndef CHARBUFFER_H__
#define CHARBUFFER_H__

    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>
    #include <stdio.h>

    #include "memory.h"

    /**
     * Structure implementing a dynamic and mutable string type.
     */
    struct charbuffer {
        char* mem;
        size_t size;
        size_t filled;
        struct charbuffer* next;
    };

    typedef struct charbuffer charbuffer_t;

    /**
     * Allocate a new charbuffer with the passed buffer size. Returns
     * NULL on failure. The buffer size depends on your needs, but any
     * value higher than 256 bytes makes much sense. For large datasets,
     * even values of 1024 or 2048 are useful.
     */
    charbuffer_t* charbuffer_alloc(size_t bufsize);

    /**
     * Free the passed charbuffer object and all its neighbor
     * buffers.
     */
    void charbuffer_free(charbuffer_t* buffer);

    /**
     * Append a single character to the charbuffer. This function
     * returns the last node in the buffer chain that was filled with
     * the character. If the return value is NULL, an error occured
     * (eg. memory allocation for a new node failed).
     */
    charbuffer_t* charbuffer_append_char(charbuffer_t* buffer, char c);

    /**
     * Append a C-string to the charbuffer. Returns the last node in
     * the buffer chain that was filled with characters. If the return
     * value is NULL, an error occured (eg. memory allocation for a
     * new node failed).
     */
    charbuffer_t* charbuffer_append_string(
            charbuffer_t* buffer, const char* s);

    /**
     * Effectively append a character buffer with a fixed size. Returns
     * the last charbuffer node a character was appended to.
     */
    charbuffer_t* charbuffer_append_buffer(
            charbuffer_t* buffer, const char* b, size_t size);

    /**
     * Append the contents of a charbuffer to another charbuffer. The
     * content is copied! One can re-use the original buffer.
     */
    charbuffer_t* charbuffer_append_charbuffer(
            charbuffer_t* buffer, charbuffer_t* source, size_t offset);

    /**
     * Return the last buffer node in the chain or allocate a new
     * node if the last node is full. Returns NULL on failure. If a
     * new node needs to be alloacted, it will be allocated with the
     * buffer size of the **first** node.
     */
    charbuffer_t* charbuffer_prepare_append(charbuffer_t* buffer);

    /**
     * Returns true if the charbuffer contains the passed string.
     * If *outPtr* and *outOffset* are not null pointers, they will
     * be assigned the information to retrieve the first occurence
     * of the string in the charbuffer.
     */
    bool charbuffer_contains_string(
            charbuffer_t* buffer, const char* string,
            charbuffer_t** ptr, size_t* offset);

    /**
     * Returns true if the charbuffer contains the passed memory.
     * If *outPtr* and *outOffset* are not null pointers, they will
     * be assigned the information to retrieve the first occurence
     * of the memory block in the charbuffer.
     */
    bool charbuffer_contains_buffer(
            charbuffer_t* buffer, const char* cbuf, size_t size,
            charbuffer_t** outPtr, size_t* outOffset);

    /**
     * Fill the memory with the joined contents of the charbuffer
     * nodes, resulting in one linear representation (without breaks)
     * of the data stored in the nodes. Returns true if the data did
     * fit into passed memory, false if it did not. No null-terminator
     * will be set!
     */
    bool charbuffer_to_buffer(charbuffer_t* buffer, char* mem, size_t max);

    /**
     * Put the contents of a charbuffer to a file. True if the data
     * was written successfully. If *outBytes* is not a null pointer,
     * it will be filled with the number of writes actually written to
     * the file.
     */
    bool charbuffer_to_file(
            charbuffer_t* buffer, FILE* fp, uint64_t* outBytes);

    /**
     * Convert the charbuffer to a string with null-terminator. The
     * resulting string may be invalid if zero has been copied to the
     * charbuffer before via `charbuffer_append_char()`.
     */
    char* charbuffer_to_string(charbuffer_t* buffer, void* (*alloc)(size_t));

    /**
     * Print the contents of the charbuffer to the stdout. Returns
     * the number of bytes written.
     */
    int charbuffer_print(charbuffer_t* buffer);

    /**
     * Flush the contents of the charbuffer. If the *truncate* argument
     * is not zero, the charbuffer chain will be truncated so that at least
     * the number of bytes passed with *truncate* will fit into the buffer.
     * If the buffer chain is not long enough, it will be expanded to hold
     * at least *truncate* bytes.
     * Returns false on a memory error or if *buffer* is a null pointer.
     */
    bool charbuffer_truncate(charbuffer_t* buffer, size_t truncate);

    /**
     * Returns the number of bytes the charbuffer was capable of
     * holding.
     */
    uint64_t charbuffer_potential(charbuffer_t* buffer);

    /**
     * Flush the passed buffer completely.
     */
    void charbuffer_flush(charbuffer_t* buffer);

    /**
     * Get the totoal length of the charbuffer chain.
     */
    uint64_t charbuffer_length(charbuffer_t* buffer);

#endif /* CHARBUFFER_H__ */
