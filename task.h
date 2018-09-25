#ifndef _TASK_H
#define _TASK_H
#define MAX_TASK_NBR 256
#define INIT_TASK_VAL 0
typedef enum {
	TASK_LIST_CHANGE,
	TASK_NAME_CHANGE,
	TASK_FOCUS_CHANGE,
	TASK_DESKTOP_CHANGE,
	TASK_STATE_CHANGE
} t_event_t;

typedef struct {
	t_event_t event;
	xcb_window_t win;
} task_event_t;
typedef enum {
	UNKNOWN,
	FOCUSED_TASK,
	VISIBLE_TASK,
	HIDDEN_TASK
} task_state_t;

typedef struct task {
	xcb_window_t win;
	char *title;
	task_state_t state;
	int desktop_cardinal;
} task_t;

size_t get_task_title(xcb_ewmh_connection_t *, xcb_window_t, char *, size_t);
task_state_t get_task_state(xcb_ewmh_connection_t *, int, xcb_window_t);
task_t get_task_t(xcb_ewmh_connection_t *, int, xcb_window_t);
int get_task_desktop(xcb_ewmh_connection_t *, xcb_window_t);
int update_task_list(xcb_ewmh_connection_t *, int, task_event_t, task_t *, int);
void task_focus_change_event(xcb_ewmh_connection_t *, int, task_t *, int);
void task_state_change_event(xcb_ewmh_connection_t *, task_t *, int, xcb_window_t);
void task_name_change_event(xcb_ewmh_connection_t *, task_t *, int, xcb_window_t);
void task_desktop_change_event(xcb_ewmh_connection_t *, task_t *, int, xcb_window_t);
int task_list_change_event(xcb_ewmh_connection_t * , int , task_t *);
void register_win_events(xcb_connection_t *, xcb_window_t);
task_t init_task(void);
task_t *create_task_list(void);


#endif
