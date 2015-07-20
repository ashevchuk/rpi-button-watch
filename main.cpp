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

#define WIRINGPI_CODES		1
#include <wiringPi.h>


#define DAEMON_NAME		"bwatch-daemon"
#define PID_FILE		"/var/run/" DAEMON_NAME ".pid"
#define PIN			4
#define PIN_STR			"4"

void daemon_stop(int signum);
void button_pressed(void);

int main(int argc, char *argv[]) {
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "Daemon starting up");

    if (geteuid() != 0) {
        syslog(LOG_ERR, "This daemon can only be run by root user, exiting");
        exit(EXIT_FAILURE);
    }

    struct stat filestat;
    if (stat("/usr/bin/gpio", &filestat) == -1) {
        syslog(LOG_ERR, "The program '/usr/bin/gpio' is missing, exiting");
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

    if (wiringPiSetup() == -1) {
       syslog(LOG_ERR, "'wiringPi' library couldn't be initialized, exiting");
       exit(EXIT_FAILURE);
    }

    pinMode(PIN, INPUT);
    pullUpDnControl(PIN, PUD_UP);

    if (wiringPiISR(PIN, INT_EDGE_FALLING, &button_pressed) == -1) {
       syslog(LOG_ERR, "Unable to set interrupt handler for specified pin, exiting");
       exit(EXIT_FAILURE);
    }

    while (true) {
	sleep(60);
    }
}

void daemon_stop(int signum) {
    syslog(LOG_INFO, "Stopping daemon");
    exit(EXIT_SUCCESS);
}

void button_pressed(void) {
//    system("/usr/bin/gpio edge " PIN_STR " none");

    sleep(2);

    switch (digitalRead(PIN)) {
	case HIGH:
	    syslog(LOG_INFO, "Short connection");
	    system("/usr/local/bin/bpush_short");
	    break;

	case LOW:
	    syslog(LOG_INFO, "Long connection");
	    system("/usr/local/bin/bpush_long");
	    break;
	}
}
