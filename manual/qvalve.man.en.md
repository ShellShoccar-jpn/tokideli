# QVALVE(1)

（日本語版は[こちら](qvalve.man.ja.md)）

## Name

qvalve - Quantitative valve: outputs a specified amount of data at a specified time

## Synopsis

```sh:
qvalve [-c|-l] [-t] [-1] [-p n] [-k command] quantity [file [...]]
qvalve [-c|-l] [-t] [-1] [-p n] [-k command] controlfile [file [...]]
```

## Description

This command imitates a quantitative valve. That is, when the amount you want to output (in bytes or lines) is specified, it sends exactly that amount of data to the standard output. By using a controlfile, you can specify the amount at any time and as many times as you like, allowing you to control the output so that any desired amount of data comes out whenever you want.

The "amount" here is a **target value**: "the total amount that should be sent, in the end," independent of how much this command has already sent so far. Consequently, if you specify the same amount again as an absolute value (i.e. without a leading `+`) while the amount already sent is equal to or greater than that target, nothing further is sent. This property is explained in detail in ["Idempotency of an Absolute Quantity Specification."](#idempotency-of-an-absolute-quantity-specification)

## Arguments

### file

The file to be used as the data source. If omitted, or if `-` is specified, the standard input is assumed. You may specify more than one.

### quantity

Specifies the amount to output (in bytes or lines), or a control command ([described below](#control-commands)). Whether the amount is specified in bytes or in lines is determined by the option described later (-c or -l). The format for specifying the amount is as follows.

> [`+`]*n*[`.`*d*][*prefix*]

* *n*[`.`*d*]
  * A number indicating the amount.
  * *n* is an integer, and if you attach `.`*d* right after it, you can also specify a value including a fraction — but note that this may introduce error arising from the floating-point (double) representation.
* [*prefix*]
  * A multiplier prefix.
  * The strings you can use, and their meanings, are as follows.
    * "`k`" ... 1000 times the original amount
    * "`M`" ... 1000^2 times the original amount
    * "`G`" ... 1000^3 times the original amount
    * "`T`" ... 1000^4 times the original amount
    * "`P`" ... 1000^5 times the original amount
    * "`E`" ... 1000^6 times the original amount
    * "`K`" ... 1024 times the original amount (conventional usage)
    * "`ki`" ... 1024 times the original amount
    * "`Mi`" ... 1024^2 times the original amount
    * "`Gi`" ... 1024^3 times the original amount
    * "`Ti`" ... 1024^4 times the original amount
    * "`Pi`" ... 1024^5 times the original amount
    * "`Ei`" ... 1024^6 times the original amount
  * If omitted, it means 1x (the original amount).
* [`+`]
  * A sign for an additive specification.
  * If you attach this sign, the amount you specify is added to the current target total (the target itself, not the amount already sent), regardless of how much has actually been sent so far.
  * If you specify an amount without this sign, the target total itself is overwritten with the amount you specify. Consequently, if the amount already sent is equal to or greater than that value, this operation causes nothing further to be sent (see ["Idempotency of an Absolute Quantity Specification."](#idempotency-of-an-absolute-quantity-specification))

Even after finishing outputting the specified amount, this command does not exit for that reason; it waits for the input file to reach EOF. For details, see [Notes on Command Termination](#notes-on-command-termination).

#### Control Commands

Instead of an amount to output, you can specify a command instructing the behavior. Currently, the following is available.

* `t`
  * Finishes the output and terminates the qvalve command.
  * If the -k option is given, the data transfer is fully stopped first, and only then is the external command run (with `QVALVE_EXEC_BY=t_command`); this command terminates only after that external command finishes. See [Notes on the -k Hook](#notes-on-the--k-hook).

### controlfile

Whereas *quantity* specifies the amount directly through the argument, this method specifies it indirectly by giving a controlfile — a file into which the amount is written. Since this command watches the content of the controlfile, you can add to or change the amount while the command is running, by writing a new parameter into it.

The format of the parameter to write is the same as *quantity*. If an invalid string is given as the parameter, it is silently ignored without an error exit. The default, while no valid parameter has yet been given, is "0".

You can choose one of the following file types to use as the controlfile.

#### Regular Files

If you use a regular file as the controlfile, you must write the new parameter using **the "create" mode (`O_CREAT` or `>`)**, not the "append" mode (`O_APPEND` or `>>`). This is because, in the case of a regular file, this command always watches the beginning of the file.

The file is checked every 0.1 seconds. If you want a newly written parameter to be reflected immediately, send this command a SIGHUP after updating the file.

When using a regular file, note that, for example, if you write "`+10`" to add 10 more output lines, and then write the same string ("`+10`") again to add another 10 lines, the second write does not convey your intent to add more. This is because, when a regular file is overwritten with exactly the same string, there is no way to tell whether it is a newly updated string or the previous one. To reliably convey that a new string has been written, write a different string that still means the same amount — for instance, by inserting a leading space (`" +10"`) or using decimal notation (`"+10.0"`). For details, see [Notes on Using a Regular File as a controlfile](#notes-on-using-a-regular-file-as-a-controlfile).

#### Character-Special Files or Named Pipes

If you use either of these as the controlfile, you may write in either of the above modes. Also, since the parameter string is reflected immediately once you append a linefeed <LF> after it, there is no need to send a signal.

From a performance standpoint, these are more advantageous than regular files.

## Options

### -c, -l

Specifies the unit of the amount of data to output. -c and -l mean, respectively, that the unit of the amount is bytes or lines, and these options are mutually exclusive. If neither is specified, -c is assumed.

In byte-output mode, the command sleeps after each byte it outputs; in line-output mode, it sleeps after each line it outputs.

### -t

If a controlfile is being used and it is a character-special file or a named pipe, the command automatically terminates when that file reaches EOF. In other words, with this option, you can terminate the command by closing the character-special file or named pipe being used as the controlfile.

### -1

Outputs one line (LF) unrelated to the data from stdin at startup. The purpose of this option is to prevent a deadlock when building a bidirectional pipe with this command from AWK or a shell script. It can also be used as a startup signal for a system that embeds this command.

### -p *n*

(Only on OSes supporting _POSIX_PRIORITY_SCHEDULING) Process priority setting. After sending the specified amount, this command enters a waiting state via the pthread_cond_wait() function, and is woken up by a pthread_cond_signal() once an amount of 1 or more is set from the controlfile. The higher the process priority, the shorter the wake-up latency is expected to be; setting *n* to 2 or 3 raises the process priority. *n* ranges over four levels from 0 to 3, and the default is 1.

Note that this option may require administrator privileges in some environments.

### -k *command*

Specifies, as a single string, an external command (together with its arguments) to run via `system(3)` every time the amount of data actually output reaches the current target, when there is no more input data, when this command is about to terminate because of the -t option, or when the controlfile receives the "t" command. Since the command is run via `system(3)`, this command's own data transfer blocks for as long as the external command is running; this is intentional. For the latter two cases, the data transfer is always fully stopped first, and this command terminates only after the external command finishes. For details, including the environment variables passed to the external command, see [Notes on the -k Hook](#notes-on-the--k-hook).

## Return Value

Returns 0 only when all the specified files were processed successfully. Returns a value other than 0 if the arguments or options are invalid, or if processing even one of the files fails.

## Examples

Output only the first 2 kibibytes (= 2048 bytes) of file a.txt, file b.txt from the standard input, and file c.txt, in that order, and then keep waiting. (If the total amount of input data is 2 kibibytes or less, the command exits.)

```sh:
$ cat b.txt | qvalve 2ki a.txt - c.txt
```

Output only the first 10 lines of this command's own help message, and then keep waiting. (If the help message happens to be 10 lines or fewer, the command exits.)

```sh:
$ qvalve -h 2>&1 | qvalve -l 10
```

Similarly, while displaying this command's help message, output it a little at a time, line by line, according to the parameters written to the named pipe "faucet".

```sh:
# Operations on terminal 1
$ mkfifo faucet
$ qvalve -h 2>&1 | qvalve -l faucet

# Operations on terminal 2
# (run in the same directory as terminal 1, after the operations on terminal 1)
$ cat > faucet
10⏎  ← outputs 10 lines
+5⏎  ← outputs 5 more lines
1E⏎  ← outputs 1 exa line (effectively, all the rest)
```

## Notes

### Notes on Command Termination

Even after finishing the output of the specified amount, this command does not exit; it waits for the input file (or, if there are several, the last one) to reach EOF. In other words, as long as the amount of input data is larger than the amount specified for this command, the command remains running. Therefore, as in the example below, if you specify the amount directly via the argument without using a controlfile, and the amount of input data exceeds that amount, this command enters a kind of deadlock state once it finishes sending the specified amount of data. (However, it can be terminated by sending a signal such as SIGINT or SIGTERM.)

```sh:
$ date | qvalve 10⏎
Thu Apr  3           ← qvalve remains running (can be terminated with [Ctrl]+[C])
```

This behavior has both advantages and disadvantages.

A command with similar behavior is the well-known head(1) command. The head(1) command exits immediately after outputting the specified amount, so it never deadlocks. Instead, it causes SIGPIPE or EOF to occur for the commands before and after it in the pipeline it is embedded in, which may result in unintended termination or a broken pipe for the remaining commands.

It is recommended that you decide whether to use the head(1) command or this command according to the circumstances of your environment.

#### Detailed Termination Conditions

To terminate this command, one of the following conditions must be met.

* Close all of this command's input files (bringing them to the EOF state).
* Send this command a signal whose default action is process termination, such as SIGINT or SIGTERM (except for SIGHUP).
* Give the "`t`" command as the parameter set via the quantity argument or the controlfile.
* After starting the command with all of the following conditions met, close the controlfile (bringing it to the EOF state).
  * The -t option is enabled.
  * A controlfile is used.
  * That controlfile is a character-special file or a named pipe.

### Idempotency of an Absolute Quantity Specification

> **[Note on a specification change made on 2026-08-27]**
> Prior to this version, qvalve implemented an absolute quantity specification as a simple overwrite of the remaining (not-yet-sent) amount. Because of this, specifying the same absolute value again after the output had already finished (i.e., once the remaining amount had reached 0) caused output to occur all over again each time, so the idempotency described below did not actually hold. This has been fixed, and an absolute quantity specification is now truly idempotent. If you have a script that relies on the old (non-idempotent) behavior, please be aware that its behavior will now be different.

An amount specified without a `+` (an absolute quantity specification) sets the target value itself — "the total amount that should be sent, in the end" — independent of how much has already been sent. Therefore, an absolute quantity specification is **idempotent**: specifying the same amount again as an absolute value, while the amount already sent is equal to or greater than that value, causes nothing further to be sent, no matter how many times you repeat it.

```sh:
# Operations on terminal 1
$ mkfifo faucet
$ seq 1 1000 | qvalve -l faucet

# Operations on terminal 2
$ cat > faucet
1⏎  ← the target becomes 1, and 1 line is sent
1⏎  ← the target is already 1, so nothing happens (no matter how many times you repeat this)
1⏎  ← same as above
10⏎ ← the target becomes 10, and the remaining 9 lines (to reach a total of 10) are sent
```

Because of this property, even if — due to, say, the output buffering behavior of the `cat` command used to write to the controlfile — the same string ends up being written to the controlfile in multiple, oddly-timed chunks (in some environments, there can be a delay between typing into `cat > controlfile` and the data actually reaching the controlfile, and several inputs can end up bundled together), the final result is unaffected as long as the specification is an absolute one.

On the other hand, an additive specification (with a leading `+`) is not idempotent, since each one adds to the target. If you want to add the same amount more than once, see ["Notes on Using a Regular File as a controlfile."](#notes-on-using-a-regular-file-as-a-controlfile)

### Notes on Using a Regular File as a controlfile

When using a regular file as the controlfile, whether a new amount (or command) has been written to the controlfile is judged by whether the string has changed. Therefore, some care is needed if you want to add the same amount again.

Note that this caveat mainly matters for an additive specification (with a leading `+`). For an absolute specification, as explained above in ["Idempotency of an Absolute Quantity Specification,"](#idempotency-of-an-absolute-quantity-specification) even if the same string fails to be detected as "written" and is thus ignored, this causes no trouble in the end.

For example, if you want to output 1 kilobyte, and then another 1 kilobyte, and so on, writing to the controlfile as follows does not convey that intent.

```sh:
$ echo +1k > controlfile⏎
$ echo +1k > controlfile⏎
$ echo +1k > controlfile⏎
$ echo +1k > controlfile⏎
$ echo +1k > controlfile⏎
    :
```

To convey it correctly, write a string that is different as a string while still meaning the same amount. An example is shown below.

```sh:
$ echo +1k > controlfile⏎
$ echo +1.0k > controlfile⏎   ← use decimal notation
$ echo +1k > controlfile⏎     ← go back to the string from two writes ago
$ echo ' +1k' > controlfile⏎  ← insert a leading space
$ echo '  +1k' > controlfile⏎ ← change the number of leading spaces
    :
```

If you include spaces or a decimal fraction, be careful that the length of the string, including the newline character, does not reach 64 or more. Strings of 64 characters or longer are ignored.

### Notes on the -k Hook

When the -k option is given, the command string you specified is run, as-is, via `system(3)` on each of the following four occasions. Because `system(3)` is synchronous, this command's own data transfer blocks for as long as the external command keeps running; this is intentional, so a slow or hanging `-k` command directly delays this command's data transfer (and, for `controlfile_closed`/`t_command` below, also delays the termination that would otherwise happen immediately). For those latter two occasions specifically, this command always stops its data transfer completely *before* running the external command, so `QVALVE_TOTAL_SIZE`/`QVALVE_SET_SIZE` (below) already reflect the final values, and this command itself terminates only after the external command finishes.

* `QVALVE_EXEC_BY=totalsize_reached`
  * The amount of data actually sent through has caught up with the current target. Since the target can be raised again via a controlfile, this can fire repeatedly over the lifetime of the command — including once right at startup, if you start this command with a controlfile whose target defaults to "0" (see [controlfile](#controlfile)), since the target is already "reached" (0 sent out of a target of 0) before any real data has flowed.
* `QVALVE_EXEC_BY=end_of_data`
  * There is no more input data to read. If you specified more than one *file*, this fires only once, after the last of them reaches EOF — not once per file.
* `QVALVE_EXEC_BY=controlfile_closed`
  * This fires only at the one specific termination trigger described in [Detailed Termination Conditions](#detailed-termination-conditions): the -t option is enabled, a controlfile is used, that controlfile is a character-special file or a named pipe, and it was closed. It does not fire when a controlfile of this type is closed while -t is *not* enabled (in that case, as described above, this command just waits for it to be reopened).
* `QVALVE_EXEC_BY=t_command`
  * The controlfile received the "`t`" command (see [Control Commands](#control-commands)). Unlike `controlfile_closed`, this fires regardless of whether the -t option is enabled.

In addition to `QVALVE_EXEC_BY`, the following environment variables are passed to the external command:

* `QVALVE_TOTAL_SIZE` — The total amount sent through so far (in bytes or lines, matching the -c/-l setting in effect).
* `QVALVE_SET_SIZE` — The current target amount.
* `QVALVE_UNIT` — `"char"` if the -c option is in effect (the default), or `"line"` if the -l option is in effect.
* `QVALVE_PID` — This command's own process ID. (Note that the `-k` command's own direct process is always a child of this command, so `$PPID` in a shell, or `getppid()` in a C program, also gives you this same value — except that, unlike an environment variable, `$PPID`/`getppid()` no longer gives you this command's PID once a grandchild process detaches itself from its parent, e.g. via `&` or `nohup`. Also note that `$PPID` itself is not among the special parameters POSIX specifies for the shell, so portable scripts should prefer this environment variable, or the POSIX-specified `ps -o ppid= -p $$`, over relying on it.)

Since `system(3)`'s child process inherits this command's own standard output as-is — the very same file descriptor this command is using to send its data through — anything the `-k` command itself writes to its standard output merges directly into the data stream being piped onward. Redirect the `-k` command's own output if you don't want that (e.g. `-k 'notify-send done >/dev/null 2>&1'`).

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
