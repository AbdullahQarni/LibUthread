# REF: Makefile_v3.0, "Makefile.pdf"
# Target library
lib := libuthread.a
objs := queue.o uthread.o context.o preempt.o

CC := gcc
FLAGS := -Wall -Werror -Wextra -MMD

ifneq ($(SHOW_CMD), 1)
DIS = @
endif

all: $(lib)
deps := $(patsubst %.o, %.d, $(objs))
-include $(deps)

$(lib): $(objs)
	@echo "Built library $@"
	$(DIS)ar rcs $(lib) $^

%.o: %.c
	@echo "Compiled $@."
	$(DIS)$(CC) $(FLAGS) -c -o $@ $<

clean:
	@echo "Restored original directory state."
	$(DIS)rm -f $(lib) $(objs) $(deps)
