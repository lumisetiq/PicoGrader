# PicoGrader
A stupid pico program to grade grades while abusing the pico's second core and memory banks.

Version 1.0 Release

# What is PicoGrader?
PicoGrader is a stupid little 240-line program I made in the Arduino IDE as a 12-hour project. It takes a bunch of grades, and grades them. Duh.
It uses the pico's multicore RP2040 processor to "do it faster" while being slower in the end. But, now you can say you grade students' work with "multicore acceleration"!

## Commands
### The `w` (write) command
`w` is literally write. When you want to add a grade to grade, you type `w [grade]`, and it adds it.
### The `r` (read) command.
`r` is also literally read. When you want to read what it has stored, you can use this command.
### The `c` (clear) command.
This command has two modes. `c [index]` will clear a specific grade out of the "database", and `c` will do an atomic clear and whipe everything.
Use this to delete/remove grades, or enter new grades all together with its atomic mode.
### Entering multiple commands
It can do multiple commands at once, using the `;` character is how you split them.
For example: `w42;w88;w12;w95;w33;w71;w5;w100;w64;w27;w19;w54;e`

# What you can expect
Honestly not much, I was making this as mostly a joke and because I was bored. It's probably not very stable, hasn't been tested much, but I'm open to you reporting issues and I may *or may not* actually go fix them but don't count on it.

# The story behind this torture
I was assigned in a class to make a program that takes a number, like 66, and assigns a letter, like D. Simply take a grade and give it a letter. So my AuDHD brain decided to do this instead. This is probably the most over-engineered thing I have ever done, at least as of Feb 19th 2026.
