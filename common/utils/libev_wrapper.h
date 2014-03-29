#ifndef LIBEV_IOS_H_
#define LIBEV_IOS_H_

#include <ev.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "conn.h"

typedef void (*event_callback)(struct ev_loop* reactor,ev_io* w, int events) ;

static void _set_evio_userdata(ev_io* watcher, void* pdata) {
    assert(watcher && "watcher is null") ;
    watcher->data = (void*)pdata ;    
}
static void* _get_evio_userdata(ev_io* watcher) {
    assert(watcher && "watcher is null") ;
    return watcher->data ;
}

#define set_watcher_userdata(watcher, pdata)    \
                            _set_evio_userdata(watcher, pdata)

#define get_watcher_userdata(watcher)           \
                            _get_evio_userdata(watcher) 


// !! NOTE !!: this function has segment core bug(the watcher's memory can not access)
//static int add_fd_event(struct ev_loop* loop, conn* c, int flags, event_callback cb) {
//    assert(loop && "ev_loop is null") ;
//    assert(c && "conn is null") ;
//    assert(c->sock_fd_ != -1 && "c->sock_fd_ == -1") ;
//
//    ev_io watcher ;
//    ev_io_init(&watcher, cb, c->sock_fd_, flags) ;
//    ev_io_start(loop, &watcher) ;
//}

static int add_fd_event(struct ev_loop* loop, ev_io* watcher, conn* c, int flags, event_callback cb) {
    assert(loop && "ev_loop is null") ;
    assert(watcher && "watcher is null") ;
    assert(c && "c is null") ;
    assert(c->sock_fd_ != -1 && "c->sock_fd_ == -1") ;

    set_watcher_userdata(watcher, (void*)c) ;
    ev_io_init(watcher, cb, c->sock_fd_, flags) ;
    ev_io_start(loop, watcher) ;
}

static int remove_fd_event(struct ev_loop* loop, ev_io* watcher) {
    assert(loop && "ev_loop is null") ;
    assert(watcher && "watcher is null") ;

    ev_io_stop(loop, watcher) ;
    return 0 ;
}



static struct ev_loop* new_loop(int flags=EVFLAG_AUTO) {
    return ev_loop_new(flags);
}

static int run_loop(struct ev_loop* loop) {
    assert(loop && "ev_loop is null") ;
    ev_run(loop, 0) ;
    ev_loop_destroy(loop) ;
    return 0 ;
}

static int stop_loop(struct ev_loop* loop) {
    assert(loop && "ev_loop is null") ;
    ev_break(loop, EVBREAK_ALL);
}

#endif

