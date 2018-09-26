#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include "task.h"
#include "helpers.h"
#include "desktop.h"





desktop_t init_desktop(void){
	desktop_t desktop;
	desktop.cardinal = -1;
	desktop.name = NULL;
	desktop.state = UNKNOWN_STATE_DESKTOP;
	desktop.number_of_tasks = 0;
	desktop.tasks = NULL;
	return desktop;
}

desktop_t *create_desktop_list(void){
	desktop_t *desktops = (desktop_t*)malloc(MAX_DESKTOP_NBR*sizeof(desktop_t));
	if (desktops == NULL) return NULL;
	for (int i = 0; i < MAX_DESKTOP_NBR; i++){
		desktops[i] = init_desktop();
	}
	return desktops;
}

void assign_tasks_to_desktop(task_t *task_list, int nbr_of_tasks, desktop_t *desktop_list, int nbr_of_desktops){
	int tmp_desktop = -1;
	task_t tmp_task_list[MAX_DESKTOP_NBR][MAX_TASK_NBR];
	for (int j = 0; j < nbr_of_desktops; j++){
		desktop_list[j].number_of_tasks = 0;
	}
	for (int i = 0; i < nbr_of_tasks; i++){
		tmp_desktop = task_list[i].desktop_cardinal;
		if (tmp_desktop != -1){
			for (int j = 0; j <  nbr_of_desktops; j++){
				if (desktop_list[j].cardinal == tmp_desktop){
					tmp_task_list[j][desktop_list[j].number_of_tasks++] = task_list[i];	
				}
			}
		}
	}
	for (int j = 0; j < nbr_of_desktops; j++){
		if (desktop_list[j].tasks != NULL){
			free(desktop_list[j].tasks);
		}
		
		desktop_list[j].tasks = (task_t*)malloc(desktop_list[j].number_of_tasks*sizeof(task_t));
		if (desktop_list[j].tasks == NULL) continue;
		for (int i = 0; i < desktop_list[j].number_of_tasks; i++){
			desktop_list[j].tasks[i] =  tmp_task_list[j][i];
		}
	}	
	
}
void desktop_task_number_change_event(task_t *task_list, int nbr_of_tasks, desktop_t * desktop_list, int nbr_of_desktops){
	assign_tasks_to_desktop(task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
}
void desktop_change_names_event(xcb_ewmh_connection_t * ewmh, int default_screen, desktop_t * desktop_list, int nbr_of_desktops){
	xcb_ewmh_get_utf8_strings_reply_t names;
	char tmp_arr[MAXLEN] = {0};
	int nlen = 0;
	for (int i=0; i < nbr_of_desktops; i++)
		if (xcb_ewmh_get_desktop_names_reply(ewmh, xcb_ewmh_get_desktop_names(ewmh, default_screen), &names, NULL) == 1){
			copy_prop(tmp_arr, names.strings, names.strings_len, i, nbr_of_desktops);
			nlen = strlen(tmp_arr);
			if (desktop_list[i].name != NULL) free(desktop_list[i].name);
			desktop_list[i].name = (char*)malloc(nlen+1);
			strncpy(desktop_list[i].name, tmp_arr, nlen+1);
			xcb_ewmh_get_utf8_strings_reply_wipe(&names);
		
		}
}
void desktop_focus_change_event(xcb_ewmh_connection_t * ewmh, int default_screen, desktop_t *desktop_list, int nbr_of_desktops){
	int focused_desktop;
	if (xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop(ewmh, default_screen), &focused_desktop, NULL) != 1){
			focused_desktop = -1;
		}
	printf("Focused : %d\n", focused_desktop);
	for (int i=0; i < nbr_of_desktops; i++){
		if (desktop_list[i].cardinal == focused_desktop){
			desktop_list[i].state = FOCUSED_DESKTOP;
		}
		if (desktop_list[i].cardinal != focused_desktop && desktop_list[i].state == FOCUSED_DESKTOP){
			if (desktop_list[i].number_of_tasks > 0)
				desktop_list[i].state = OCCUPIED_DESKTOP;
			else
				desktop_list[i].state = UNOCCUPIED_DESKTOP;
		}
	}
	
}
int desktop_number_change_event(xcb_ewmh_connection_t * ewmh, int default_screen, task_t *task_list, int nbr_of_tasks, desktop_t * desktop_list, int nbr_of_desktops){
	int tmp_nbr_of_desks = 0; int nlen = 0; int focused_desktop = -1;
	xcb_ewmh_get_utf8_strings_reply_t names;
	char tmp_arr[MAXLEN] = {0};
	if (xcb_ewmh_get_number_of_desktops_reply(ewmh, xcb_ewmh_get_number_of_desktops(ewmh, default_screen), &tmp_nbr_of_desks, NULL) != 1){
		fprintf(stderr, "Can't get current number of desktops!\n");
		for (int i = 0; i < nbr_of_desktops; i++){
			free(desktop_list[i].name); desktop_list[i].name = NULL;
			free(desktop_list[i].tasks); desktop_list[i].tasks = NULL;
		}
		return nbr_of_desktops;
	}
	for (int i = 0; i < nbr_of_desktops; i++){
			free(desktop_list[i].name); desktop_list[i].name = NULL;
			free(desktop_list[i].tasks); desktop_list[i].tasks = NULL;
	}
	nbr_of_desktops = tmp_nbr_of_desks;
	if (xcb_ewmh_get_desktop_names_reply(ewmh, xcb_ewmh_get_desktop_names(ewmh, default_screen), &names, NULL) == 1){
		for (int i = 0; i < nbr_of_desktops; i++){
			desktop_list[i].cardinal = i;
			copy_prop(tmp_arr, names.strings, names.strings_len, i, nbr_of_desktops);
			nlen = strlen(tmp_arr);
			
			if (desktop_list[i].name != NULL) free(desktop_list[i].name);
			desktop_list[i].name = (char*)malloc(nlen+1);
			strncpy(desktop_list[i].name, tmp_arr, nlen+1);
		}
		assign_tasks_to_desktop(task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
		if (xcb_ewmh_get_current_desktop_reply(ewmh, xcb_ewmh_get_current_desktop(ewmh, default_screen), &focused_desktop, NULL) != 1){
			focused_desktop = -1;
		}
		for (int j = 0; j < nbr_of_desktops; j++){
			if (desktop_list[j].cardinal == focused_desktop)
				desktop_list[j].state = FOCUSED_DESKTOP;
			else{
				if (desktop_list[j].number_of_tasks > 0)
					desktop_list[j].state = OCCUPIED_DESKTOP;
				else
					desktop_list[j].state = UNOCCUPIED_DESKTOP;
				
			}
			
		}
		xcb_ewmh_get_utf8_strings_reply_wipe(&names);
		return nbr_of_desktops;
	}
	for (int i = 0; i < nbr_of_desktops; i++){
		if (desktop_list[i].name != NULL)
			if (realloc(desktop_list[i].name, 3))
				strncpy(desktop_list[i].name, MISSING_VALUE, 3);
		else{
			desktop_list[i].name = malloc(3);
			if (desktop_list[i].name){
				strncpy(desktop_list[i].name, MISSING_VALUE, 3);
			}
		}
	}
	return nbr_of_desktops;
}
int update_desktop_list(xcb_ewmh_connection_t * ewmh, int default_screen, desktop_event_t event, task_t *task_list, int nbr_of_tasks, desktop_t *desktop_list, int nbr_of_desktops){
	if (event.event == TASK_NUMBER_CHANGE){
		desktop_task_number_change_event(task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
	}else 
	if (event.event == DESKTOP_NAMES_CHANGE){
		desktop_change_names_event(ewmh, default_screen, desktop_list, nbr_of_desktops);
	} else
	if (event.event == DESKTOP_FOCUS_CHANGE){
		desktop_focus_change_event(ewmh, default_screen, desktop_list, nbr_of_desktops);	
	}else
	if (event.event == DESKTOP_NUMBER_CHANGE){
		nbr_of_desktops = desktop_number_change_event(ewmh, default_screen, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
	}
	return nbr_of_desktops;
}
