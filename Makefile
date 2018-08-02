OUT_LIB = libthreadpool.a

TARGET = threadpool_test

OBJ = threadpool.o

CPP = g++ -std=c++11 -g

$(OUT_LIB) : $(OBJ)
	rm -f $(OUT_LIB)
	ar cq $(OUT_LIB) $(OBJ)
	ranlib $(OUT_LIB)

threadpool_test: threadpool_test.o $(OUT_LIB)
	$(CPP) -o $@ $< $(OUT_LIB) -lpthread

threadpool_test.o: threadpool_test.cpp threadpool.h
	$(CPP) -o $@ -c $<

threadpool.o: threadpool.cpp threadpool.h
	$(CPP) -o $@ -c $<

all: $(TARGET)

clean:
	rm -f *.o