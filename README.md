# lmedit
Reads and interprets MIPS R2K load modules. Allows for binary editing and viewing of</br>.
both load and object modules.

# Commands
quit: quits the program</br>
size: prints the size in bytes of the current section</br>
write: writes out the file with changes</br>
section [name]: switches section to specified [name]</br>
A[,N][:T][=V]: examine/edit command</br>
    - A: the address within the current section (hex or decimal)</br>
    - N: the count</br>
    - T: type (b for byte, h for halfword, w for word)</br>
    - V: the replacement value</br>

# Usage
lmedit [module.obj/out]</br>
