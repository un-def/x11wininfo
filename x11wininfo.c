#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xproto.h>


#define _NAME "x11wininfo"
#define _VERSION "0.1.0"


static char *modes[] = {
    "text",
    "mintext",
    "json",
};
static int modes_count = sizeof(modes) / sizeof(char *);


void
die(char *message, ...) {
    fprintf(stderr, "ERROR: ");
    va_list argptr;
    va_start(argptr, message);
    vfprintf(stderr, message, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");
    exit(1);
}


xcb_atom_t
get_atom(xcb_connection_t *conn, char *name) {
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


xcb_get_property_reply_t *
get_window_property_reply(xcb_connection_t *conn, xcb_window_t window, char *property, char *type) {
    xcb_atom_t property_atom = get_atom(conn, property);
    if (!property_atom) {
        die("cannot get %s property atom", property);
    }
    xcb_atom_t type_atom = get_atom(conn, type);
    if (!type_atom) {
        die("cannot get %s type atom", type);
    }
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *reply;
    cookie = xcb_get_property(conn, 0, window, property_atom, type_atom, 0, 64);
    reply = xcb_get_property_reply(conn, cookie, NULL);
    if (!reply) {
        return NULL;
    }
    return reply;
}


xcb_window_t
get_focused_window(xcb_connection_t *conn, xcb_window_t root) {
    xcb_get_property_reply_t *reply;
    reply = get_window_property_reply(conn, root, "_NET_ACTIVE_WINDOW", "WINDOW");
    if (!reply) {
        return 0;
    }
    xcb_window_t window = *(xcb_window_t *)xcb_get_property_value(reply);
    free(reply);
    return window;
}


char *
get_window_string_property(xcb_connection_t *conn, xcb_window_t window, char *property, char *type, uint8_t empty_to_null) {
    xcb_get_property_reply_t *reply = get_window_property_reply(conn, window, property, type);
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


char *
get_mode_from_argv(int argc, char **argv) {
    extern char *optarg;
    extern int optopt;

    char *mode = modes[0];
    int option = 0;
    while ((option = getopt(argc, argv, ":m:hv")) != -1) {
        switch (option) {
            case 'm':
                mode = optarg;
                break;
            case 'h':
                printf("usage: " _NAME " [-m MODE]\n\nmodes:\n");
                for (int i = 0; i < modes_count; i++) {
                    if (i == 0) {
                        printf("\t%s\t(default)\n", modes[i]);
                    }
                    else {
                       printf("\t%s\n", modes[i]);

                    }
                }
                exit(0);
            case 'v':
                printf(_NAME " version " _VERSION "\n");
                exit(0);
            case ':':
                die("no option value: %c", optopt);
            case '?':
                die("unknown option: %c", optopt);
            default:
                die("unexpected error while parsing options");
        }
    }
    for (int i = 0; i < modes_count; i++) {
        if (strcmp(mode, modes[i]) == 0) {
            return mode;
        }
    }
    die("unsupported mode: %s", mode);
    return mode;
}


int
main(int argc, char **argv) {
    char *mode = get_mode_from_argv(argc, argv);

    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        xcb_disconnect(conn);
        die("cannot connect to X server");
    }

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    if (!screen) {
        xcb_disconnect(conn);
        die("cannot get screen");
    }

    xcb_window_t window = get_focused_window(conn, screen->root);
    if (!window) {
        xcb_disconnect(conn);
        die("cannot find focused window");
    }

    char *name;
    name = get_window_string_property(conn, window, "_NET_WM_NAME", "UTF8_STRING", 1);
    if (!name) {
        name = get_window_string_property(conn, window, "WM_NAME", "UTF8_STRING", 1);
    }
    if (!name) {
        name = get_window_string_property(conn, window, "WM_NAME", "STRING", 0);
    }
    if (!name) {
        name = "";
    }

    char *raw_class = get_window_string_property(conn, window, "WM_CLASS", "STRING", 0);
    if (!raw_class) {
        xcb_disconnect(conn);
        die("cannot read window class");
    }
    char *instance = raw_class;
    char *class = strchr(raw_class, '\0');
    if (!class) {
        xcb_disconnect(conn);
        die("cannot find class");
    }
    class++;

    if (strcmp(mode, "json") == 0) {
        printf(
            "{\"id\": \"%u\", \"name\": \"%s\", "
            "\"instance\": \"%s\", \"class\": \"%s\"}\n",
            window, name, instance, class
        );
    } else if (strcmp(mode, "mintext") == 0) {
        printf("%u\n%s\n%s\n%s\n", window, name, instance, class);
    } else {
        printf(
            "id: %u\nname: %s\ninstance: %s\nclass: %s\n",
            window, name, instance, class
        );
    }
    free(name);
    free(raw_class);
    xcb_disconnect(conn);
}
