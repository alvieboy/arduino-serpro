#!/usr/bin/perl

print<<EOM;
#ifndef __PREPROCESSOR_TABLE_H__
#define __PREPROCESSOR_TABLE_H__

EOM
print "#define EXPAND_NUMBER_1(x) EXPAND_VALUE(1,x) \n";

for ($i=2;$i<256;$i++) {
    print "#define EXPAND_NUMBER_$i(x) EXPAND_VALUE($i,x) EXPAND_DELIM EXPAND_NUMBER_".($i-1)."(x)\n";
}
print<<EOM;
#define DO_EXPAND(x) EXPAND_NUMBER_##x(x)

#endif
EOM
