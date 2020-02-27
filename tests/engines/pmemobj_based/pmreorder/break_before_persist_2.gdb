set width 0
set height 0
set verbose off
set confirm off
set breakpoint pending on

# break before the persist #2
b pmemobj_persist
ignore 1 1
run

# it should print out: "breakpoint already hit 2 times"
info break 1

quit
