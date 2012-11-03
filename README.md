delayed_init
-

A template class to prevent the compiler from default initialising objects.

This template class is presented in <a href="#Neri">[1]</a> for didactic
purposes only and [boost::optional][optional] is a better alternative. The
**union** rules in C++03 are strict and make the implementation of
[boost::optional][optional] more complex and difficult to understand. In
contrast, **delayed\_init** assumes C++11 rules and has a simpler code. See
**delayed\_init** as a draft of what [boost::optional][optional] could be if
written in C++11.

Files
-

* **delayed\_init.h** The definition of **delayed\_init**.
* **delayed\_init.cpp** Unit tests for **delayed\_init**.
* **makefile** : Makefile for compiling the unit tests.

References
-

<a id="Neri">[1]</a> Cassio Neri, *Complex logic in the member initialiser
list*, to appear in [Overload](http://accu.org/index.php/journals/c78).

[optional]: http://www.boost.org/doc/libs/1_51_0/libs/optional/doc/html/index.html "boost::optional"
