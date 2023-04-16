# Hustle Dwarves

This is a demo solution to a problem of distributed synchronization.
It utilizes [open-mpi](https://github.com/open-mpi/ompi) to support network communication.

# The problem

There are _S_ taverns generating jobs, _K_ dwarves and _P_ portals.
In order to make a living, dwarves must take jobs from taverns.
It is up to dwarves to decide which one of them gets to take the job (taverns don't take part in the process).
After that, a dwarf occupy one of _P_ portals to do the job and come back once it is done.

# Building

To build `.pdf` description of the algorithm, run:

```bash
cd description && pdflatex algorithm.tex --shell-escape
```

# Authors

* Grzegorz Płaczek (148071)
* Łukasz Kania (148077)
