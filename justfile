project := 'x11wininfo'

build:
  gcc {{project}}.c -o {{project}} $(pkg-config --cflags --libs xcb-atom) -Wall

run: build
  ./{{project}}
