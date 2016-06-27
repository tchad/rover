/*
 * main.cpp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Copyright (C) 2016 Tomasz Chadzynski
 */

/*
 * TODO:
 * - add filelock to prevent multiple instances
 * - reopen descriptors 0,1,2 as /dev/null
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>

#include "util.h"

volatile bool RUNNING;

void PrintUsage(FILE *s)
{
    fprintf(s,
            "Usage: \n"
            "   -d  --daemon    Start in daemon mode.\n"
            "   -h  --help      Print this message.\n");
}

void ExitSigHandler(int sig)
{
    RUNNING = false;
    syslog(LOG_NOTICE, "caught terminating signal, shutting down.\n");
}

void Run()
{
    RUNNING = true;

    if(signal(SIGINT, ExitSigHandler) == SIG_ERR){
        syslog(LOG_ERR, "Error registering SIGINT handler.\n");
        exit(EXIT_FAILURE);
    }

    if(signal(SIGTERM, ExitSigHandler) == SIG_ERR){
        syslog(LOG_ERR, "Error registering SIGTERM handler.\n");
        exit(EXIT_FAILURE);
    }

    if(signal(SIGHUP, ExitSigHandler) == SIG_ERR){
        syslog(LOG_ERR, "Error registering SIGHUP handler.\n");
        exit(EXIT_FAILURE);
    }

    if(signal(SIGQUIT, ExitSigHandler) == SIG_ERR){
        syslog(LOG_ERR, "Error registering SIGQUIT handler.\n");
        exit(EXIT_FAILURE);
    }

    while(RUNNING){
        sleep(1);
        syslog(LOG_NOTICE, "ping\n");
    }
    
}

void Daemonize()
{

    pid_t pid = fork();
    if(pid < 0) {
        fprintf(stderr, "%s unable to fork into daemon: %s\n", MAIN_NAME, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(pid > 0) {
        exit(EXIT_SUCCESS);
    }
    else {
        umask(0);

        openlog(MAIN_NAME, LOG_PID|LOG_NOWAIT,LOG_USER);
        syslog(LOG_NOTICE, "Starting daemon.\n");

        pid_t sid = setsid();
        if(sid < 0) {
            syslog(LOG_ERR, "Error creating process group.\n");
            exit(EXIT_FAILURE);
        }
        
        if(chdir("/") < 0) {
            syslog(LOG_ERR, "Error changing working directory to /.\n");
            exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        Run();

        closelog();
    }
        
}

void RunLocal()
{
        openlog(MAIN_NAME, LOG_PID|LOG_NOWAIT,LOG_USER);
        syslog(LOG_NOTICE, "Starting local.\n");

        Run();

        closelog();
}



int main(int argc, char *argv[])
{
    const char* const short_options = "dh";

    const struct option long_options[] = {
        { "help",   0,  NULL,  'h'},
        { "daemon", 0,  NULL,  'd'},
        { NULL,     0,  NULL,   0 }
    };

    char option;
    bool runAsDaemon = false;

    do{
        option = getopt_long(argc, argv, short_options, long_options, NULL);
        
        switch(option) {
            case 'h':
                PrintUsage(stdout);
                exit(EXIT_SUCCESS);
                break;
            case 'd':
                runAsDaemon = true;
                break;
            case -1:
                break;
            default:
                PrintUsage(stdout);
                abort();
        };
    }
    while (option != -1);

    if(runAsDaemon) {
        Daemonize();
    }
    else {
        RunLocal();
    }

    return EXIT_SUCCESS;
}
