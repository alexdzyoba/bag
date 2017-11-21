# bag

Safety bag for your stdin.

This tool is handy when your stdout has pauses but you don't want to block stdin. 
To archieve this `bag` reads stdin, saves it in ring buffer and output to stdout from ring buffer.
In case of stdout block input will be saved in ring buffer until it fills up. 
In the latter case it will block stdin.

The size of the bag (the ringbuffer) is set by lines of size `16 * LINE_MAX` that usually is `16 * 2048 = 32K` lines.

To manage stdin, stdout and signals it will poll it with `poll` (signal is polled with signalfd).
