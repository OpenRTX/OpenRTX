BEGIN {
    BINMODE = "r"
    infile = ARGV[1]
    ARGV[1] = ""

    if(length(infile)==0) {
        print "\n" \
              "bin2c - (C) 2026 by Niccolò Izzo\n" \
              "        License: WTFPL (wikipedia.org/wiki/WTFPL)\n" \
              "\n" \
              "usage: gawk -b -f this_file input_binary_file\n" \
              "\n" \
              "Convert the binary data of a file into a hex form.\n" 
        exit(3)
    }

    ####################################################################################################
    # From "The GNU Awk User's Guide" paragraph: "Translating Between Characters and Numbers" (adapted)
    ####################################################################################################

    for(i=0; i<256; i++)
        _ord_[sprintf("%c", i)] = i

    RS="a sequence that is SURELY not found in any input file, \x00 not even in this file"

    getline < infile # read the whole input file into $0

    len = length($0)

    bytes_x_line = 12
    outfile = infile
    gsub(getext(infile), ".h", outfile)

    # Print header
    printf("unsigned char %s[] = {\n", basename(infile)) > outfile
    for(i=0; i<len; i+=bytes_x_line) {
        printf("   ") > outfile
        # printf("0x%08x ", i)
        for(j=i; j<i+bytes_x_line; j++) {
            if(j==len)
                break
            # +1 is needed because the 1st char in a string is at position 1
            printf(" 0x%02x,", _ord_[substr($0, j+1, 1)]) > outfile
            # _ord_(char) converts a character to its ASCII code
            # same as 'ord(char)' in Python, or Asc(c) in Excel/VBA.
        }
        print "" > outfile
    }
    printf("};\n\n") > outfile
    printf("unsigned int %s_len = %d;\n", basename(infile), len) > outfile
}
# Extract basename of file
function basename(filename,     a) {
    # Get file name
    split(filename, a, "/")
    filename = a[length(a)]
    # Get without extension
    split(filename, a, ".")
    return a[1]
}
# Extract file extension
function getext(filename,      a) {
    split(filename, a, ".")
    return "." a[length(a)]
}
