# Show derived object type, not Base
set print object on

# History.
set history filename ~/.gdb_history
set history save on
set history size 100000

# "Are you sure you want to quit?"
set confirm off

# Don't hickup on thread creation.
handle SIG32 noprint pass

# does not work with tui
#set prompt \033[1mgdb> \033[0m
