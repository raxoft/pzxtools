#! /usr/bin/perl -wT
#
# Convert sequences of long pulses to PAUSE blocks.

use Getopt::Long;

my $duration_limit = 2500 ;
my $count_limit = 1000 ;

die "usage: $0 [-d duration_limit] [-c count_limit]"
unless GetOptions(
    'duration|d=i'  =>  \$duration_limit,
    'count|c=i'     =>  \$count_limit,
) ;

my @pulses = () ;

my $sum_duration = 0 ;
my $pause_duration = 0 ;

while(<>) {

    die "expand pulses with expand_pulses.pl or pzx2txt -e\n" if /^PULSE\s+\d+\s+\d+/i ;

    if ( my( $duration ) = /^PULSE\s+(\d+)/i ) {
        if ( $duration < $pulse_limit ) {
            push( @pulses, $duration ) ;
            $sum_duration += $duration ;
            next ;
        }
        $pause_duration += process_pulses( @pulses, $sum_duration ) ;
        $pause_duration += $duration ;
    }

    if ( $pause_duration > 0 ) {
        print "\n" ;
        print "PAUSE $pause_duration\n" ;
        print "\n" ;
        print "PULSES\n" ;
        print "PULSE 0\n" unless $level ;
        $pause_duration = 0 ;
    }

    print ;
}

if ( $pause_duration > 0 ) {
    print "\n" ;
    print "PAUSE $pause_duration\n" ;
}
