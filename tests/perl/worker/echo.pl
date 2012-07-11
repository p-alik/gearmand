#!/usr/bin/perl 
#===============================================================================
#
#         FILE: echo.pl
#
#        USAGE: ./echo.pl  
#
#  DESCRIPTION: Echo back the workload.
#
#      OPTIONS: ---
# REQUIREMENTS: ---
#         BUGS: ---
#        NOTES: ---
#       AUTHOR: YOUR NAME (), 
# ORGANIZATION: 
#      VERSION: 1.0
#      CREATED: 07/11/2012 04:02:42 PM
#     REVISION: ---
#===============================================================================

use strict;
use warnings;

my $host = 'localhost';
my $port = '4730';

my $servers = $host . ':' . $port;

print "Connecting to $servers\n";
use Gearman::Worker;
my $worker = Gearman::Worker->new;
$worker->job_servers($servers);
$worker->register_function(sum => sub { return $_[0] });
$worker->work while 1;
