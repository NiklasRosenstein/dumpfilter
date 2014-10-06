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

#include "memory.h"

#ifdef DEBUG

    _memory_node_t* _shiney_memory_start = NULL;

    void* _allocate(size_t size, size_t line, const char* filename) {
        size_t newSize = sizeof(_memory_node_t) + size;
        _memory_node_t* node = malloc(newSize);
        if (!node) {
            fprintf(stderr, "%s:%lu Failed to allocate %lu bytes.\n",
                    filename, (unsigned long) line, (unsigned long) size);
            return NULL;
        }

        node->size = size;
        node->line = line;
        node->filename = filename;
        node->prev = NULL;
        node->next = _shiney_memory_start;
        if (_shiney_memory_start) {
            _shiney_memory_start->prev = node;
        }
        _shiney_memory_start = node;

        return ((char*) node) + sizeof(_memory_node_t);
    }

    void _deallocate(void* ptr, size_t line, const char* filename) {
        _memory_node_t* node = (void*) (((char*) ptr) - sizeof(_memory_node_t));

        // Search for the node in the allocated nodes list.
        _memory_node_t* current = _shiney_memory_start;
        while (current && current != node) {
            current = current->next;
        }

        if (!current) {
            fprintf(stderr, "%s:%lu Attempt to deallocate memory block not "
                    "allocated with shiney_alloc().\n", filename,
                    (unsigned long) line);
            return;
        }

        if (current->next) {
            current->next->prev = current->prev;
        }
        if (current->prev) {
            current->prev->next = current->next;
        }

        if (current == _shiney_memory_start) {
            _shiney_memory_start = current->next;
        }

        free(current);
    }

    void memory_info(FILE* fp) {
        _memory_node_t* node = _shiney_memory_start;
        while (node) {
            fprintf(fp, "%s:%lu (%lu bytes)\n", node->filename,
                    (unsigned long) node->line, (unsigned long) node->size);
            node = node->next;
        }
    }

#else

    void* allocate(size_t size) {
        return malloc(size);
    }

    void deallocate(void* ptr) {
        free(ptr);
    }

    void memory_info(FILE* fp) {
    }

#endif /* DEBUG */
