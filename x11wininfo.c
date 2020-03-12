#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xproto.h>


void die(char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    exit(1);
}


xcb_window_t get_focused_window(xcb_connection_t *conn) {
    xcb_window_t focused;
    xcb_get_input_focus_cookie_t cookie;
    xcb_get_input_focus_reply_t *reply;
    cookie = xcb_get_input_focus(conn);
    reply = xcb_get_input_focus_reply(conn, cookie, NULL);
    if (!reply) {
        return 0;
    }
    focused = reply->focus;
    free(reply);
    return focused;
}


xcb_atom_t get_atom(xcb_connection_t *conn, char *name) {
    xcb_atom_t atom;
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t *reply;
    cookie = xcb_intern_atom(conn, 0, strlen(name), name);
    reply = xcb_intern_atom_reply(conn, cookie, NULL);
    if (!reply) {
        return 0;
    }
    atom = reply->atom;
    free(reply);
    return atom;
}


char *get_window_property(xcb_connection_t *conn, xcb_window_t window, xcb_atom_t atom) {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *reply;
    cookie = xcb_get_property(conn, 0, window, atom, XCB_ATOM_STRING, 0, 64);
    reply = xcb_get_property_reply(conn, cookie, NULL);
    if (!reply) {
        return NULL;
    }
    int length = xcb_get_property_value_length(reply);
    char *value = malloc(length + 1);
    value[length] = '\0';
    memcpy(value, xcb_get_property_value(reply), length);
    free(reply);
    return value;
}


int main() {
    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        xcb_disconnect(conn);
        die("error opening display");
    }

    uint32_t window = get_focused_window(conn);
    if (!window) {
        die("cannot find focused window");
    }

    char *name;
    char *class;
    char *classname;

    xcb_atom_t atom_wm_name;
    atom_wm_name = get_atom(conn, "_NET_WM_NAME");
    if (!atom_wm_name) {
        die("cannot get _NET_WM_NAME atom");
    }
    name = get_window_property(conn, window, atom_wm_name);
    if (!name || strlen(name) == 0) {
        atom_wm_name = get_atom(conn, "WM_NAME");
        if (!atom_wm_name) {
            die("cannot get WM_NAME atom");
        }
        name = get_window_property(conn, window, atom_wm_name);
        if (!name) {
            die("cannot read window name");
        }
    }

    class = get_window_property(conn, window, XCB_ATOM_WM_CLASS);
    if (!class) {
        die("cannot read window class");
    }

    classname = strchr(class, '\0');
    if (!classname) {
        die("cannot find classname");
    }
    classname++;

    printf(
        "id: %u\nname: %s\nclass: %s\nclassname: %s\n",
        window, name, class, classname
    );

    xcb_disconnect(conn);
}
