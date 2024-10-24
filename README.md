Minimal cli based Activity monitor for mac.

The way it works is that this daemon is created and increments every minute. Then the cli tool just sends it a sighup which dumps the file and is read by the tool. It doesn't read anything about any windows and it just queries a macos api to check how long the screen has been idle.


# How to install:

git clone https://github.com/patrickfenn/mac_activity_monitor.git

#Override optional knobs by settings env variables:

#Sunday = 0, Monday = 1, ..., Saturday = 6
#export DAY_TO_RESET="1" # Reset on Monday

#export HOUR_TO_RESET="00" # At Midnight (24 hour)

#export MAX_IDLE_SECONDS=600 # How many seconds one can be afk for until counter stops

cd mac_activity_monitor

make

# To Run:


install/counter #start the daemon

install/activity #query the daemon.

# Example:

➜  install/counter

Daemon process PID: 42854

...

➜  install/activity

➜  activity_monitor git:(main) ✗ install/activity

-----------------------------------------

\*************************
\* Day  total: 0h 9m     \*
\* Week total: 0h 9m     \*
\*************************
\* 00|00|00|00|00|00|00| \*
\* S |M |T |W |T |F |S | \*
\************************\*

-----------------------------------------

---

# For easier use, you can do:

chmod +x install/activity

sudo mv install/activity /usr/local/bin

And then it will be available in your path. Where you can just type in your preferred shell:

> activity

---

Only tested on latest mac