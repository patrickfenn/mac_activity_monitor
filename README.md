# Minimal cli based Activity monitor for mac.

The way it works is that this daemon is created and increments every minute. Then the cli tool just sends it a sighup which dumps the file and is read by the tool. It doesn't read anything about any windows and it just queries a macos api to check how long the screen has been idle.


# How to build:

```
git clone https://github.com/patrickfenn/mac_activity_monitor.git
cd mac_activity_monitor
make
```

# To Run:

```
#Override optional knobs by settings env variables:

#Sunday = 0, Monday = 1, ..., Saturday = 6
#export DAY_TO_RESET="1" # Reset on Monday

#export HOUR_TO_RESET="00" # At the beginning of the day (Midnight on Sunday) (24H)
#export MAX_IDLE_SECONDS=600 # How many seconds one can be afk for until counter stops

install/counter #start the daemon
install/activity #query the daemon
```

# For ease of use:

```
chmod +x install/activity
sudo mv install/activity /usr/local/bin
```

And then it will be available in your path. Where you can just type in your preferred shell:

```
activity
```

# Example:

```
 ✗ install/counter
Daemon process PID: 33518
Counter started.
```

```
 ✗ install/activity
**************************************
* Day  total: 2h 30m                 *
* Week total: 2h 30m                 *
**************************************
* 00.0|00.0|00.0|00.0|02.5|00.0|00.0 *
* S   |M   |T   |W   |T   |F   |S    *
**************************************
```

# How to upgrade counter daemon:

```
kill -1 `pgrep counter` # Dump current counters
kill -9 `pgrep counter` # Kill the process
install/counter # Run the new daemon
```

---

Only tested on latest mac