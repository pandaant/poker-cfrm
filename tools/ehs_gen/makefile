CC	 = clang
CXX	 = clang++ -g

CFLAGS ?=-std=c11 -Wall -g -O3
LDLIBS ?=-c

CXXFLAGS ?= -std=c++11 -MMD -MP

INCLUDES +=-I ./include\
		   -I ../../lib/libpoker/include\
		   -I ../../lib/libecalc/include

CPP_LIBRARIES += -L ../../lib/libpoker/lib/release -lpoker\
				 -L ../../lib/libecalc/lib/release -lecalc\
				-lpthread -lboost_program_options

ifeq ($(target),debug)
	CXXFLAGS +=-O0 -Weverything -Wno-c++98-compat
else
	target = release
	CXXFLAGS +=-O3 -Wall
endif

OBJ_PATH 	= obj/$(target)/
SERVER_PATH = ../../benchmark/acpc_server/

C_FILES     = $(addprefix $(SERVER_PATH),game.c net.c rng.c)
C_FILES     += $(addprefix src/,deck.c hand_index.c)
C_OBJ_FILES = $(addprefix obj/$(target)/,$(notdir $(C_FILES:.c=.o)))

CPP_FILES 	  = $(wildcard src/*.cpp)
CPP_OBJ_FILES = $(addprefix $(OBJ_PATH),$(notdir $(CPP_FILES:.cpp=.o)))
CPP_OBJ_FILES_CORE = $(filter-out $(CPP_EXCLUDE), $(CPP_OBJ_FILES))

DEP_FILES = $(CPP_OBJ_FILES:.o=.d)

all: prepare $(C_OBJ_FILES) $(CPP_OBJ_FILES) gen_eval_table 

prepare:
	mkdir -p obj/{release,debug}

obj/$(target)/%.o: src/%.c
	$(CC) $(INCLUDES) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $< -o $@

obj/$(target)/%.o: $(SERVER_PATH)/%.c
	$(CC) $(INCLUDES) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $< -o $@


obj/$(target)/%.o: src/%.cpp 
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c -o $@ $<

gen_eval_table: $(C_OBJ_FILES) $(CPP_OBJ_FILES) 
	$(CXX) $(INCLUDES) $(C_OBJ_FILES) $(CPP_OBJ_FILES_CORE) $(CPP_LIBRARIES) -o gen_eval_table 

clean:
	rm -r -f obj gen_eval_table 
	rm -f $(DEP_FILES)

.PHONY: clean all gen_eval_table 

-include $(DEP_FILES)
