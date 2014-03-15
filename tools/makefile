
default:    dircomp2.c
	gcc -O6 -mpentium -Wall -s dircomp2.c -o dircomp2.exe

asm:
	gcc -O6 -mpentium -Wall -g dircomp2.c -o tmp.exe
	objdump  -dS tmp.exe --debugging --no-show-raw-insn > dircomp2.s
	del tmp.exe

h2d:	 h2d.c
	gcc -O6 -mpentium -Wall -s h2d.c -o h2d.exe

dckconv:     dckconv.c
	gcc -O6 -mpentium -Wall -s dckconv.c -o dckconv.exe

dckconv2:     dckconv2.c
	gcc -O6 -mpentium -Wall -s dckconv2.c -o dckconv2.exe

dckcomp:     dckcomp.c
	gcc -O6 -mpentium -Wall -s dckcomp.c -o dckcomp.exe

convert:    convert.c
	gcc -O6 -mpentium -Wall -s convert.c -o convert.exe
	
