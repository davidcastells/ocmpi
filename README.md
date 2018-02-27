ocMPI is a lightweight version of mpi. 

The oc prefix stands from on-chip.

This work was used in the publication of "128-core Many-Soft-Core Processor with MPI support" available online on
https://www.researchgate.net/publication/282124163_128-core_Many-Soft-Core_Processor_with_MPI_support

I encourage you to cite our papers as:
<pre>
[*] David Castells-Rufas and Jordi Carrabina. "128-core Many-Soft-Core Processor with MPI support." 
Jornadas de Computación Reconfigurable y Aplicaciones (JCRA) (2015).

[*] Eduard Fernandez-Alonso, David Castells-Rufas, Jaume Joven, and Jordi Carrabina. "Embedding MPI 
in Many-Soft-Core Processors." In Proceedings of: High Performance Energy Efficient Embedded Systems (HIP3ES 2013) (2013).

</pre>




Building
--------

I provide a Netbeans project, but you can also compile with command line

<pre>make -f Makefile.cygwin all</pre>

this will create a build directory with the compiled examples


Use the *config.h* file to change the number of processors used in shared memory systems.

Testing 
-------

By now I build the mpiqueens2 example application to compute nqueens (with 8x8 board). Several executables are build, 
to compare the performance obtained with different buffering options (single queue, or mailbox matrix).


<pre>build/mpiqueens2_sq.exe</pre>


<pre>build/mpiqueens2_mm.exe</pre>

