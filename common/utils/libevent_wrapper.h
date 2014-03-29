
#ifndef _EVENT_WRAPPER_H_
#define _EVENT_WRAPPER_H_

#include <assert.h>
#include <event.h>

typedef void (*event_cb)(const int fd, const short which, void* args);

static struct event_base* my_event_base_new() {
    struct event_base* base = event_base_new();
    return base;
}

static void my_add_event(struct event_base* base, struct event* fd_ev, int fd, short flags, event_cb fd_handler, void* args)
{
    event_set(fd_ev, fd, flags, fd_handler, args);
    event_base_set(base, fd_ev);
    event_add(fd_ev, NULL);
}

static void my_add_timer(struct event_base* base, struct event* timer_ev, event_cb handler, struct timeval& tv, void* args)
{
    assert(base);
    assert(timer_ev);
    evtimer_set(timer_ev, handler, args);
    event_base_set(base, timer_ev);
    evtimer_add(timer_ev, &tv);
    return;
}

static void my_add_timer(struct event_base* base, struct event* timer_ev, event_cb handler, int sec, int us=0, void* args=NULL)
{
    assert(base);
    assert(timer_ev);
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = sec; tv.tv_usec = us;
    my_add_timer(base, timer_ev, handler, tv, args);
    return;
}


static void my_run_loop(struct event_base* base) {
    assert(base);
    event_base_dispatch(base);
}

#endif

