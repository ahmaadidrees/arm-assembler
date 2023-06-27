PROG = project04
OBJS = armasm.o scan.o parse.o codegen.o
HEADERS = armasm.h
ELF = elf/elf.a
CFLAGS = -g
%.o: %.c
	gcc -c ${CFLAGS} -o $@ $<
all : ${PROG}
$(ELF):
	make -C elf
${PROG} : ${OBJS} ${HEADERS} ${ELF}
	gcc ${CFLAGS} -o $@ ${OBJS} ${ELF}

clean :
	rm -rf ${PROG} ${OBJS} *~ *.dSYM *.o *.hex *.score *.actual 

