#ifndef _EVENT_H
#define _EVENT_H


typedef enum {
	NO_EVENT,
	DESKTOP_NAMES_EVENT,
	TASK_FOCUS_EVENT,
	DESKTOP_LIST_EVENT,
	DESKTOP_FOCUS_EVENT,
	TASK_LIST_EVENT,
	TASK_NAME_EVENT,
	TASK_HIDDEN_EVENT,
	TASK_DESKTOP_EVENT
} server_event_t;
typedef struct {
	server_event_t server_event;
	xcb_window_t win;
} generic_event_t;

void register_root_events(xcb_connection_t *, xcb_screen_t *, int *);

generic_event_t event_generator(xcb_ewmh_connection_t *, xcb_screen_t *, xcb_generic_event_t *);
int event_handler(xcb_ewmh_connection_t *, int, generic_event_t, task_t *, int *, desktop_t *, int *);
















#endif
