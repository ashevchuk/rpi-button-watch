#!/usr/bin/env perl

use lib 'lib';

use Device::PZEM004T;

my $powerd = new Device::PZEM004T("/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0");

my $voltage = $powerd->voltage;
my $current = $powerd->current;
my $power = $powerd->power;
my $energy = $powerd->energy;

printf "%sV %sA %sW %sWh\n", $voltage, $current, $power, $energy;
