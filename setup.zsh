#!/bin/zsh
echo "Building..."
make clean; make
activity_pid=`/Users/Shared/activity.pid`
if [[ -n "$activity_pid" ]]; then
    echo "Pid found, dumping counters and killing..."
    kill -1 "$activity_pid"
    sleep 1
    kill -9 "$activity_pid"
fi
echo "Starting new counter daemon..."
install/counter
chmod +x install/activity
echo "Moving new activity binary to /usr/local/bin"
sudo mv install/activity /usr/local/bin
echo "Completed"