Minimal cli based Activity monitor for mac. Motivated by my company since they insist on removing everything else I have tried to use. So I figure I can just write my own. The way it works is that this daemon is created and increments every minute. Then the cli tool just sends it a sighup which dumps the file and is read by the tool.

--------

--
How to install:
--

git clone {this repo}

Set knobs in :

counter/src/counter.cpp

cli/src/cli.cpp

->

#define ACTIVE_TIME_PATH "/Users/patrickfenn/active" # path to active time dump path

#define PID_PATH "/Users/patrickfenn/counter.pid" # path to pid of counter daemon

#define DAY_TO_RESET "Sunday" // Resets Sunday at...

#define HOUR_TO_RESET "00" // Midnight (24 hour)

Ensure they are both writeable by the application.

--
To Build:
--

cd cli

make

cd ../counter

make

cd ..

counter/install/counter #start the daemon

cli/install/cli #query the daemon.

--

Only tested on latest mac version (mbp m2). If this gets any traction at all I will attempt to make it more user friendly...