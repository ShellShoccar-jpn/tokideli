# How to Trick-Play Timestamped Text Data

（日本語版は[こちら](trick_plays_with_awk_and_tscat.info.ja.md)）

Using [linets(1)](linets.man.en.md)/[charts(1)](charts.man.en.md) and [tscat(1)](tscat.man.en.md), you can record and replay the timing of data as it flows — that much was already explained in [How to Record and Replay Your Baby's First Typing (A typeliner & linets & tscat Tutorial)](rec_baby_typing.info.en.md).

With a bit of extra work, though, you can go beyond simple playback and pull off trick plays — double speed, half speed, and the like — and that's the technique this document introduces.

## Recording "Dun-dun Dun Dun-Dun," and Trying Out Trick Play

This English version of the document uses the "Dun-dun Dun Dun-Dun" sting from the *Terminator 2* theme — a rhythm familiar to readers everywhere — as the subject for this trick-play tutorial.

### First, Record It

Type the following one-liner. Once you press [Enter], tap any character key on your keyboard to the rhythm of "Dun-dun Dun Dun-Dun." When you're done, press [Ctrl]+[D] to end the command.

```sh
$ typeliner -e | linets -3Z > T2_typing.txt
(tap any character key to the rhythm of "Dun-dun Dun Dun-Dun" here)
(finish by pressing [Ctrl]+[D])
```

This records the characters you typed, along with the "Dun-dun Dun Dun-Dun" rhythm, into a file called `T2_typing.txt`. You can check its contents with the `cat` command.

```sh
$ cat T2_typing.txt
0 d
0.172 d
0.481 d
0.802 d
1.000 d
2.043 d
2.183 d
2.526 d
2.806 d
2.984 d
4.046 d
4.206 d
4.513 d
4.859 d
5.044 d
6.124 d
6.263 d
6.618 d
6.904 d
7.081 d
$ 
```

The key point here is that the timestamps are given as elapsed seconds measured from the first line. As you can see from the command above, that's exactly why we used `linets(1)`'s `-Z` option. You could instead use elapsed seconds measured from the moment the command itself started — in that case, use `linets(1)`'s `-z` option.

### Playing It Back at the Original Speed First

Playing it back at the original timing is easy — just pipe it into `tscat -z` (the `tscat(1)` option that corresponds to timestamps `linets(1)` generated with either `-z` or `-Z` is `-z`). But since this is a rhythm, you'll want to enjoy it by ear too. So let's use `sed(1)` to append a bell character (`0x07`) to the end of each line, and play it like this.

```sh
$ cat T2_typing.txt | sed $(printf 's/$/\007/') | tscat -z
```

This should have played back the "Dun-dun Dun Dun-Dun" rhythm you typed. If you typed it well, it'll play back well; if you fumbled it, it'll play back just as fumbled.

### Playing It Back at 2x and 0.5x Speed

Changing the playback speed means changing how fast time itself flows, so all you need to do is multiply each line's timestamp (the value in the first column) by a constant. The `awk(1)` command is handy for that.

To play it back at 2x speed, just divide by 2, like this.

```sh
$ cat T2_typing.txt | awk '{print $1/2,$2}' | sed $(printf 's/$/\007/') | tscat -z
```

To play it back at 0.5x speed, multiply by 2 instead.

```sh
$ cat T2_typing.txt | awk '{print $1*2,$2}' | sed $(printf 's/$/\007/') | tscat -z
```

To generalize this, if you want to play it back at *n* times speed, write it like this.

```sh
$ cat T2_typing.txt | awk -v spd=n '{print $1/spd,$2}' | sed $(printf 's/$/\007/') | tscat -z
```

Here's a video showing the whole process, from recording the timing to trick-playing it back.

![Recording, playback, double-speed, and half-speed playback of "Dun-dun dun dun-dun"](https://raw.githubusercontent.com/ShellShoccar-jpn/tokideli-assets/main/article_figs/images/t2_typing_demo.gif)

### [Note] When the Original Text Data Contains Spaces

In the "Dun-dun Dun Dun-Dun" example shown here, the text data following each timestamp was only a single character, so this wasn't an issue. But if, say, the data contains a half-width space, the AWK script above won't output anything after that space. If you need such data to come out correctly too, you need to write the AWK script more generally, like this.

```sh
  awk -v spd=n 'BEGIN {
                  while (getline l) {
                    p = index(l," ");
                    t = substr(l,1,p-1)/spd;
                    print t,substr(l,p+1);
                  }
                }' |
```

To explain: since the timestamp and the original text data are separated by the first half-width space that appears, this doesn't rely on AWK's own field-splitting — instead, it finds that position itself and splits the line into the part before and the part after.

## Trick-Playing Data Recorded as UNIX Time or Calendar Time

The trick play introduced here only applies to timestamps expressed as elapsed seconds from some reference time. But with a bit more work, you can also trick-play data that's already recorded in calendar time (`YYYYMMDDhhmmss.ddddddddd`) or UNIX time (`n.ddddddddd`).

### For UNIX-Time Data

This one's easy. Record the time on the first line of the original data, subtract it from each line's UNIX time, and output that — then you can handle it the same way.

First, let's quickly make a text data file with UNIX-time timestamps (`1to10_unixtime.txt`).

```sh
$ seq 1 10 | valve -l 0.1s | linets -3e > 1to10_unixtime.txt
$ cat 1to10_unixtime.txt
1794995170.567 1
1794995170.668 2
1794995170.768 3
1794995170.868 4
1794995170.968 5
1794995171.068 6
1794995171.168 7
1794995171.268 8
1794995171.368 9
1794995171.468 10
$ 
```

To play this back at half speed, let's add the approach shown above into an AWK script and pipe it to `tscat(1)`.

```sh
$ cat 1to10_unixtime.txt |
  awk -v spd=0.5 'BEGIN {if (!getline) {exit;}; t0=$1*1; print 0,$2;}
                  {print ($1-t0)/spd,$2;                            }' |
  tscat -z
1
2
3
4
5
6
7
8
9
10
$ 
```

If you want to write this more generally, it's better to write the `awk(1)` part like this.

```sh
  awk -v spd=0.5 'BEGIN {
                    if (! getline l           ) {exit;}
                    if (! match(l,/^[0-9.]+ /)) {exit;}
                    t0 = substr(l,1,RLENGTH-1)*1;
                    print 0, substr(l,RLENGTH+1);
                    while (getline l) {
                      if (! match(l,/^[0-9.]+ /)) {exit;}
                      t = substr(l,1,RLENGTH-1)-t0;
                      print t/spd, substr(l,RLENGTH+1);
                    }
                  }' |
```

### For Calendar-Time Data

Calendar time can't simply be added or subtracted, so you first have to convert it to UNIX time. That's where tokideli's [calclock(1)](calclock.man.en.md) command comes in. Given a column number as its argument, this command treats that column as calendar time (`YYYYMMDDhhmmss.ddddddddd`), converts it to UNIX time, and inserts the result right after that column.

```sh
$ date +%Y%m%d%H%M%S.%6N | calclock 1
20261118191754.295573 1794997074.295573
$ 
```

Let's use this command to show how to trick-play data with calendar-time timestamps. First, just as before, let's create some text data with calendar-time timestamps.

```sh
$ seq 1 10 | valve -l 0.1s | linets -3c > 1to10_caltime.txt
$ cat 1to10_caltime.txt
20261118192010.744 1
20261118192010.845 2
20261118192010.945 3
20261118192011.045 4
20261118192011.145 5
20261118192011.245 6
20261118192011.345 7
20261118192011.445 8
20261118192011.544 9
20261118192011.645 10
$ 
```

Pipe this through `calclock(1)` and the UNIX time comes out in the second column, so subtract the first line's value from each line's second column, and output only columns 2 and 3 at the end. (You no longer need column 1.)

```sh
$ cat 1to10_caltime.txt |
  calclock 1          |
  awk -v spd=0.5 'BEGIN {if (!getline) {exit;}; t0=$2*1; print 0,$3;}
                  {print ($2-t0)/spd,$3;                            }' |
  tscat -z
1
2
3
4
5
6
7
8
9
10
$ 
```

In this case too, if you want to write it more generally, it's better to write the `awk(1)` part like this.

```sh
  awk -v spd=0.5 'BEGIN {
                    if (! getline l           ) {exit;}
                    if (! match(l,/ [0-9.]+ /)) {exit;}
                    t0 = substr(l,RSTART+1,RLENGTH-2)*1;
                    print 0, substr(l,RSTART+RLENGTH);
                    while (getline l) {
                      if (! match(l,/ [0-9.]+ /)) {exit;}
                      t = substr(l,RSTART+1,RLENGTH-2)-t0;
                      print t/spd, substr(l,RSTART+RLENGTH);
                    }
                  }' |
```

## Summary

This document won't spell out the concrete code, but by making good use of `awk(1)`, you can also trick-play the sample data from [How to Record and Replay Your Baby's First Typing (A typeliner & linets & tscat Tutorial)](rec_baby_typing.info.en.md), which is written with extended ISO 8601 timestamps.

What a wonderfully handy language AWK is. Borrow its power, and put UNIX shells, shell scripts, and tokideli to even better use!

## See Also

[linets(1)](linets.man.en.md), [charts(1)](charts.man.en.md), [tscat(1)](tscat.man.en.md), [typeliner(1)](typeliner.man.en.md), [calclock(1)](calclock.man.en.md), [How to Record and Replay Your Baby's First Typing (A typeliner & linets & tscat Tutorial)](rec_baby_typing.info.en.md)
