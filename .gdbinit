# Show derived object type, not Base
set print object on

# History.
set history filename ~/.gdb_history
set history save on
set history size 10000

# "Are you sure you want to quit?"
set confirm off

# Don't hickup on thread creation.
handle SIG32 noprint pass
