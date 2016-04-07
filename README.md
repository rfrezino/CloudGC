# CloudGC
[Delphi Garbage Collector][10]

By now, it's a research. I'm trying to understand Delphi's architeture to check the viability to create a GC for it.
Do you like greate challagens ? Let's think together.


## Steps for a Simple Garbage Collector
1. Stop the world
* Identify Roots
  * Global variables
  * Pointers
2. Mark
3. Sweep (would be nice)

[This is a link for a very important reading][11]

## Challenges to solve

1. **How to access the data's segment of the PE in run time ?**</br>
It's gonna be necessary to get all the global variables and use them as root objects for Mark step. </br>
*Links to study:</br>*
[Global memory][4]

* **How to access all the registers on Stop the World stage**</br>
It's necessary to list all the root objects.

* **Create the linked list for objects**</br>
?</br>
* **Develop the stop the world function**</br>
?</br>

## Interesting Delphi Project - PE parser </br>
[pe-image-for-delphi][5]

## Some helpful reading</br>
[Link on StackOverflow][1]</br>
[Difference btw GC and ARC: Part 1][2]</br>
[Difference btw GC and ARC: Part 2][3]</br>
[Undestanding JAVA GC][9]</br>

##Programs to visualize the PE structure </br>
[IDR - Interactive Delphi Reconstructor][6]</br>
[IDA][7]</br>
[PE.Explorer][8]</br>


[1]:http://stackoverflow.com/questions/36357588/is-there-someway-to-get-objects-linked-to-an-object-no-rtti
[2]:http://patshaughnessy.net/2013/10/24/visualizing-garbage-collection-in-ruby-and-python
[3]:http://patshaughnessy.net/2013/10/30/generational-gc-in-python-and-ruby
[4]:http://stackoverflow.com/questions/1169858/global-memory-management-in-c-in-stack-or-heap
[5]:https://github.com/vdisasm/pe-image-for-delphi
[6]:http://kpnc.org/idr32/en/index.htm
[7]:https://www.hex-rays.com/products/ida/index.shtml
[8]:http://www.heaventools.com/overview.htm
[9]:http://www.cubrid.org/blog/dev-platform/understanding-java-garbage-collection/
[10]:https://en.wikipedia.org/wiki/Garbage_collection_(computer_science)
[11]:http://www.codeproject.com/Articles/31747/Conservative-Garbage-Collector-for-C
