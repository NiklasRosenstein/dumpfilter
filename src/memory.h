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

#ifndef MEMORY_H__
#define MEMORY_H__

    #include <stdlib.h>
    #include <stdio.h>

    #ifdef DEBUG

        struct _memory_node {
            size_t size;
            size_t line;
            const char* filename;
            struct _memory_node* prev;
            struct _memory_node* next;

            // additonal memory
        };

        typedef struct _memory_node _memory_node_t;

        extern _memory_node_t* _shiney_memory_start;

        void* _allocate(size_t size, size_t line, const char* filename);

        void _deallocate(void* ptr, size_t line, const char* filename);

        #define allocate(size) _allocate((size), __LINE__, __FILE__)

        #define deallocate(ptr) _deallocate((ptr), __LINE__, __FILE__)

    #else

        void* allocate(size_t size);
        void deallocate(void* ptr);

    #endif /* DEBUG */

    void memory_info(FILE* fp);

#endif /* MEMORY_H__ */
