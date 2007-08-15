#! /usr/bin/perl -w
#
# Filter the blocks of input through other scripts.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

use IPC::Open3 ;

my $pid ;

while(<>) {

    if ( $pid && /^SCRIPT/i ) {
        close PIPE ;
        waitpid( $pid, 0 ) ;
        $pid = 0 ;
    }

    if ( $pid ) {
        print PIPE ;
        next ;
    }

    if ( my( $script ) = /^SCRIPT\s*(.*?)\s*$/i ) {
        if ( $script ne "" ) {
            local $SIG{__WARN__} = sub {}  ;
            $pid = eval { return open3( \*PIPE, ">&1", ">&2", $script ) }
                or die "error: unable to run \"$script\"\n" ;
        }
        next ;
    }

    print ;
}

if ( $pid ) {
    close PIPE ;
    waitpid( $pid, 0 ) ;
}
