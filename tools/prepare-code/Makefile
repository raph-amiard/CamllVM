#===- ./Makefile -------------------------------------------*- Makefile -*--===#

LEVEL := ../..

main_target=$(LIBDIR)/PrepareCode.so

CFLAGS+=-fPIC -I$(LEVEL)/lib
CXXFLAGS+=-fPIC -I$(LEVEL)/lib

OBJ=$(patsubst %,$(LEVEL)/lib/.build/%.bc, \
			$(PROJECT_NAME)-names util  \
	)

include $(LEVEL)/Makefile.rules

$(LIBDIR)/PrepareCode.so: $(OBJ)
	$(Echo) Linking $@
	$(Verb) mkdir -p $(LIBDIR) && clang++ -o $@ -fno-common -Wl,-flat_namespace -Wl,-undefined,suppress -shared -fPIC $^
