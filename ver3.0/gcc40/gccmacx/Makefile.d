#=======================================================================
#@V@:Note: File automatically generated by VIDE - 2.00/10Apr03 (g++).
# Generated 10:44:07 AM 22 Feb 2006
# This file regenerated each time you run VIDE, so save under a
#    new name if you hand edit, or it will be overwritten.
#=======================================================================

all:
	(cd ./aiocomport && make -f Makefile.d)
	(cd ./aiofile && make -f Makefile.d)
	(cd ./aiogate && make -f Makefile.d)
	(cd ./aiomsg && make -f Makefile.d)
	(cd ./aiosock && make -f Makefile.d)
	(cd ./alg && make -f Makefile.d)
	(cd ./base && make -f Makefile.d)
	(cd ./crypt && make -f Makefile.d)
	(cd ./daemon && make -f Makefile.d)
	(cd ./db && make -f Makefile.d)
	(cd ./dborcl && make -f Makefile.d)
	(cd ./dbodbc && make -f Makefile.d)
	(cd ./dbmysql && make -f Makefile.d)
	(cd ./fuzzy && make -f Makefile.d)
	(cd ./memdb && make -f Makefile.d)
	(cd ./pcre && make -f Makefile.d)
	(cd ./smart && make -f Makefile.d)
	(cd ./threadpool && make -f Makefile.d)
	(cd ./tokenizer && make -f Makefile.d)
	(cd ./xml && make -f Makefile.d)

clean:
	(cd ./aiocomport && make -f Makefile.d clean)
	(cd ./aiofile && make -f Makefile.d clean)
	(cd ./aiogate && make -f Makefile.d clean)
	(cd ./aiomsg && make -f Makefile.d clean)
	(cd ./aiosock && make -f Makefile.d clean)
	(cd ./alg && make -f Makefile.d clean)
	(cd ./base && make -f Makefile.d clean)
	(cd ./crypt && make -f Makefile.d clean)
	(cd ./daemon && make -f Makefile.d clean)
	(cd ./db && make -f Makefile.d clean)
	(cd ./dborcl && make -f Makefile.d clean)
	(cd ./dbodbc && make -f Makefile.d clean)
	(cd ./dbmysql && make -f Makefile.d clean)
	(cd ./fuzzy && make -f Makefile.d clean)
	(cd ./memdb && make -f Makefile.d clean)
	(cd ./pcre && make -f Makefile.d clean)
	(cd ./smart && make -f Makefile.d clean)
	(cd ./threadpool && make -f Makefile.d clean)
	(cd ./tokenizer && make -f Makefile.d clean)
	(cd ./xml && make -f Makefile.d clean)
	rm -rf ../output_gccmacx/debug/*
