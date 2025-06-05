# Simple C Program to Set CPU Performance Bias

This program is a simple C program to set the CPU performance bias for CPUs on a linux system. The bias is a value between 1 and 15 that is used to adjust the performance or power efficiency of the CPU. 
The lower bias value means tuning for performance (`1`), while the higher bias means tuning for power efficiency (`15`).

## Usage

The program requires root privileges to run.
To use the program, simply run the following command:

```
./setperfbias <value>
```

Where `<value>` is a number between 1 and 15. The program will set the performance bias for all CPU cores on the system to the specified value.

## Example

To set the performance bias to 7, run the following command:

```
./setperfbias 7
```

This will set the performance bias for all CPU cores on the system to 7.

## Future Improvements

- Add support for setting the performance bias for specific CPU cores.