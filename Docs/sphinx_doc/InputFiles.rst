Running
-------

The input file specified on the command line is a free-format text file, one entry per row, that specifies input data processed by the AMReX ``ParmParse`` module.

This file needs to be specified along with the executable as an ``argv`` option, for example:


::

    mpirun -np 64 ./ROMSX3d.xxx.yyy.ex inputs

Also, any entry that can be specified in the inputs file can also be specified on the command line; values specified on the command line override values in the inputs file, e.g.:

::

    mpirun -np 64 ./ROMSX3d.gnu.DEBUG.MPI.ex inputs amr.restart=chk0030 romsx.use_gravity=true

See :ref:`sec:Inputs` for details on run-time options that can be specified.
