# Standard defines:
CC  	=	g++
LD  	=	g++
CCOMP	=	gcc
oDir	=	../../obj_gcc/release/dborcl
Bin	=	../../output_gcc/release
libDirs	=
incDirs	=	-I../../src/os/linux -I../../src -I../../src/db -I../../src/orclinc
srcDirs	=	../../src/dborcl

LD_FLAGS =	
LIBS	=	
C_FLAGS	=	-O

SRCS	=\
	$(srcDirs)/orclsrv.cpp\
	$(srcDirs)/terorcl.cpp

EXOBJS	=\
	$(oDir)/orclsrv.o\
	$(oDir)/terorcl.o

ALLOBJS	=	$(EXOBJS)
ALLBIN	=	$(Bin)/libdborcl.a
ALLTGT	=	$(Bin)/libdborcl.a

# User defines:

#@# Targets follow ---------------------------------

all:	pre\
	$(ALLTGT)

pre: 	
	mkdir -p $(oDir)
	mkdir -p $(Bin)

clean:
	rm -f $(ALLOBJS)
	rm -f $(ALLBIN)n

#@# User Targets follow ---------------------------------


#@# Dependency rules follow -----------------------------

$(Bin)/libdborcl.a: $(EXOBJS)
	rm -f $(Bin)/libdborcl.a
	ar cr $(Bin)/libdborcl.a $(EXOBJS)
	ranlib $(Bin)/libdborcl.a

$(oDir)/orclsrv.o: $(srcDirs)/orclsrv.cpp
	$(CC) $(C_FLAGS) $(incDirs) -c -o $@ $<

$(oDir)/terorcl.o: $(srcDirs)/terorcl.cpp
	$(CC) $(C_FLAGS) $(incDirs) -c -o $@ $<
