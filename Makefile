.PHONY: all clean 3ds

all: 3ds

3ds:
	@$(MAKE) -C native-3ds

clean:
	@$(MAKE) -C native-3ds clean

