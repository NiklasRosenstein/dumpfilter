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

#include "charbuffer.h"

#include <string.h>

charbuffer_t* charbuffer_alloc(size_t bufsize) {
    if (bufsize <= 0) {
        return NULL;
    }

    charbuffer_t* buffer = allocate(sizeof(charbuffer_t));
    if (buffer) {
        buffer->mem = allocate(bufsize + 1);
        if (!buffer->mem) {
            deallocate(buffer);
            return NULL;
        }

        buffer->size = bufsize;
        buffer->filled = 0;
        buffer->next = NULL;
    }
    return buffer;
}

void charbuffer_free(charbuffer_t* buffer) {
    while (buffer) {
        charbuffer_t* next = buffer->next;
        deallocate(buffer->mem);
        deallocate(buffer);
        buffer = next;
    }
}

charbuffer_t* charbuffer_append_char(charbuffer_t* buffer, char c) {
    buffer = charbuffer_prepare_append(buffer);
    if (buffer) {
        buffer->mem[buffer->filled++] = c;
        buffer->mem[buffer->filled] = 0;
    }
    return buffer;
}

charbuffer_t* charbuffer_append_string(
        charbuffer_t* buffer, const char* s) {
    while (*s) {
        buffer = charbuffer_append_char(buffer, *s++);
        if (!buffer) {
            return NULL;
        }
    }
    return buffer;
}

charbuffer_t* charbuffer_append_buffer(
        charbuffer_t* buffer, const char* b, size_t size) {
    size_t i;
    for (i=0; i < size && buffer; i++) {
        buffer = charbuffer_append_char(buffer, b[i]);
    }
    return buffer;
}

charbuffer_t* charbuffer_append_charbuffer(
        charbuffer_t* buffer, charbuffer_t* source, size_t offset) {
    while (buffer && source) {
        if (offset <= source->filled) {
            buffer = charbuffer_append_buffer(
                    buffer, source->mem + offset, source->filled - offset);
        }
        else {
            offset -= source->filled;
        }
        source = source->next;
    }
    return buffer;
}

charbuffer_t* charbuffer_prepare_append(charbuffer_t* buffer) {
    charbuffer_t* init_buffer = buffer;
    while (buffer && buffer->filled >= buffer->size) {
        if (!buffer->next) {
            buffer->next = charbuffer_alloc(init_buffer->size);
        }
        buffer = buffer->next;
    }
    return buffer;
}

bool charbuffer_contains_string(charbuffer_t* buffer, const char* string,
        charbuffer_t** outPtr, size_t* outOffset) {
    if (!buffer || !string) {
        return false;
    }

    size_t length = strlen(string);
    if (length == 0) {
        return true;
    }

    return charbuffer_contains_buffer(buffer, string, length,
                                      outPtr, outOffset);
}

bool charbuffer_contains_buffer(
        charbuffer_t* buffer, const char* cbuf, size_t size,
        charbuffer_t** outPtr, size_t* outOffset) {
    if (!buffer || !cbuf) {
        return false;
    }

    size_t matched = 0;
    size_t offset = -1;

    while (buffer && matched < size) {
        size_t i;
        for (i=0; i < buffer->filled && matched < size; i++) {
            if (cbuf[matched] == buffer->mem[i]) {
                if (matched == 0) {
                    offset = i;
                }
                matched++;
            }
            else {
                matched = 0;
            }
        }

        if (matched == size) {
            if (outPtr) {
                *outPtr = buffer;
            }
            if (outOffset) {
                *outOffset = offset;
            }
            break;
        }
        buffer = buffer->next;
    }

    return matched == size;
}

bool charbuffer_to_buffer(charbuffer_t* buffer, char* mem, size_t max) {
    uint64_t n_filled = 0;
    uint64_t n_bytes;
    bool result = true;
    while (buffer && result) {
        n_bytes = buffer->filled;
        if (n_filled + n_bytes > max) {
            n_bytes = max - n_filled;
            result = false;
        }
        memcpy(mem + n_filled, buffer->mem, n_bytes);
        buffer = buffer->next;
        n_filled += n_bytes;
    }
    return result;
}

bool charbuffer_to_file(
        charbuffer_t* buffer, FILE* fp, uint64_t* outBytes) {
    if (!buffer || !fp) {
        return false;
    }

    if (outBytes) {
        *outBytes = 0;
    }
    while (buffer) {
        size_t written = fwrite(buffer->mem, 1, buffer->filled, fp);
        if (outBytes) {
            *outBytes += written;
        }
        if(written != buffer->filled) {
            // writing error
            return false;
        }
        buffer = buffer->next;
    }
    return true;
}

char* charbuffer_to_string(charbuffer_t* buffer, void* (*alloc)(size_t)) {
    uint64_t length = charbuffer_length(buffer);
    char* string = alloc(length + 1);
    if (string) {
        string[length] = 0;
        charbuffer_to_buffer(buffer, string, length);
    }
    return string;
}

int charbuffer_print(charbuffer_t* buffer) {
    uint64_t bytesWritten = 0;
    charbuffer_to_file(buffer, stdout, &bytesWritten);
    return bytesWritten;
}

bool charbuffer_truncate(charbuffer_t* buffer, size_t truncate) {
    if (!buffer) {
        return false;
    }
    if (truncate == 0) {
        if (buffer->next) {
            charbuffer_free(buffer->next);
            buffer->next = NULL;
        }
        return true;
    }
    else {
        size_t has_bytes = 0;
        charbuffer_t* init_buffer = buffer;
        while (buffer) {
            if (has_bytes >= truncate) {
                return charbuffer_truncate(buffer, 0);
            }
            has_bytes += buffer->size;
            buffer = buffer->next;
        }

        while (buffer && has_bytes < truncate) {
            buffer->next = charbuffer_alloc(init_buffer->size);
            buffer = buffer->next;
            has_bytes += buffer->size;
        }

        return buffer != NULL;
    }
}

uint64_t charbuffer_potential(charbuffer_t* buffer) {
    uint64_t sum = 0;
    while (buffer) {
        sum += (uint64_t) buffer->size;
        buffer = buffer->next;
    }
    return sum;
}

void charbuffer_flush(charbuffer_t* buffer) {
    while (buffer) {
        buffer->filled = 0;
        buffer = buffer->next;
    }
}

uint64_t charbuffer_length(charbuffer_t* buffer) {
    uint64_t length = 0;
    while (buffer) {
        length += (uint64_t) buffer->filled;
        buffer = buffer->next;
    }
    return length;
}

