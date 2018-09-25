#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "task.h"
#include "desktop.h"
#include "event.h"


void register_root_events(xcb_connection_t *dpy, xcb_screen_t *screen, int *run){
	uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	xcb_generic_error_t *err = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, screen->root, XCB_CW_EVENT_MASK, values));
	if (err != NULL)
		run = 0;	
}


generic_event_t event_generator(xcb_ewmh_connection_t *ewmh, xcb_screen_t *screen, xcb_generic_event_t *evt){
	xcb_property_notify_event_t *pne;
	generic_event_t event;
	switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
		case XCB_PROPERTY_NOTIFY:
			pne = (xcb_property_notify_event_t *) evt;
			if (pne->atom == ewmh->_NET_DESKTOP_NAMES) {
				event.server_event = DESKTOP_NAMES_EVENT;
				event.win = pne->window;
				return event;
			} else if (pne->atom == ewmh->_NET_ACTIVE_WINDOW) {
				event.server_event = TASK_FOCUS_EVENT;
				event.win = pne->window;
				return event;
			} else if (pne->atom == ewmh->_NET_NUMBER_OF_DESKTOPS) {
				event.server_event = DESKTOP_LIST_EVENT;
				event.win = pne->window;
				return event;
			} else if (pne->atom == ewmh->_NET_CURRENT_DESKTOP) {
				event.server_event = DESKTOP_FOCUS_EVENT;
				event.win = pne->window;
				return event;
			} else if (pne->atom == ewmh->_NET_CLIENT_LIST){
				event.server_event = TASK_LIST_EVENT;
				event.win = pne->window;
				return event;
			} else if (pne->window != screen->root && pne->atom != ewmh->_NET_ACTIVE_WINDOW){
				if (pne->atom == ewmh->_NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME){
					event.server_event = TASK_NAME_EVENT;
					event.win = pne->window;
					return event;
				}
				if (pne->atom == ewmh ->_NET_WM_STATE){
				 	event.server_event = TASK_HIDDEN_EVENT;
					event.win = pne->window;
					return event;
				}
				if (pne->atom == ewmh ->_NET_WM_DESKTOP){
				 	event.server_event = TASK_DESKTOP_EVENT;
					event.win = pne->window;
					return event;
				}
			}
		default:
			event.server_event = NO_EVENT;
			event.win = pne->window;
			return event;
	}
}

int event_handler(xcb_ewmh_connection_t * ewmh, int default_screen, generic_event_t event, task_t *task_list, int *number_of_tasks, desktop_t *desktop_list, int *number_of_desktops){
	desktop_event_t d_event;
	task_event_t t_event;
	int nbr_of_tasks = *number_of_tasks; int nbr_of_desktops = *number_of_desktops;
	switch (event.server_event) {
		case DESKTOP_NAMES_EVENT:
			d_event.event = DESKTOP_NAMES_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;
			return 1;
		case DESKTOP_LIST_EVENT:
			d_event.event = DESKTOP_NUMBER_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;
			return 1;
		case DESKTOP_FOCUS_EVENT:
			d_event.event = DESKTOP_FOCUS_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;
			return 1;
		case TASK_FOCUS_EVENT:
			t_event.event = TASK_FOCUS_CHANGE;
			t_event.win = event.win;
			nbr_of_tasks = update_task_list(ewmh, default_screen, t_event, task_list, nbr_of_tasks);
			*number_of_tasks = nbr_of_tasks;
			d_event.event = TASK_NUMBER_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;
			return 1;
		case TASK_LIST_EVENT:
			t_event.event = TASK_LIST_CHANGE;
			t_event.win = event.win;
			nbr_of_tasks = update_task_list(ewmh, default_screen, t_event, task_list, nbr_of_tasks);
			*number_of_tasks = nbr_of_tasks;
			d_event.event = TASK_NUMBER_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;
			return 1;
		case TASK_NAME_EVENT:
			t_event.event = TASK_NAME_CHANGE;
			t_event.win = event.win;
			nbr_of_tasks = update_task_list(ewmh, default_screen, t_event, task_list, nbr_of_tasks);
			*number_of_tasks = nbr_of_tasks;
			d_event.event = TASK_NUMBER_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;
			return 1;
		case TASK_HIDDEN_EVENT:
			t_event.event = TASK_STATE_CHANGE;
			t_event.win = event.win;
			nbr_of_tasks = update_task_list(ewmh, default_screen, t_event, task_list, nbr_of_tasks);
			*number_of_tasks = nbr_of_tasks;
			d_event.event = TASK_NUMBER_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;	
			return 1;
		case TASK_DESKTOP_EVENT:
			t_event.event = TASK_DESKTOP_CHANGE;
			t_event.win = event.win;
			nbr_of_tasks = update_task_list(ewmh, default_screen, t_event, task_list, nbr_of_tasks);
			d_event.event = TASK_NUMBER_CHANGE;
			d_event.win = event.win;
			nbr_of_desktops = update_desktop_list(ewmh, default_screen, d_event, task_list, nbr_of_tasks, desktop_list, nbr_of_desktops);
			*number_of_desktops = nbr_of_desktops;	
				
		default:
			return 0;		
	}
	return 0;
}




