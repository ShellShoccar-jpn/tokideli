# How to Record and Replay Your Baby's First Typing (A typeliner & linets & tscat Tutorial)

（日本語版は[こちら](rec_baby_typing.info.ja.md)）

Watching your child grow up is a joy. But you only get to see them as they are right now, so you want to capture their day-to-day moments in photos and videos. For a hacker, though, wouldn't it be fun to record exactly what characters your baby types — and how — the very first time they touch a keyboard, so you can look back on it later? This document is for hackers like that: it shows you how to record, timing included, what your baby types, and how to play it back.

## What You'll Need

* A baby (at least about six months old)
* A computer running a POSIX-compatible OS (one where you can build C programs)
* tokideli (this software)

## Preparation

Got everything ready? If you don't have a baby, borrow one from a coworker or a relative. Or, bring your computer to wherever they are.

Once you have everything, open a terminal and install tokideli. See the [README](../README.en.md) for installation instructions.

Once tokideli's commands are installed and ready to use, first test the typing-recording setup yourself. Careful preparation matters in everything, so you don't fail when it really counts.

Type the following one-liner, then type `hello,⏎world!⏎` on the keyboard, and finally press [Ctrl]+[D].

```
$ typeliner -e | linets -3I > test_typing.log
hello,⏎
world!⏎ ← (press [Ctrl]+[D] at the end)
$ 
```

Here is what each command's options mean.

* [typeliner(1)](typeliner.man.en.md) command
  * -e option: enables local echo, so the typed characters also show up on the terminal screen
* [linets(1)](linets.man.en.md) command
  * -3 option: sets the timestamp resolution to milliseconds
  * -I option: sets the timestamp format to extended ISO 8601

This records both what you typed and its timing, at millisecond precision. These commands can record at even higher precision, but given that home videos top out around 60fps, this should be more than enough.

Now, let's play it back. Type the following one-liner.

```
$ tscat -yIZ test_typing.log
hello,  ← (what you typed,)
world!  ← (appears with the same timing)
$ 
```

Here is what the options mean.

* [tscat(1)](tscat.man.en.md) command
  * -I option: accepts timestamps in extended ISO 8601 format
  * -y option: outputs each timestamped string without adding a newline (i.e., faithfully restores what was fed into [typeliner(1)](typeliner.man.en.md))
  * -Z option: uses the timestamp of the first string in the input file as the baseline, and times the output of the following strings relative to it

Don't forget the -Z option — it's the most important one. Without it, `tscat` tries to output each line at its recorded timestamp's actual date and time, which is of course always in the past, so every character ends up printed instantly, all at once.

So, did your own "hello, world!" play back with the same timing you typed it? If it worked, let's move on to the real thing!

## Showtime

Now for showtime. Bring your baby in front of the keyboard. Type the one-liner below, this time with the real recording's filename, and let your baby bang away on the keyboard to their heart's content. Once the baby's excitement dies down, press [Ctrl]+[D] to end the recording.

```
$ typeliner -e | linets -3I > baby_typing.log
(let the baby bang away on the keyboard, then press [Ctrl]+[D] at the end)
$ 
```

Now `baby_typing.log` should hold a record of your baby's energetic typing. Let's play it back!

```
$ tscat -yIZ baby_typing.log
(your baby's typing appears here, with its original timing)
$ 
```

By the way, I made a recording of my own, too. I've pasted that data at the end of this document, so try playing it back with the one-liner below.

```
$ cat /PATH/TO/TOKIDELI/manual/rec_baby_typing.info.en.md | sed -n '/BEGIN_BABY_TYPING/,/END_BABY_TYPING/{/^[0-9]/p;}' | tscat -yIZ
```

## Finally

Blessings to the future genius hacker!

## Appendix: One Baby's Typing Data

```
### BEGIN_BABY_TYPING ###
2026-10-06T10:51:14,483+09:00 [D[B
2026-10-06T10:51:15,426+09:00 [B
2026-10-06T10:51:15,890+09:00 [B
2026-10-06T10:51:16,132+09:00 [A
2026-10-06T10:51:16,641+09:00 [A
2026-10-06T10:51:16,671+09:00 [A
2026-10-06T10:51:16,703+09:00 [A
2026-10-06T10:51:16,735+09:00 [A
2026-10-06T10:51:16,766+09:00 [A
2026-10-06T10:51:16,796+09:00 [A
2026-10-06T10:51:16,842+09:00 [A
2026-10-06T10:51:16,874+09:00 [A
2026-10-06T10:51:16,905+09:00 [A
2026-10-06T10:51:16,937+09:00 [A
2026-10-06T10:51:16,967+09:00 [A
2026-10-06T10:51:16,999+09:00 [A
2026-10-06T10:51:17,029+09:00 [A
2026-10-06T10:51:17,062+09:00 [A
2026-10-06T10:51:17,107+09:00 [A
2026-10-06T10:51:17,139+09:00 [A
2026-10-06T10:51:17,170+09:00 [A
2026-10-06T10:51:17,203+09:00 [A
2026-10-06T10:51:17,234+09:00 [A
2026-10-06T10:51:17,264+09:00 [A
2026-10-06T10:51:17,295+09:00 [A
2026-10-06T10:51:17,325+09:00 [A
2026-10-06T10:51:17,372+09:00 [A
2026-10-06T10:51:17,402+09:00 [A
2026-10-06T10:51:17,435+09:00 [A
2026-10-06T10:51:17,465+09:00 [A
2026-10-06T10:51:17,496+09:00 [A
2026-10-06T10:51:17,526+09:00 [A
2026-10-06T10:51:17,558+09:00 [A
2026-10-06T10:51:17,590+09:00 [A
2026-10-06T10:51:17,620+09:00 [A
2026-10-06T10:51:17,666+09:00 [A
2026-10-06T10:51:17,699+09:00 [A
2026-10-06T10:51:17,729+09:00 [A
2026-10-06T10:51:23,941+09:00 m
2026-10-06T10:51:23,941+09:00 ,
2026-10-06T10:51:25,814+09:00 m,
2026-10-06T10:51:26,631+09:00 m,.
2026-10-06T10:51:26,739+09:00 .
2026-10-06T10:51:26,947+09:00 .
2026-10-06T10:51:33,854+09:00 ：＠＠ｐ－オｌ０８
2026-10-06T10:51:34,356+09:00 7
2026-10-06T10:51:34,387+09:00 u
2026-10-06T10:51:35,845+09:00 u
2026-10-06T10:51:36,351+09:00 u
2026-10-06T10:51:36,382+09:00 u
2026-10-06T10:51:36,414+09:00 u
2026-10-06T10:51:36,446+09:00 u
2026-10-06T10:51:36,478+09:00 u
2026-10-06T10:51:36,523+09:00 u
2026-10-06T10:51:36,556+09:00 u
2026-10-06T10:51:36,586+09:00 u
2026-10-06T10:51:36,619+09:00 u
2026-10-06T10:51:36,650+09:00 u
2026-10-06T10:51:36,681+09:00 u
2026-10-06T10:51:36,713+09:00 u
2026-10-06T10:51:36,744+09:00 u
2026-10-06T10:51:36,775+09:00 u
2026-10-06T10:51:36,820+09:00 u
2026-10-06T10:51:36,852+09:00 u
2026-10-06T10:51:36,882+09:00 u
2026-10-06T10:51:36,915+09:00 u
2026-10-06T10:51:36,945+09:00 u
2026-10-06T10:51:36,978+09:00 u
2026-10-06T10:51:37,008+09:00 u
2026-10-06T10:51:37,040+09:00 u
2026-10-06T10:51:37,072+09:00 u
2026-10-06T10:51:37,118+09:00 u
2026-10-06T10:51:37,149+09:00 u
2026-10-06T10:51:37,181+09:00 u
2026-10-06T10:51:37,211+09:00 u
2026-10-06T10:51:37,242+09:00 u
2026-10-06T10:51:37,274+09:00 u
2026-10-06T10:51:37,305+09:00 u
2026-10-06T10:51:37,336+09:00 u
2026-10-06T10:51:37,603+09:00 [15~
2026-10-06T10:51:38,100+09:00 	
2026-10-06T10:51:40,117+09:00 １ｑ１
2026-10-06T10:51:41,123+09:00 [20~
2026-10-06T10:51:41,571+09:00 [17~
2026-10-06T10:51:41,684+09:00 [17~
2026-10-06T10:51:41,812+09:00 [18~
2026-10-06T10:51:42,500+09:00 [17~
### END_BABY_TYPING ###
```
