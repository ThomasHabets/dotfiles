# Show derived object type, not Base
set print object on

# History.
set history filename ~/.gdb_history
set history save on
set history size 100000

set pagination off

# "Are you sure you want to quit?"
set confirm off

# Don't hickup on thread creation.
handle SIG32 noprint pass

# does not work with tui
#set prompt \033[1mgdb> \033[0m

define th-dis
  disass $pc-0x20,+0x40
end

define th-ni
  ni
  th-dis
end
define th-si
  si
  th-dis
end

define th-tracefile
  rbreak $arg0
  commands
    silent
    bt 1
    continue
  end
end

# Command tips:
#
# ## Print backtrace on breakpoint, and continue
#
# (gdb) break close
# blah blah break 2
# (gdb) command 2
# > bt
# > continue
# > end
#
# ## 
