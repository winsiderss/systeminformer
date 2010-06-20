Microsoft's ntdll.lib files are supplied with the DDK, 
but are not redistributable.

The alternative used in Process Hacker is to have a 
ntdll.def file and create ntdll.lib from that using 
the LIB tool. The x86 ntdll.lib must be fixed using 
fixlib, which converts the exports from ordinal to 
name entries. The x64 ntdll.lib does not suffer from 
this problem.

The regex for swapping the export entries is below:

Replace <:b:b:b:b{.*}:b=:b{.*}>
with    <    \2 = \1>
