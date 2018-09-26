#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "task.h"


size_t get_window_title(xcb_ewmh_connection_t *ewmh, xcb_window_t win, char *title, size_t len) {
	xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
	ewmh_txt_prop.strings = NULL;
	title[0] = '\0';
	if (xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, win), &ewmh_txt_prop, NULL) == 1) {
		char *src = NULL;
		size_t title_len = 0;
		
		if (ewmh_txt_prop.strings != NULL) {
			src = ewmh_txt_prop.strings;
			title_len = MIN(len, ewmh_txt_prop.strings_len);
		}
		
		if (src != NULL) {
			strncpy(title, src, title_len);
			title[title_len] = '\0';
			xcb_ewmh_get_utf8_strings_reply_wipe(&ewmh_txt_prop);
			return title_len;
		}
	}
    return 0;
}


task_state_t get_task_state(xcb_ewmh_connection_t * ewmh, int default_screen, xcb_window_t win){
	xcb_ewmh_get_atoms_reply_t ewmh_atoms_reply;
	xcb_window_t curr_win;
	if (xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, default_screen), &curr_win, NULL) == 1){
		if (win == curr_win) return FOCUSED_TASK;
	}
	if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &ewmh_atoms_reply, NULL ) == 1){
		if (ewmh_atoms_reply.atoms_len > 0){
			for (int i=0; i < ewmh_atoms_reply.atoms_len;i++){
				if (ewmh_atoms_reply.atoms[i] == ewmh ->_NET_WM_STATE_HIDDEN) {
					xcb_ewmh_get_atoms_reply_wipe(&ewmh_atoms_reply);
					return HIDDEN_TASK;
				}
			}
		}
		xcb_ewmh_get_atoms_reply_wipe(&ewmh_atoms_reply);
		
	}
	return VISIBLE_TASK;
}
task_t get_task_t(xcb_ewmh_connection_t * ewmh, int default_screen, xcb_window_t win){
	task_t task; char tmp_arr[MAXLEN] = {0}; size_t tit_len = 0;
	task.win = win;
	if ((tit_len = get_window_title(ewmh, win, tmp_arr, MAXLEN)) > 0){
		task.title = (char*)malloc(tit_len+1);
		memset(task.title, 0, tit_len+1);
		strncpy(task.title, tmp_arr, tit_len+1);
	}
	else{
		task.title = (char*)malloc(3);
		memset(task.title, 0, 3);
		strncpy(task.title, MISSING_VALUE, 2);
		task.title[3] = '\0';
	}
	task.state = get_task_state(ewmh, default_screen, win);
	task.desktop_cardinal = get_task_desktop(ewmh, win);
	return task;
}
int get_task_desktop(xcb_ewmh_connection_t *ewmh, xcb_window_t win){
	uint32_t desktop;
	if (xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop(ewmh, win), &desktop, NULL) == 1) {
		return desktop;
	}
	return -1;
   
}
int task_list_change_event(xcb_ewmh_connection_t * ewmh, int default_screen, task_t *task_list){
	int nbr_of_tasks = 0;
	for (int i = 0; i < MAX_TASK_NBR; i++) 
		if (task_list[i].title != NULL){
			free(task_list[i].title);
			task_list[i].title = NULL;
		}
	xcb_ewmh_get_windows_reply_t win_reply;xcb_window_t *wins = NULL;
	if (xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list(ewmh, default_screen), 
										&win_reply, NULL)  == 1) {
		nbr_of_tasks = win_reply.windows_len;
		wins = win_reply.windows;
		if (wins != NULL)
			for (int i = 0; i < nbr_of_tasks; i++) {
				if (task_list[i].title != NULL) free(task_list[i].title);
				task_list[i] = get_task_t(ewmh, default_screen, wins[i]);
				register_win_events(ewmh->connection, wins[i]);
		}
		xcb_ewmh_get_windows_reply_wipe(&win_reply);
	}
	return nbr_of_tasks;
}
void task_name_change_event(xcb_ewmh_connection_t * ewmh, task_t *task_list, int nbr_of_tasks, xcb_window_t win){
	char tmp_arr[MAXLEN] = {0};
	int tit_len = get_window_title(ewmh, win, tmp_arr, MAXLEN);
	for (int i = 0; i < nbr_of_tasks; i++){
		if (task_list[i].win == win){
			if (task_list[i].title != NULL) free(task_list[i].title);
			if (tit_len > 0){
				task_list[i].title = (char*)malloc(tit_len+1);
				memset(task_list[i].title, 0, tit_len+1);
			    	strncpy(task_list[i].title, tmp_arr, tit_len+1);	
			}else{
				task_list[i].title = (char*)malloc(3);
				memset(task_list[i].title, 0, 3);
				strncpy(task_list[i].title, MISSING_VALUE, 2);
				task_list[i].title[3] = '\0';
			}
		}
	}
}
void task_state_change_event(xcb_ewmh_connection_t * ewmh, task_t *task_list, int nbr_of_tasks, xcb_window_t win){
	xcb_ewmh_get_atoms_reply_t ewmh_atoms_reply;
	if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &ewmh_atoms_reply, NULL ) == 1){
		if (ewmh_atoms_reply.atoms_len > 0){
			for (int i=0; i < ewmh_atoms_reply.atoms_len;i++){
				if (ewmh_atoms_reply.atoms[i] == ewmh ->_NET_WM_STATE_HIDDEN) {
					for (int i = 0; i < nbr_of_tasks; i++) if (task_list[i].win == win) task_list[i].state = HIDDEN_TASK;
					}
				
			}
				
		}
		else{
			for (int i = 0; i < nbr_of_tasks; i++) if (task_list[i].win == win) task_list[i].state = VISIBLE_TASK;
			
		}
		xcb_ewmh_get_atoms_reply_wipe(&ewmh_atoms_reply);
	}
}

void task_focus_change_event(xcb_ewmh_connection_t * ewmh, int default_screen, task_t *task_list, int nbr_of_tasks){
	xcb_window_t fwin;
	if (xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, default_screen), &fwin, NULL) == 1){
		for (int i = 0; i < nbr_of_tasks; i++){
			if (fwin == task_list[i].win){
				task_list[i].state = FOCUSED_TASK;
			}
			if (task_list[i].win != fwin && task_list[i].state == FOCUSED_TASK){
				task_list[i].state = VISIBLE_TASK;
			}
		}
	}
}
void task_desktop_change_event(xcb_ewmh_connection_t * ewmh, task_t *task_list, int nbr_of_tasks, xcb_window_t win){
	for (int i = 0; i < nbr_of_tasks; i++){
		if (task_list[i].win == win){
			task_list[i].desktop_cardinal = get_task_desktop(ewmh, win);
		}
	}
}
int update_task_list(xcb_ewmh_connection_t * ewmh, int default_screen, task_event_t event, task_t *task_list, int nbr_of_tasks){
	if (event.event == TASK_LIST_CHANGE){
		nbr_of_tasks = task_list_change_event(ewmh,  default_screen, task_list);
	}else 
	if (event.event == TASK_NAME_CHANGE){
		task_name_change_event(ewmh, task_list, nbr_of_tasks, event.win);
	} else
	if (event.event == TASK_FOCUS_CHANGE){	
		task_focus_change_event(ewmh, default_screen, task_list, nbr_of_tasks);	
	}else
	if (event.event == TASK_STATE_CHANGE){
		task_state_change_event(ewmh, task_list, nbr_of_tasks, event.win);
	}else
	if (event.event == TASK_DESKTOP_CHANGE){
		task_desktop_change_event(ewmh, task_list, nbr_of_tasks, event.win);
	}
	return nbr_of_tasks;
}

void register_win_events(xcb_connection_t *dpy, xcb_window_t win){
	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	xcb_generic_error_t *err = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, win, XCB_CW_EVENT_MASK, values));
    if (err != NULL)
        fprintf(stderr, "could not capture property change events on window 0x%X\n", win);
	
}
task_t init_task(void){
	task_t task;
	task.win = INIT_TASK_VAL;
	task.title = NULL;
	task.state = UNKNOWN;
	task.desktop_cardinal = -1;
	return task;
}
task_t *create_task_list(void){
	task_t *tasks = (task_t*)malloc(MAX_TASK_NBR*sizeof(task_t));
	if (tasks == NULL) return tasks;
	for (int i = 0; i < MAX_TASK_NBR; i++){
		tasks[i] = init_task();
	}
	return tasks;
}
