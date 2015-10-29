OBJS = test_simple_enc.o exr_interface.o
CXX = gcc
DEBUG = 
CFLAGS = -c -Wall -I/usr/local/include/hdrvc -I/usr/local/include/OpenEXR -I/home/gabriel/Documents/projects/hdrv_codec/release/test $(DEBUG)
LFLAGS = -L/usr/local/lib -lhdrv_encoder -lIlmThread -lIlmImf -lImath -lHalf -lstdc++ $(DEBUG)


test_simple_enc: $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS) -o test_simple_enc

test_simple_enc.o: test_simple_enc.cpp
	$(CXX) $(CFLAGS) test_simple_enc.cpp

exr_interface.o: exr_interface.cpp
	$(CXX) $(CFLAGS) exr_interface.cpp
	
clean:
	\rm *.o *~ test_simple_enc
