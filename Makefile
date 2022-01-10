T=demo.exe
CXX=clang++
CXXFLAG= -Xclang -flto-visibility-public-std \
		 -Wno-invalid-source-encoding \
		 -Wno-microsoft-template \
		 -Wno-switch \
		 -Wno-braced-scalar-init \
		 -std=c++17 -g \
		 -D_CRT_SECURE_NO_WARNINGS \
		 -DDBG_MACRO_NO_WARNING \
#		 -DDBG_MACRO_DISABLE \

OBJ=bnf.obj \
	demo.obj \
	llua.obj \

X=$(shell time /T)
SHELL=cmd.exe
BASEHEADER=bnf.h \
#	llua.h

$(T) : $(BASEHEADER)

$(T) : $(OBJ)
	@echo [demo.exe]
	$(CXX) $(CXXFLAG) -o $(T) $(OBJ)

%.obj : %.cpp $(BASEHEADER)
	@echo [$@]
	@rem @echo $^
	@rem @echo $*
	$(CXX) $(CXXFLAG) -c $< -o $@

clean:
	del $(OBJ) $(T)

test:
	@echo $(X)
	@echo $(SHELL)

.PHONY: clean test
