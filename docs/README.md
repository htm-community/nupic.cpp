# Document generation

We use Doxygen to generate the documents. The documentation is generated from the .md files and the comments in the source code that follow the
Doxygen conventions.

Check that Doxygen, and its dependecies, are installed on you computer.
Ensure that doxygen and its dependencies can be started from the command line: That their bin directories are in the PATH environment variable.

To generate the documentation, execute doxygen in this directory. It will use the file Doxyfile for configuration.

The generated files are under docs/html. The main entry point is docs/html/index.html.
The file docs/Doxyfile lists the source directories/files that are to be processed, under key INPUT.
