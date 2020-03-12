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


void die_atom_error(char *atom) {
    char error[64];
    sprintf(error, "cannot get %s atom", atom);
    die(error);
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


char *get_window_property(xcb_connection_t *conn, xcb_window_t window, char *property, char *type, uint8_t empty_to_null) {
    xcb_atom_t property_atom = get_atom(conn, property);
    if (!property_atom) {
        die_atom_error(property);
    }
    xcb_atom_t type_atom = get_atom(conn, type);
    if (!type_atom) {
        die_atom_error(type);
    }
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *reply;
    cookie = xcb_get_property(conn, 0, window, property_atom, type_atom, 0, 64);
    reply = xcb_get_property_reply(conn, cookie, NULL);
    if (!reply) {
        return NULL;
    }
    int length = xcb_get_property_value_length(reply);
    if (length == 0 && empty_to_null) {
        return NULL;
    }
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
    name = get_window_property(conn, window, "_NET_WM_NAME", "UTF8_STRING", 1);
    if (!name) {
        name = get_window_property(conn, window, "WM_NAME", "UTF8_STRING", 1);
    }
    if (!name) {
        name = get_window_property(conn, window, "WM_NAME", "STRING", 0);
    }
    if (!name) {
        name = "";
    }

    char *raw_class = get_window_property(conn, window, "WM_CLASS", "STRING", 0);
    if (!raw_class) {
        die("cannot read window class");
    }
    char *instance = raw_class;
    char *class = strchr(raw_class, '\0');
    if (!class) {
        die("cannot find class");
    }
    class++;

    printf(
        "id: %u\nname: %s\ninstance: %s\nclass: %s\n",
        window, name, instance, class
    );

    xcb_disconnect(conn);
}
