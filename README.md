Minimal cli based Activity monitor for mac. Motivated by my company since they insist on removing everything else I have tried to use.

So I figure I can just write my own. The way it works is that this daemon is created and increments every minute. Then the cli tool just sends it a sighup which dumps the file and is read by the tool.

--------

--
How to install:
--

git clone {this repo}

Override optional knobs by settings env variables:

export DAY_TO_RESET="Sunday" # Reset on Sunday

export HOUR_TO_RESET="00" # At Midnight (24 hour)

export MAX_IDLE_SECONDS=600 # How many seconds one can be afk for until counter stops


--
To Build:
--

cd {repo path}

make

--
To Run:
--

install/counter #start the daemon

install/cli #query the daemon.

--
Example:
--
➜  install/counter

Daemon process PID: 42854

...

➜  install/cli

16 hours, 22 minutes

--

Only tested on latest mac version (mbp m2).