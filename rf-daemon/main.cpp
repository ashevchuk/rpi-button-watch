#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>

#include <RF24/RF24.h>

using namespace std;

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

const uint64_t pipes[2] = { 0xFAF0F0F0E1LL, 0xFAF0F0F0D2LL };

unsigned long  got_message;

const int min_payload_size = 4;
const int max_payload_size = 254;

//static char send_payload[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ789012";

char receive_payload[max_payload_size+1]; // +1 to allow room for a terminating NULL char

#define DAEMON_NAME		"rf-daemon"
#define PID_FILE		"/var/run/" DAEMON_NAME ".pid"

void daemon_stop(int signum);

void rf_setup(void){
	radio.begin();

	radio.setRetries(15, 15);
	radio.setPALevel(RF24_PA_MAX);
	radio.setDataRate(RF24_250KBPS);

	radio.enableDynamicPayloads();

	radio.printDetails();

	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1,pipes[1]);
}

bool sendMessage(char message[], int len){
	radio.stopListening();
	bool ok = radio.write( message, len);
	if (!ok){
		printf("failed...\n\r");
	}else{
		printf("ok!\n\r");
	}	

	//Listen for ACK
	radio.startListening();

	//Let's take the time while we listen
	unsigned long started_waiting_at = millis();
	bool timeout = false;
	while ( ! radio.available() && ! timeout ) {
		if (millis() - started_waiting_at > 5000 ){
			timeout = true;
		}
	}

	if( timeout ){
		printf("It's not giving me any response...\n\r");
		return false;
	}else{
		//If we received the message in time, let's read it and print it
		uint8_t rlen = radio.getDynamicPayloadSize();
		radio.read(receive_payload, rlen);
		receive_payload[rlen] = 0;
//		radio.read(&receive_payload, sizeof(unsigned long) );
		printf("Got this response length %d.\n\r", rlen);
		printf("Got this response message %s.\n\r", receive_payload);

		const char* app = "/usr/local/bin/rfaction ";

		char command[254];
		uint8_t command_len = strlen(app)+strlen(receive_payload)+1;
		strcpy(command, app);
		strcat(command, receive_payload);
		command[command_len] = 0;

		printf("Got this response message %s.\n\r", command);

		FILE *fp;
		char var[254];
		fp = popen(command, "r");

		radio.stopListening();

		while (fgets(var, sizeof(var), fp) != NULL) {
    		    printf("%s", var);
    		    radio.write(var, sizeof(var));
		}
		pclose(fp);

		radio.startListening();

		return true;
	}
}

int setup_daemon(void) {
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "Daemon starting up");

    if (geteuid() != 0) {
        syslog(LOG_ERR, "This daemon can only be run by root user, exiting");
        exit(EXIT_FAILURE);
    }

    const int PIDFILE_PERMISSION = 0644;
    int pidFilehandle = open(PID_FILE, O_RDWR | O_CREAT, PIDFILE_PERMISSION);

    if (pidFilehandle == -1) {
	syslog(LOG_ERR, "Could not open PID lock file %s, exiting", PID_FILE);
        exit(EXIT_FAILURE);
    }

    if (lockf(pidFilehandle, F_TLOCK, 0) == -1) {
	syslog(LOG_ERR, "Could not lock PID lock file %s, exiting", PID_FILE);
	exit(EXIT_FAILURE);
    }

    pid_t pid, sid;

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    char szPID[16];
    sprintf(szPID, "%d\n", getpid());

    write(pidFilehandle, szPID, strlen(szPID));

    umask(0);

    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);

    if (chdir("/") < 0) exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGTERM, &daemon_stop);

    return 1;
}

bool read_messages(void) {
    if ( radio.available() ) {
	uint8_t rlen = radio.getDynamicPayloadSize();
	radio.read(receive_payload, rlen);
	receive_payload[rlen] = 0;

	printf("Got this response length %d.\n\r", rlen);
	printf("Got this response message %s.\n\r", receive_payload);

	syslog(LOG_INFO, receive_payload);

	const char* app = "/usr/local/bin/rfaction ";

	char command[254];
	uint8_t command_len = strlen(app)+strlen(receive_payload)+1;
	strcpy(command, app);
	strcat(command, receive_payload);
	command[command_len] = 0;

	printf("Got this response message %s.\n\r", command);
	syslog(LOG_INFO, command);

	FILE *fp;
	char var[254];
	fp = popen(command, "r");

	radio.stopListening();

	while (fgets(var, sizeof(var), fp) != NULL) {
	    printf("%s", var);
	    syslog(LOG_INFO, var);
	    radio.write(var, sizeof(var));
	}
	pclose(fp);

	radio.startListening();

	return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    setup_daemon();

    rf_setup();

    radio.startListening();

    while (true) {
	while ( radio.available() ) {
	    read_messages();
	}
	sleep(2);
//	sendMessage(send_payload, 16);
    }
}

void daemon_stop(int signum) {
    syslog(LOG_INFO, "Stopping daemon");
    exit(EXIT_SUCCESS);
}
