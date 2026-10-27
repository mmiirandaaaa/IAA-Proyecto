GW = g++ -Wall

mdmkp: isla.o individuo.o reader.o global.o
	$(GW) -o mdmkp main.cpp isla.o individuo.o reader.o global.o

reader.o: global.o
	$(GW) -c reader.cpp reader.h

isla.o: individuo.o global.o
	$(GW) -c isla.cpp isla.h

individuo.o: global.o
	$(GW) -c individuo.cpp individuo.h

global.o:
	$(GW) -c global.cpp global.h

.PHONY: clean
clean:
	rm -rf mdmkp *.o *.h.gch