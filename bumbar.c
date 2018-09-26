#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "task.h"
#include "desktop.h"
#include "event.h"

#ifndef FIFO_NAME
#define FIFO_NAME "/tmp/bumbar_fifo"
#endif

static xcb_connection_t *dpy;
static xcb_screen_t *screen;
static int default_screen;
static xcb_window_t root;
static xcb_ewmh_connection_t *ewmh;
static int run;

static int DAEMON = 0;
void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        run = 0;
		
}
void setup(void)
{
	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy))
		err("Can't open display.\n");
	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL)
		err("Can't acquire screen.\n");
	root = screen->root;
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0)
		err("Can't initialize EWMH atoms.\n");
}



void print_output(desktop_t *desktop_list, int nbr_of_desktops){
	for (int i=0; i<nbr_of_desktops; i++){
		if (desktop_list[i].state == FOCUSED_DESKTOP)
			printf("F:Desktop %d: %s %d\n", desktop_list[i].cardinal, desktop_list[i].name, desktop_list[i].number_of_tasks);
		if (desktop_list[i].state == OCCUPIED_DESKTOP)
			printf("O:Desktop %d: %s %d\n", desktop_list[i].cardinal, desktop_list[i].name, desktop_list[i].number_of_tasks);
		if (desktop_list[i].state == UNOCCUPIED_DESKTOP)
			printf("U:Desktop %d: %s %d\n", desktop_list[i].cardinal, desktop_list[i].name, desktop_list[i].number_of_tasks);
		for (int j=0; j < desktop_list[i].number_of_tasks; j++){
			if (desktop_list[i].tasks[j].state == FOCUSED_TASK)
				printf("\tF: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
			if (desktop_list[i].tasks[j].state == VISIBLE_TASK)
				printf("\tV: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
			if (desktop_list[i].tasks[j].state == HIDDEN_TASK)
				printf("\tH: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
			if (desktop_list[i].tasks[j].state == UNKNOWN)
				printf("\tU: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
			
		}
	}
}

void print_output_to_fifo(desktop_t *desktop_list, int nbr_of_desktops){
	int fifo_fd = open(FIFO_NAME, O_WRONLY|O_NONBLOCK);
	char write_arr[(MAX_TASK_NBR+MAX_DESKTOP_NBR)*2*MAXLEN] = {0};
	char tmp_arr[2*MAXLEN] = {0};
	if (fifo_fd != -1){
		*write_arr = '\0';
		for (int i=0; i<nbr_of_desktops; i++){
			if (desktop_list[i].state == FOCUSED_DESKTOP)
				sprintf(tmp_arr, "F:Desktop %d: %s %d\n", desktop_list[i].cardinal, desktop_list[i].name, desktop_list[i].number_of_tasks);
			if (desktop_list[i].state == OCCUPIED_DESKTOP)
				sprintf(tmp_arr, "O:Desktop %d: %s %d\n", desktop_list[i].cardinal, desktop_list[i].name, desktop_list[i].number_of_tasks);
			if (desktop_list[i].state == UNOCCUPIED_DESKTOP)
				sprintf(tmp_arr, "U:Desktop %d: %s %d\n", desktop_list[i].cardinal, desktop_list[i].name, desktop_list[i].number_of_tasks);
			strcat(write_arr, tmp_arr);
			for (int j=0; j < desktop_list[i].number_of_tasks; j++){
				memset(tmp_arr, 0, 2*MAXLEN);
				if (desktop_list[i].tasks[j].state == FOCUSED_TASK)
					sprintf(tmp_arr, "\tF: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
				if (desktop_list[i].tasks[j].state == VISIBLE_TASK)
					sprintf(tmp_arr, "\tV: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
				if (desktop_list[i].tasks[j].state == HIDDEN_TASK)
					sprintf(tmp_arr, "\tH: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
				if (desktop_list[i].tasks[j].state == UNKNOWN)
					sprintf(tmp_arr, "\tU: win:%d, title: %s\n", desktop_list[i].tasks[j].win, desktop_list[i].tasks[j].title);
				strcat(write_arr, tmp_arr);
				
			}
		}
		write(fifo_fd, write_arr, strlen(write_arr)+1);
		close(fifo_fd);
	}
}

int cleanup(task_t *task_list, int nbr_of_tasks, desktop_t *desktop_list, int nbr_of_desktops){
	for (int i = 0; i < nbr_of_desktops; i++) {
		free(desktop_list[i].name);
		free(desktop_list[i].tasks);
	}
	free(desktop_list);
	for (int i = 0; i < nbr_of_tasks; i++) free(task_list[i].title);
	free(task_list);
	xcb_ewmh_connection_wipe(ewmh);
    	free(ewmh);
	xcb_disconnect(dpy);	
}
int make_fifo(void){
	if (access(FIFO_NAME, F_OK) == -1) {
		int res = mkfifo(FIFO_NAME, 0777);
		if (res != 0) {
		    return 1;
		}
    	}
	return 0;
}
void continuos_output( int deamonize, task_t *task_list, int nbr_of_tasks, desktop_t *desktop_list, int nbr_of_desktops){
	fd_set descriptors;xcb_generic_event_t *evt;int print=0;
	register_root_events(dpy, screen, &run);
	int dpy_fd = xcb_get_file_descriptor(dpy);
	int sel_fd = MAX(dpy_fd, -1) + 1;
	while (run){
		FD_ZERO(&descriptors);
		FD_SET(dpy_fd, &descriptors);
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(dpy_fd, &descriptors))
				while ((evt = xcb_poll_for_event(dpy)) != NULL) {
					print = event_handler(ewmh, default_screen, event_generator(ewmh, screen, evt), task_list, &nbr_of_tasks, desktop_list, &nbr_of_desktops);
					if (print==1){
						if (deamonize==1){						
							print_output_to_fifo(desktop_list, nbr_of_desktops);
						}
						else{
							print_output(desktop_list, nbr_of_desktops);
						}
				
					}
					free(evt);
				}
			}
	}	
	cleanup(task_list,  nbr_of_tasks, desktop_list, nbr_of_desktops);
		
}
void single_output(task_t *task_list, int nbr_of_tasks, desktop_t *desktop_list, int nbr_of_desktops){
	print_output(desktop_list, nbr_of_desktops);
	cleanup(task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
}
int main(int argc, char **argv){
	int deamonize = 0, continuos = 0;
	int desktops=0, tasks=0;
	if (argc == 2){
		if (strcmp(argv[1], "-c")==0) continuos = 1;
		if (strcmp(argv[1], "-D")==0) deamonize = 1;
	}
	setup();
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	task_t *task_list = create_task_list();
	desktop_t *desktop_list = create_desktop_list();
	task_event_t event;
	event.event = TASK_LIST_CHANGE;
	event.win = root;
	desktop_event_t devent;
	devent.event = DESKTOP_NUMBER_CHANGE;
	devent.win = root;
	tasks = update_task_list(ewmh, default_screen, event, task_list, tasks);
	desktops = update_desktop_list(ewmh, default_screen, devent, task_list, tasks, desktop_list, desktops);
	if (task_list == NULL){
		return 1;
	}
	if (deamonize == 1){
		if (make_fifo()) {
			printf("HERE");
			cleanup(task_list, tasks, desktop_list, desktops);
			return 1;
		}
		run = 1;
		daemon(1, 1);
		continuos_output(deamonize, task_list, tasks, desktop_list, desktops);
	}
	if (continuos == 1){
		run = 1;
		continuos_output(deamonize, task_list, tasks, desktop_list, desktops);
	}
	if (deamonize != 1 && continuos != 1) single_output( task_list, tasks, desktop_list, desktops);
	return 0;
}
int main_o(int argc, char **argv){
	int deamonize = 0, continuos = 0;
	if (argc == 2){
		if (strcmp(argv[1], "-c")) continuos = 1;
		if (strcmp(argv[1], "-D")) deamonize = 1;
	}
	setup();
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	task_t *task_list = create_task_list();
	if (task_list == NULL){
		printf("HERE!");
		return 1;
	}
	int nbr_of_tasks = 0;
	int res;
	task_event_t event;
	event.event = TASK_LIST_CHANGE;
	event.win = root;
	nbr_of_tasks = update_task_list(ewmh, default_screen, event, task_list, nbr_of_tasks);
	desktop_t *desktop_list = create_desktop_list();
	if (desktop_list == NULL){
		printf("HERE!");
		return 1;
	}
	desktop_event_t devent;
	devent.event = DESKTOP_NUMBER_CHANGE;
	devent.win = root;
	int nbr_of_desktops = update_desktop_list(ewmh, default_screen, devent, task_list, nbr_of_tasks, desktop_list, 0);
	xcb_generic_event_t *evt;
	run = 1;
	int print = 0;
	register_root_events(dpy, screen, &run);
	int dpy_fd = xcb_get_file_descriptor(dpy);
	int sel_fd = MAX(dpy_fd, -1) + 1;
	fd_set descriptors;
	if (DAEMON){
		if (access(FIFO_NAME, F_OK) == -1) {
			res = mkfifo(FIFO_NAME, 0777);
			if (res != 0) {
			    fprintf(stderr, "Could not create fifo %s\n", FIFO_NAME);
			    exit(EXIT_FAILURE);
			}
    		}
		daemon(1, 0);
	}
	while (run){
		FD_ZERO(&descriptors);
		FD_SET(dpy_fd, &descriptors);
		if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
			if (FD_ISSET(dpy_fd, &descriptors))
				while ((evt = xcb_poll_for_event(dpy)) != NULL) {
					print = event_handler(ewmh, default_screen, event_generator(ewmh, screen, evt), task_list, &nbr_of_tasks, desktop_list, &nbr_of_desktops);
					if (print==1){
						if (DAEMON){						
							print_output_to_fifo(desktop_list, nbr_of_desktops);
						}
						else{
							print_output(desktop_list, nbr_of_desktops);
						}
				
					}
					free(evt);
				}
			}
	}
	for (int i = 0; i < nbr_of_desktops; i++) {
		free(desktop_list[i].name);
		free(desktop_list[i].tasks);
	}
	free(desktop_list);
	for (int i = 0; i < nbr_of_tasks; i++) free(task_list[i].title);
	free(task_list);
	xcb_ewmh_connection_wipe(ewmh);
    	free(ewmh);
	xcb_disconnect(dpy);
	return 0;
}
