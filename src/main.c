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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include "charbuffer.h"

struct program_args {
    char** argv;
    const char* program;

    char** searchTerms;
    size_t searchTermCount;

    uint64_t nUnprintablesAllowed;
    uint64_t resultMaxSize;
    uint64_t minChunkSize;
    uint64_t nSkipBytes;
    uint64_t nUntil;
    bool treatWhitespacesPrintable;

    const char* inFilePath;
    const char* outFilePath;
    FILE* inFile;
    FILE* outFile;

    size_t bufSize;
    bool verbose;
} args = {0};

int usage() {
    printf("Usage: %s [options] dumpfile search-terms\n", args.argv[0]);
    printf(
        "Options:\n"
        "  -o <filename>        Write printable sections matching the search\n"
        "                       term to this file. If not given, stdout will\n"
        "                       be used instead.\n"
        "  -a <bytes>           The number of unprintable bytes allowed between\n"
        "                       two printable sections.\n"
        "  -b <bytes>           The buffer-size used internally. Default value\n"
        "                       is 1024.\n"
        "  -s <bytes>           The number of bytes to skip from the beginning\n"
        "                       of the input file.\n"
        "  -m <bytes>           The maximum size of a result-chunk.If the value\n"
        "                       is zero, no maximum is set. Default is zero.\n"
        "                       search term and fulfills all other criteria.\n"
        "  -c <bytes>           The minimum size a printable sub-chunk must\n"
        "                       have. Defaults to 0.\n"
        "  -v                   Be verbose about the actual input information\n"
        "                       and stop processing afterwards.\n"
        "  -u <bytes>           Only process until this anmount of bytes have\n"
        "                       been passed.\n"
        "  -w                   Do not treat whitespaces as printables.\n"
        "\n"
        "<bytes> arguments can be a simple mathematical expression. No spaces\n"
        "are allowed and the operators are +, -, * and /. The additional\n"
        "operators are k (= *1000), m(= *1000^2), K (= *1024) and M (= *1000^2)\n"
        "\n"
        "For example, to achieve 1Mb and 100 bytes, the expression    1M0+100\n"
        "can be used. Note that the expression does not follow mathematical\n"
        "rules such as operator precendence.\n"
        "\n"
    );
    return EINVAL;
}

int memory_error() {
    printf("Memory error.\n");
    return ENOMEM;
}

bool is_printable(int i) {
    if (args.treatWhitespacesPrintable) {
        switch (i) {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                return true;
        }
    }
    return isprint(i);
}

uint64_t parsellu(char* string) {
    char* endptr = NULL;
    uint64_t value = strtoull(string, &endptr, 10);
    if (*endptr) {
        // Retrieve the value multiplier.
        long long mul = 0;
        switch (*endptr++) {
            case 'k':
                mul = 1000;
                break;
            case 'K':
                mul = 1024;
                break;
            case 'm':
                mul = 1000L * 1000L;
                break;
            case 'M':
                mul = 1024L * 1024L;
                break;
            default:
                endptr--;
                break;
        }
        if (mul) {
            value *= mul;
        }

        // Catch the operator.
        char op = 0;
        switch (*endptr) {
            case '+':
            case '-':
            case '*':
            case '/':
                op = *endptr++;
                break;
        }

        if (*endptr && op) {
            uint64_t right = parsellu(endptr);
            switch (op) {
                case '+':
                    value += right;
                    break;
                case '-':
                    value -= right;
                    break;
                case '*':
                    value *= right;
                    break;
                case '/':
                    if (right != 0)
                        value /= right;
                    break;
            }
        }
    }

    return value;
}

bool chunk_accepted(charbuffer_t* buffer, size_t byteOffset) {
    bool matches = false;

    // Check all search terms if they are contained in the buffer. If
    // at least one of the terms is included, the chunk will be output.
    size_t i;
    for (i=0; i < args.searchTermCount; i++) {
        if (charbuffer_contains_string(buffer, args.searchTerms[i],
                NULL, NULL)) {
            matches = true;
            break;
        }
    }

    if (matches) {
        // Its a printable section and contains the search term.
        fprintf(args.outFile, "%llu\n", byteOffset);
        fprintf(args.outFile, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

        charbuffer_to_file(buffer, args.outFile, NULL);
        fprintf(args.outFile, "\n\n");
    }
    return matches;
}

int scan_file(FILE* fp) {
    charbuffer_t* printable = charbuffer_alloc(args.bufSize);
    if (!printable) {
        return memory_error();
    }
    charbuffer_t* unprintable = charbuffer_alloc(args.bufSize);
    if (!unprintable) {
        charbuffer_free(printable);
        return memory_error();
    }

    size_t chunkSkip = 1024 * 4;
    uint64_t skipped = 0;
    while (skipped < args.nSkipBytes) {
        uint64_t offset;
        if (skipped + chunkSkip > args.nSkipBytes) {
            offset = args.nSkipBytes - skipped;
        }
        else {
            offset = chunkSkip;
        }
        // TODO: Seeking over 2GB fails.
        int res = fseek(fp, offset, SEEK_CUR);
        if (res != 0) {
            fprintf(stderr, "Could not skip %llu bytes, file may be "
                    "too small. Result: %d\n", args.nSkipBytes, res);
            charbuffer_free(printable);
            charbuffer_free(unprintable);
            return ECANCELED;
        }
        skipped += offset;
    }

    charbuffer_t* printableOpt = printable;
    charbuffer_t* unprintableOpt = unprintable;
    uint64_t printableCount = 0;
    uint64_t unprintableCount = 0;

    // For output, the Mb that have previously been printed out.
    uint64_t prevPrint = 0;
    uint64_t bytesPassed = skipped;

    // The maximum chunk size that was printable, added to the complete
    // printable buffer.
    uint64_t maxChunkSize = 0;
    uint64_t currChunkSize = 0;

    uint64_t bytesPrint = 0;

    // Go through the complete file and search for printable sections.
    char buffer[args.bufSize];
    size_t bytes;
    bool prevPrintable = false;
    while (printableOpt && unprintableOpt) {
        bytes = fread(buffer, 1, args.bufSize, fp);
        if (bytes <= 0) {
            break;
        }

        int i;
        for (i=0; i < bytes; i++) {
            bool isPrintable = is_printable(buffer[i]);
            if (isPrintable && (args.resultMaxSize == 0 || printableCount <= args.resultMaxSize)) {
                if (!prevPrintable) {
                    currChunkSize = 0;
                }

                // Append the unprintable characters since they are
                // allowed due to `args.nUnprintablesAllowed`.
                printableOpt = charbuffer_append_charbuffer(printable, unprintable, 0);
                charbuffer_flush(unprintable);
                unprintableOpt = unprintable;
                unprintableCount = 0;

                // Append the current character.
                printableOpt = charbuffer_append_char(printableOpt, buffer[i]);
                printableCount++;

                currChunkSize++;
                prevPrintable = true;
            }
            else if (unprintableCount > args.nUnprintablesAllowed) {
                // The number of unprintable character was exceeded.

                if (currChunkSize > maxChunkSize) {
                    maxChunkSize = currChunkSize;
                }

                bool accepted = args.resultMaxSize == 0;
                accepted |= printableCount <= args.resultMaxSize;
                accepted &= maxChunkSize >= args.minChunkSize;
                if (accepted) {
                    if (chunk_accepted(printable, bytesPassed + i)) // TODO: Remove this line
                        fprintf(stderr, ">> Matched with block of %llu max chars.\n", maxChunkSize);
                }

                charbuffer_flush(printable);
                charbuffer_flush(unprintable);
                printableCount = 0;
                unprintableCount = 0;
                printableOpt = printable;
                unprintableOpt = unprintable;
                maxChunkSize = 0;
                currChunkSize = 0;
            }
            else {
                unprintableOpt = charbuffer_append_char(unprintableOpt, buffer[i]);
                unprintableCount++;
            }
            prevPrintable = isPrintable;
        }

        bytesPassed += bytes;

        uint64_t newBytesPrint = bytesPassed / 1024 / 1024 / 10;
        if (newBytesPrint != bytesPrint) {
            fprintf(stderr, "Passed %lluM bytes.\n", newBytesPrint * 10);
            bytesPrint = newBytesPrint;
        }

        if (args.nUntil != 0 && bytesPassed > args.nUntil) {
            break;
        }
    }

    charbuffer_free(printable);
    charbuffer_free(unprintable);
    return 0;
}

int main(int argc, char** argv) {
    args.argv = argv;
    args.program = argv[0];
    args.bufSize = 1024;
    args.treatWhitespacesPrintable = true;

    // Parse the command-line arguments.
    char c;
    while ((c = getopt(argc, argv, "o:a:b:m:c:s:u:whv")) != -1) {
        switch (c) {
        case 'o':
            if (args.outFilePath) {
                printf("-o: multiple parameters are not allowed.\n\n");
                return usage();
            }
            args.outFilePath = optarg;
            break;
        case 'a':
            args.nUnprintablesAllowed = parsellu(optarg);
            break;
        case 'b':
            args.bufSize = parsellu(optarg);
            if (args.bufSize < 128) {
                printf("-b: buffer size must be greater than 128 bytes.\n\n");
                return usage();
            }
        case 'm':
            args.resultMaxSize = parsellu(optarg);
            if (args.resultMaxSize < 128 && args.resultMaxSize != 0) {
                printf("-m: must be >= 128 or 0.\n\n");
                return usage();
            }
        case 'c':
            args.minChunkSize = parsellu(optarg);
            if (args.minChunkSize < 0) {
                printf("-x: must be > 0.\n\n");
                return usage();
            }
            break;
        case 's':
            args.nSkipBytes = parsellu(optarg);
            if (args.nSkipBytes < 0) {
                printf("-s: must be > 0\n\n");
                return usage();
            }
            break;
        case 'v':
            args.verbose = true;
            break;
        case 'w':
            args.treatWhitespacesPrintable = false;
            break;
        case 'u':
            args.nUntil = parsellu(optarg);
            break;
        case '?':
        case 'h':
        default:
            return usage();
        }
    }

    // Now skip the already parsed arguments.
    argc -= optind;
    argv += optind;

    if (argc < 1) {
        printf("%s: no input file\n", args.program);
        return EINVAL;
    }

    args.inFilePath = argv[0];
    argc--;
    argv++;

    args.inFile = fopen(args.inFilePath, "rb");
    if (!args.inFile) {
        printf("%s: could not open input file %s\n", args.program, args.inFilePath);
        return ENOENT;
    }

    if (argc < 1) {
        printf("%s: no search terms\n", args.program);
        return EINVAL;
    }

    args.searchTerms = argv;
    args.searchTermCount = argc;

    if (args.verbose) {
        fprintf(stderr, "Input File:             %s\n", args.inFilePath);
        fprintf(stderr, "Output file:            %s\n", (args.outFilePath ? args.outFilePath : "stdout"));
        fprintf(stderr, "unprintables allowed:   %llu\n", args.nUnprintablesAllowed);
        fprintf(stderr, "Buffer size:            %llu\n", args.bufSize);
        fprintf(stderr, "Bytes to skip:          %llu\n", args.nSkipBytes);
        fprintf(stderr, "Max chunk-size:         %llu\n", args.resultMaxSize);
        fprintf(stderr, "Min Sub-chunk size:     %llu\n", args.minChunkSize);
        fprintf(stderr, "Wspace as printables:   %s\n", (args.treatWhitespacesPrintable ? "Yes" : "No"));
        fprintf(stderr, "Search Terms:\n");
        int i;
        for (i=0; i < args.searchTermCount; i++) {
            fprintf(stderr, " |  %s\n", args.searchTerms[i]);
        }
        fprintf(stderr, "\n");
    }

    // Open the output file.
    if (args.outFilePath) {
        args.outFile = fopen(args.outFilePath, "wb");
        if (!args.outFile) {
            printf("-o: File %s could not be opened.\n", optarg);
            return ENOENT;
        }
    }
    else {
        args.outFile = stdout;
    }

    int result = scan_file(args.inFile);
    memory_info(stderr);

    if (args.verbose) {
        printf("scan_file() result: %d\n", result);
    }
    return result;
}

