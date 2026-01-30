
RGFW_SRCS := $(PATH_INTERNAL_C)/parts/core/rgfw/RGFW.c
GLEW_SRCS := $(PATH_INTERNAL_C)/parts/core/glew/glew.c

CORE_INCLUDE := -I$(PATH_INTERNAL_C)/parts/core/glew/include

RGFW_OBJS := $(RGFW_SRCS:.c=.o)
GLEW_OBJS := $(GLEW_SRCS:.c=.o)

RGFW_DEFS := -DRGFWDEF=
GLEW_DEFS := -DGLEW_STATIC

CORE_LIB := $(PATH_INTERNAL_C)/parts/core/core.a

$(PATH_INTERNAL_C)/parts/core/rgfw/%.o: $(PATH_INTERNAL_C)/parts/core/rgfw/%.c
	$(CC) -O3 $(CFLAGS) $(CORE_INCLUDE) $(RGFW_DEFS) -w $< -c -o $@

$(PATH_INTERNAL_C)/parts/core/glew/%.o: $(PATH_INTERNAL_C)/parts/core/glew/%.c
	$(CC) -O1 $(CFLAGS) $(CORE_INCLUDE) $(GLEW_DEFS) -w $< -c -o $@

$(CORE_LIB): $(RGFW_OBJS) $(GLEW_OBJS)
	$(AR) rcs $@ $(RGFW_OBJS) $(GLEW_OBJS)

QB_CORE_LIB := $(CORE_LIB)

CXXFLAGS += $(CORE_INCLUDE) $(RGFW_DEFS) $(GLEW_DEFS)

CLEAN_LIST += $(CORE_LIB) $(RGFW_OBJS) $(GLEW_OBJS)
