all: master slave readme

slave: slave.o
	gcc -g -o slave slave.o

master: master.o
	gcc -g -o master master.o
	
master.o: master.c
	gcc -g -c master.c

slave.o: slave.c 
	gcc -g -c slave.c
	
clean: remove

remove:
	rm *.o master slave *.out

clear: 
	clear
	
success: 
	$(info SUCCESS)
	
readme:
	cat README
