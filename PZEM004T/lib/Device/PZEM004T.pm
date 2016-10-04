package Device::PZEM004T;

use strict;
use warnings;

our $VERSION = 1.000_002;

use Device::SerialPort;

our %COMMAND = (
    set_address		=> (join '', map { chr $_ } (0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB4)),
    read_current	=> (join '', map { chr $_ } (0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB1)),
    read_energy		=> (join '', map { chr $_ } (0xB3, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB3)),
    read_voltage	=> (join '', map { chr $_ } (0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0)),
    read_power		=> (join '', map { chr $_ } (0xB2, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB2))
);

sub new {
    my $class = shift;
    my $port  = shift;

    my $self  = { };

    $self->{timeout} = 1000;

    $self->{port} = Device::SerialPort->new($port) || die "Can't open $port: $!\n";

    $self->{port}->baudrate(9600);
    $self->{port}->databits(8);
    $self->{port}->parity("none");
    $self->{port}->stopbits(1);
    $self->{port}->datatype("raw");

    $self->{port}->purge_all;

    #while ( $self->{port}->read(7) ) {}

    bless($self, $class);

    $self->write_command($COMMAND{set_address});
    $self->read_result;

    return $self;
}

sub read_result {
    my $self = shift;

    my $buff = '';

    my $timeout = $self->{timeout} || 1000;

    while (length $buff < 7) {
	my ($c, $char)= $self->{port}->read(1);
	if ($char) {
#	    print "Received character: |";
#	    printf ("0x%lX", ord $char);
#	    print "|\n";
	    $buff .= $char;
	}

	last unless --$timeout;
    }

    return $buff;
}

sub write_command {
    my $self = shift;
    my $command = shift;

    $self->{port}->write($command);
    $self->{port}->write_drain;
#    sleep(1);
    select (undef, undef, undef, 0.4);
}

sub destroy {
    my $self = shift;

    $self->{port}->close;
}

sub voltage {
    my $self = shift;

    $self->write_command($COMMAND{read_voltage});
    my $result = $self->read_result();

    my @octets = map { ord $_ } split //, $result;

    if ($octets[0] == 0xA0 || $octets[0] == 0x20) {
	my $voltage = ($octets[1] << 8) + $octets[2] + ($octets[3] / 10.0);
	return $voltage;
    }

    return undef;
}

sub current {
    my $self = shift;

    $self->write_command($COMMAND{read_current});
    my $result = $self->read_result();

    my @octets = map { ord $_ } split //, $result;

    if ($octets[0] == 0xA1 || $octets[0] == 0x21) {
	my $current = ($octets[1] << 8) + $octets[2] + ($octets[3] / 100.0);
	return $current;
    }

    return undef;
}

sub energy {
    my $self = shift;

    $self->write_command($COMMAND{read_energy});
    my $result = $self->read_result();

    my @octets = map { ord $_ } split //, $result;

    if ($octets[0] == 0xA3 || $octets[0] == 0x23) {
	my $energy = ($octets[1] << 16) + ($octets[2] << 8) + $octets[3];
	return $energy;
    }

    return undef;
}

sub power {
    my $self = shift;

    $self->write_command($COMMAND{read_power});
    my $result = $self->read_result();

    my @octets = map { ord $_ } split //, $result;

    if ($octets[0] == 0xA2 || $octets[0] == 0x22) {
	my $power = ($octets[1] << 8) + $octets[2];
	return $power;
    }

    return undef;
}

1;
