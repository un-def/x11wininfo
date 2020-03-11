#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>


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


char *get_window_property(xcb_connection_t *conn, xcb_window_t window, xcb_atom_enum_t property) {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *reply;
    cookie = xcb_get_property(conn, 0, window, property, XCB_ATOM_STRING, 0, 64);
    reply = xcb_get_property_reply(conn, cookie, NULL);
    if (!reply) {
        return NULL;
    }
    char *name = (char *)xcb_get_property_value(reply);
    free(reply);
    return name;
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

    char *name = get_window_property(conn, window, XCB_ATOM_WM_NAME);
    if (!name) {
        die("cannot read window name");
    }

    char *class = get_window_property(conn, window, XCB_ATOM_WM_CLASS);
    if (!class) {
        die("cannot read window class");
    }

    char *classname = strchr(class, '\0');
    if (!classname) {
        die("cannot find classname");
    }
    classname++;

    printf(
        "id: %u\nname: %s\nclass: %s\nclassname: %s\n",
        window, name, class, classname
    );
}
