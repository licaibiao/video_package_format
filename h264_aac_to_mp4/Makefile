#********define shared lib short name**********

APPNAME=test
EXECUTABLE := wisdom_hi3520_avi2mp4
SONAME=lib$(EXECUTABLE).so

#build for arm
ARCH=arm

INC := -I ./include/avilib
INC += -I ./include/mediaconvert
INC += -I ./include/mp4file
INC += -I ./include/mp4lib

PROJECT_SRC := $(wildcard ./avilib/*.cpp)
PROJECT_SRC += $(wildcard ./*.cpp)
PROJECT_SRC += $(wildcard ./mediaconvert/*.cpp)
PROJECT_SRC += $(wildcard ./mp4file/*.cpp)
PROJECT_SRC += $(wildcard ./mp4lib/*.cpp)

ifeq ($(ARCH),arm)
    COMPILE_CROSS:= arm-hisiv300-linux-
else
    COMPILE_CROSS  :=
endif

CC    := $(COMPILE_CROSS)gcc
GG    := $(COMPILE_CROSS)g++
STRIP := $(COMPILE_CROSS)strip

LDFLAGS = -shared 

OBJS = $(addprefix ./obj/, $(addsuffix .o,$(basename $(notdir $(PROJECT_SRC)))))

app:
	@mkdir -p obj
	@for file in $(PROJECT_SRC); do \
        OBJ=`basename $$file|sed -e 's/\.cpp/\.o/' -e 's/\.c/\.o/'`; \
		$(GG) $(CFLAGS) $(INC) -c -o obj/$$OBJ $$file; \
    done; \
	$(GG) $(CFLAGS) -o $(APPNAME) $(OBJS) 

	
install:
	@mkdir -p obj
	@for file in $(PROJECT_SRC); do \
        OBJ=`basename $$file|sed -e 's/\.cpp/\.o/' -e 's/\.c/\.o/'`; \
		$(GG) $(CFLAGS) $(INC) -fPIC -c -o obj/$$OBJ $$file; \
    done; \
	$(GG) $(LDFLAGS) -fPIC -o $(SONAME) $(OBJS) 
	$(STRIP) -s ./$(SONAME)


clean:
	@rm -rf ./obj/*.o ./obj/*.a ./libsoDir/*.so  ./*.so ./$(APPNAME) ./out/*
