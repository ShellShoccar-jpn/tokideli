# Smooth out data delivery by delaying it, just like live video streaming

（日本語版は[こちら](smooth_out_data_delivery_like_live_video_streaming.info.ja.md)）

[How to Delay Output Line by Line](delayed_output_line_by_line.info.en.md) only covered delaying output on its own. Here, let's walk through a practical example: a case where you want to send data from an IoT device over a path — such as the Internet — whose arrival-time jitter is large.

## A similar example (live video streaming)

Think about watching a live video stream. When the network is unstable, the video suddenly freezes or fast-forwards. To prevent this, it's common to intentionally delay playback by a few seconds from the moment of capture (let's call this "buffered playback" here). If you decide to delay playback by, say, 3 seconds, the video can play back smoothly as long as the arrival-time jitter stays within 3 seconds.

You can achieve the same thing by combining the commands bundled with tokideli with the awk(1) command.

## The model we'll assume

First, here's the setup we'll assume.

```
 [Device #1]
      |
 (USB-Serial)
      |
      V
+--------------------------------------+
| [IoT host #1]                        |
|                                      |
| * Receive data from Device #1 via cu |
| * Add a timestamp with linets        |
| * Publish it as MQTT via             |
|   mosquitto_pub, etc.                |---(MQTT over Internet)---+
+--------------------------------------+                          |
                                                                  |
                                                                  |
                                                                  V
                                                           [MQTT broker]
                                                                  |
+--------------------------------------+                          |
| [IoT host #2]                        |                          |
|                                      |                          |
| * Receive via mosquitto_sub, etc.    |<-------------------------+
| * Add a fixed delay to the           |
|   timestamp with AWK                 |
| * Buffer it with surgetk             |
| * Delay-output it with tscat         |
| * Send it to Device #2 via cu        |
+--------------------------------------+
      |
 (USB-Serial)
      |
      V
 [Device #2]
```

Suppose there are two devices with a serial interface (USB-serial), "Device #1" and "Device #2", and you want to send data from the former to the latter. However, the two devices are physically far apart from each other. So you decide to connect each of them to "IoT host #1" and "IoT host #2" respectively, and have the two IoT hosts exchange data via an MQTT broker on the Internet.

The problem here is jitter in the arrival time. By uniformly delaying the arrival time from Device #1 to Device #2 (i.e. buffered playback toward Device #2), you can absorb this jitter.

The other conditions are as follows.

* The serial interface's transfer rate is 57600bps.
* You decide to delay the arrival from Device #1 to Device #2 by 3 seconds, so that you can absorb up to 3 seconds of arrival-time jitter.

## The one-liners for buffered playback

Here are the one-liners you should write to satisfy the setup and requirements described above. Let's look at IoT host #1 and #2 in turn.

### IoT host #1

Run a command like the following.

```sh:
$ cat | cu -s 57600 -l /dev/MY_USBSERIAL1 | linets -e6 | mosquitto_pub -l -h BROKER_ADDRESS -t TOPIC_NAME
```

Here's the role of each command.

* The cat(1) command
  * Plays no role at all in the data exchange itself. (It's harmless.)
  * It's only there as a trick so that the [cu(1)](https://pubs.opengroup.org/onlinepubs/7908799/xcu/cu.html) command right after it can be terminated easily with [Ctrl]+[C].
* The [cu(1)](https://pubs.opengroup.org/onlinepubs/7908799/xcu/cu.html) command
  * Used to exchange data with the device connected to the serial interface.
  * Assuming "Device #1" is connected to /dev/MY_USBSERIAL1, it receives data from there at 57600bps.
* The [linets(1)](linets.man.en.md) command
  * Prepends a timestamp to each incoming line.
  * Outputs it as UNIX time with the `-e` option, so that it can be used for time arithmetic later.
  * Outputs it with microsecond precision, thanks to the `-6` option.
* The [mosquitto_pub(1)](https://mosquitto.org/man/mosquitto_pub-1.html) command
  * Sends (publishes) the received data to the MQTT broker.
  * The MQTT broker address is "BROKER_ADDRESS" and the topic name is "TOPIC_NAME".
  * Each MQTT packet is sent line by line. (the `-l` option)

The key point is to attach a timestamp to the data as soon as it comes in over the serial interface. That's why the [linets(1)](linets.man.en.md) command should be placed right after the [cu(1)](https://pubs.opengroup.org/onlinepubs/7908799/xcu/cu.html) command on IoT host #1. If you attached the timestamp on IoT host #2 instead, it would already have been affected by jitter by then, so you would no longer be able to absorb it.

If you want to manage this on a per-byte basis rather than per-line, use the [charts(1)](charts.man.en.md) command instead of the [linets(1)](linets.man.en.md) command, as follows.

```sh:
$ cat | cu -s 57600 -l /dev/MY_USBSERIAL1 | charts -be6 | mosquitto_pub -l -h BROKER_ADDRESS -t TOPIC_NAME
```

* The [charts(1)](charts.man.en.md) command
  * Prepends a timestamp to each incoming character or byte.
  * Here, the `-b` option is given, so it adds the timestamp on a per-byte basis.
  * Outputs it as UNIX time with the `-e` option, so that it can be used for time arithmetic later. (same as [linets(1)](linets.man.en.md))
  * Outputs it with microsecond precision, thanks to the `-6` option. (same as [linets(1)](linets.man.en.md))

### IoT host #2

Run a command like the following.

```sh:
$ mosquitto_sub -h BROKER_ADDRESS -t TOPIC_NAME |
  surgetk 22KB                                  |
  awk -v dt=3.0 '
    BEGIN {
      OFMT="%.6f"; OFS="";
      while (getline l) {
        i=index(l," ");
        print substr(l,1,i-1)+dt, substr(l,i);
        fflush();
      }
    }'                                          |
  tscat -e                                      |
  cu -s 57600 -l /dev/MY_USBSERIAL2
```

* The [mosquitto_sub(1)](https://mosquitto.org/man/mosquitto_sub-1.html) command
  * Requests data from the MQTT broker (subscribes).
  * The MQTT broker address is "BROKER_ADDRESS" and the topic name is "TOPIC_NAME".
* The [surgetk(1)](surgetk.man.en.md) command
  * Holds the received data in a buffer, so that a delay downstream doesn't block the command upstream of it (here, [mosquitto_sub(1)](https://mosquitto.org/man/mosquitto_sub-1.html)).
  * The buffer size is 22KB. (See [Deciding on a buffer size](#deciding-on-a-buffer-size) below for how this value was derived)
* The awk(1) command
  * Adds the number of seconds you want to delay by (here, 3.0 seconds) to the UNIX time at the front of each line (the timestamp added by the [linets(1)](linets.man.en.md) command).
* The [tscat(1)](tscat.man.en.md) command
  * Outputs data line by line, according to the timestamp at the front of each line.
  * The `-e` option tells it to interpret the timestamp as UNIX time.
* The [cu(1)](https://pubs.opengroup.org/onlinepubs/7908799/xcu/cu.html) command
  * Used to exchange data with the device connected to the serial interface.
  * Assuming "Device #2" is connected to /dev/MY_USBSERIAL2, it sends data there at 57600bps.

If you added the timestamp on a per-byte basis on the IoT host #1 side (using the [charts(1)](charts.man.en.md) command instead of the [linets(1)](linets.man.en.md) command), add the `-y` option to the [tscat(1)](tscat.man.en.md) command. This prevents a newline from being appended after every single byte of the original data, so that it's correctly reconstructed.

## Deciding on a buffer size

To decide on a reasonable buffer size (the *size* argument) for the [surgetk(1)](surgetk.man.en.md) command (likewise for the [delay(1)](delay.man.en.md) command), define the following parameters.

* *sl* — the average number of bytes per line in the incoming text data
* *nl* — the number of lines arriving per second
* *dt* — the number of seconds you want to delay by

Given these, the number of bytes *sb* that must be held for the delayed output (i.e. the minimum value to give as the *size* parameter of the [surgetk(1)](surgetk.man.en.md)/[delay(1)](delay.man.en.md) commands) can be computed as:

```
sb = sl * nl * dt
```

So, for example, if each line is 81 bytes on average, 50 lines arrive per second on average, and you want to delay them by 3 seconds, then *sb* = 12150 (about 12KiB).

Alternatively, if you know the incoming data's transfer rate is *r* bps, *sb* can be computed as:

```
sb = ( r / 8 ) * dt
```

So, for example, if you want to delay data arriving over a 57600bps link by 5 seconds, *sb* = 36000 (36kB).

A couple of notes:

* These are theoretical estimates. Real-world fluctuations in the transfer rate can push the actual requirement above them, so it's safer to allocate a somewhat larger buffer than the computed value.
* When you give this value to the [delay(1)](delay.man.en.md) command, it internally tries to allocate roughly 16 times as much memory (on a 64-bit environment), so pay attention to how much memory the host actually has installed. (This multiplier does not apply to the [surgetk(1)](surgetk.man.en.md) command.)

## Summary

On a communication path where the data's arrival time (latency) jitters, if you want to keep the latency constant even at the cost of some delay, buffering the data and delay-replaying it after a fixed amount of time — just like live video streaming — is an effective countermeasure. You can achieve this by combining tokideli's [charts(1)](charts.man.en.md), [linets(1)](linets.man.en.md), [surgetk(1)](surgetk.man.en.md), and [tscat(1)](tscat.man.en.md) commands with the awk(1) command.

## See Also

[How to Delay Output Line by Line](delayed_output_line_by_line.info.en.md)
