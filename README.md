# This is the branch for the host machine code for the eeprom programmer. 
Effectively, the code here will be trying to bitstream a recently generated assembled and linked binary file to a given Arduino (most likely a mega). Note that only Linux Serial libraries are compatible so only if demand of a need arises, additional crossplatform libraries will not be added.

Compilation instructions:
Simple for now will add Makefile if necessary
{% highlight bash %}
gcc -Wall binary_eeprom_writer.c -o binary_eeprom_writer
{% endhighlight %}
