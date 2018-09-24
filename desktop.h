#ifndef _DESKTOP_H
#define _DESKTOP_H
#define MAX_DESKTOP_NBR 20


typedef enum {
	TASK_NUMBER_CHANGE,
	DESKTOP_NAMES_CHANGE,
	DESKTOP_FOCUS_CHANGE,
	DESKTOP_NUMBER_CHANGE
} d_event_t;

typedef struct {
	d_event_t event;
	xcb_window_t win;
} desktop_event_t;



typedef enum {
	UNKNOWN_STATE_DESKTOP,
	FOCUSED_DESKTOP,
	OCCUPIED_DESKTOP,
	UNOCCUPIED_DESKTOP
} desktop_state_t;


typedef struct {
	int cardinal;
	char *name;
	desktop_state_t state;
	uint32_t number_of_tasks;
	task_t *tasks;
} desktop_t;







desktop_t init_desktop(void);
desktop_t *create_desktop_list(void);
void desktop_task_number_change_event( task_t *, int, desktop_t *, int);
void desktop_change_names_event(xcb_ewmh_connection_t *, int, desktop_t *, int);
void desktop_focus_change_event(xcb_ewmh_connection_t *, int, desktop_t *, int);
int desktop_number_change_event(xcb_ewmh_connection_t *, int, task_t *, int, desktop_t *, int);
void assign_tasks_to_desktop(task_t *, int, desktop_t *, int);
int update_desktop_list(xcb_ewmh_connection_t *, int, desktop_event_t, task_t *, int, desktop_t *, int);





#endif
