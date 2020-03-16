project := 'x11wininfo'

build:
  gcc {{project}}.c -o {{project}} $(pkg-config --cflags --libs xcb) -Wall

run +args='': build
  ./{{project}} {{args}}
