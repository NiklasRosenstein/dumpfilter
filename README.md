# dumpfilter

C-Program to find occurences of search terms in dump files that are
cluttered with your and other eventually binary data. Useful if you
shreddered your recent work and need it back.

    Usage: dumpfilter [options] dumpfile search-terms
    Options:
      -o <filename>        Write printable sections matching the search
                           term to this file. If not given, stdout will
                           be used instead.
      -a <bytes>           The number of unprintable bytes allowed between
                           two printable sections.
      -b <bytes>           The buffer-size used internally. Default value
                           is 1024.
      -s <bytes>           The number of bytes to skip from the beginning
                           of the input file.
      -m <bytes>           The maximum size of a result-chunk.If the value
                           is zero, no maximum is set. Default is zero.
                           search term and fulfills all other criteria.
      -c <bytes>           The minimum size a printable sub-chunk must
                           have. Defaults to 0.
      -v                   Be verbose about the actual input information
                           and stop processing afterwards.
      -u <bytes>           Only process until this anmount of bytes have
                           been passed.
      -w                   Do not treat whitespaces as printables.

    <bytes> arguments can be a simple mathematical expression. No spaces
    are allowed and the operators are +, -, * and /. The additional
    operators are k (= *1000), m(= *1000^2), K (= *1024) and M (= *1000^2)

    For example, to achieve 1Mb and 100 bytes, the expression    1M0+100
    can be used. Note that the expression does not follow mathematical
    rules such as operator precendence.
