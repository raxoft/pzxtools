#! /usr/bin/perl -wT
#
# Delete any PAUSE blocks.

while(<>) {
    next if /^PAUSE\s+/ ;
    print ;
}
