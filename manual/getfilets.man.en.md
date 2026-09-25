# GETFILETS(1)

（日本語版は[こちら](getfilets.man.ja.md)）

## Name

getfilets - get the timestamps (atime, mtime, ctime) of files

## Synopsis

```sh:
getfilets [options] file [file [...]]
```

## Description

The ls(1) command can also retrieve a file's timestamp (mtime), but it is incomplete — for instance, it cannot give sub-minute precision. Retrieving it via the stat(1) command is another option, but that has practical problems too: its output format is not standardized, and the command itself does not exist on some OSes.

This command always outputs 4 columns, `atime mtime ctime filename`, for each *file* given as an argument. See the options described below for the exact format of atime, mtime, and ctime.

## Options

### -9

Displays the atime, mtime, and ctime timestamps with nanosecond precision. The exact format is determined by the -c, -e, or -I option; see each of them.

Note that, depending on what the OS (filesystem) supports, the low-order 9 digits below a second, 6 digits below a millisecond, or 3 digits below a microsecond may be padded with zeros in the output.

### -c

Outputs atime, mtime, and ctime in calendar-time format. Specific examples are as follows.

* Without the -9 option ... *YYYYMMDDhhmmss*
* With the -9 option ... *YYYYMMDDhhmmss.ddddddddd* (*d* is a sub-second digit)

The timezone follows whatever is set for the OS. If you want to specify the timezone explicitly, set the environment variable TZ. (See the "Examples" section.)

The -c, -e, and -I options are mutually exclusive; if none of them is given, this -c option is assumed.

### -e

Outputs atime, mtime, and ctime in UNIX-time format. Specific examples are as follows.

* Without the -9 option ... *n* (*n* is the number of seconds of UNIX time)
* With the -9 option ... *n.ddddddddd* (*d* is a sub-second digit)

The -c, -e, and -I options are mutually exclusive; if none of them is given, the -c option is assumed.

### -I

Outputs atime, mtime, and ctime in extended ISO 8601 format. Specific examples are as follows.

* Without the -9 option ... *YYYY-MM-DDThh:mm:ss+hh:mm*
* With the -9 option ... *YYYY-MM-DDThh:mm:ss,ddddddddd+hh:mm*

The -c, -e, and -I options are mutually exclusive; if none of them is given, the -c option is assumed.

### -u

Sets the timezone to UTC. This is the same as setting the environment variable TZ to `UTC0`. This option affects the displayed values when the -c or -I option is given.

## Return Value

Returns 0 only when the timestamps of all the specified files were retrieved successfully. Returns non-zero if an argument or option was invalid, or if one or more files could not be processed due to a file error.

## Examples

Display the timestamps of the files /etc/crontab and /etc/fstab in calendar-time format, using the timezone currently set for the OS.

```sh:
$ getfilets /etc/crontab /etc/fstab
```

Display the timestamps of all files in the current directory in ISO 8601 format, using the PST (Pacific Standard Time, UTC-8) timezone.

```sh:
$ TZ=PST8 getfilets -I *
```

## Compliance with Standards

The source code of this command is written to conform to C99 and IEEE Std 1003.1-2008 ("POSIX.1").
