all: build_debug build_debug_asan build_release
all_ninja: build_debug_ninja build_debug_ninja_asan build_release_ninja

debug_build_dir = debug_BUILD
debug_build_dir_target = $(debug_build_dir)-$(wildcard $(debug_build_dir))
debug_build_dir_present = $(debug_build_dir)-$(debug_build_dir)
debug_build_dir_absent = $(debug_build_dir)-
debug_ninja_build_dir = debug_ninja_BUILD
debug_ninja_build_dir_target = $(debug_ninja_build_dir)-$(wildcard $(debug_ninja_build_dir))
debug_ninja_build_dir_present = $(debug_ninja_build_dir)-$(debug_ninja_build_dir)
debug_ninja_build_dir_absent = $(debug_ninja_build_dir)-
release_build_dir = release_BUILD
release_build_dir_target = $(release_build_dir)-$(wildcard $(release_build_dir))
release_build_dir_present = $(release_build_dir)-$(release_build_dir)
release_build_dir_absent = $(release_build_dir)-
release_ninja_build_dir = release_ninja_BUILD
release_ninja_build_dir_target = $(release_ninja_build_dir)-$(wildcard $(release_ninja_build_dir))
release_ninja_build_dir_present = $(release_ninja_build_dir)-$(release_ninja_build_dir)
release_ninja_build_dir_absent = $(release_ninja_build_dir)-

debug_executable_dir = debug_EXECUTABLE
debug_executable_dir_target = $(debug_executable_dir)-$(wildcard $(debug_executable_dir))
debug_executable_dir_present = $(debug_executable_dir)-$(debug_executable_dir)
debug_executable_dir_absent = $(debug_executable_dir)-
release_executable_dir = release_EXECUTABLE
release_executable_dir_target = $(release_executable_dir)-$(wildcard $(release_executable_dir))
release_executable_dir_present = $(release_executable_dir)-$(release_executable_dir)
release_executable_dir_absent = $(release_executable_dir)-

$(release_executable_dir_present):
$(release_build_dir_present):
$(release_ninja_build_dir_present):

$(release_build_dir_absent):
	mkdir $(release_build_dir)
$(release_ninja_build_dir_absent):
	mkdir $(release_ninja_build_dir)

$(release_executable_dir_absent):
	mkdir $(release_executable_dir)
	mkdir $(release_executable_dir)_TMP

$(debug_executable_dir_present):
$(debug_build_dir_present):
$(debug_ninja_build_dir_present):

$(debug_build_dir_absent):
	mkdir $(debug_build_dir)
$(debug_ninja_build_dir_absent):
	mkdir $(debug_ninja_build_dir)
$(debug_executable_dir_absent):
	mkdir $(debug_executable_dir)
	mkdir $(debug_executable_dir)_TMP

debug_asan_build_dir = debug_asan_BUILD
debug_asan_build_dir_target = $(debug_asan_build_dir)-$(wildcard $(debug_asan_build_dir))
debug_asan_build_dir_present = $(debug_asan_build_dir)-$(debug_asan_build_dir)
debug_asan_build_dir_absent = $(debug_asan_build_dir)-
debug_ninja_asan_build_dir = debug_ninja_asan_BUILD
debug_ninja_asan_build_dir_target = $(debug_ninja_asan_build_dir)-$(wildcard $(debug_ninja_asan_build_dir))
debug_ninja_asan_build_dir_present = $(debug_ninja_asan_build_dir)-$(debug_ninja_asan_build_dir)
debug_ninja_asan_build_dir_absent = $(debug_ninja_asan_build_dir)-
debug_asan_executable_dir = debug_asan_EXECUTABLE
debug_asan_executable_dir_target = $(debug_asan_executable_dir)-$(wildcard $(debug_asan_executable_dir))
debug_asan_executable_dir_present = $(debug_asan_executable_dir)-$(debug_asan_executable_dir)
debug_asan_executable_dir_absent = $(debug_asan_executable_dir)-
$(debug_asan_executable_dir_present):
$(debug_asan_build_dir_present):
$(debug_ninja_asan_build_dir_present):
$(debug_asan_build_dir_absent):
	mkdir $(debug_asan_build_dir)
$(debug_ninja_asan_build_dir_absent):
	mkdir $(debug_ninja_asan_build_dir)
$(debug_asan_executable_dir_absent):
	mkdir $(debug_asan_executable_dir)
	mkdir $(debug_asan_executable_dir)_TMP

debug_directories: | $(debug_build_dir_target) $(debug_executable_dir_target)
debug_ninja_directories: | $(debug_ninja_build_dir_target) $(debug_executable_dir_target)

release_directories: | $(release_build_dir_target) $(release_executable_dir_target)
release_ninja_directories: | $(release_ninja_build_dir_target) $(release_executable_dir_target)

debug_asan_directories: | $(debug_asan_build_dir_target) $(debug_asan_executable_dir_target)
debug_ninja_asan_directories: | $(debug_ninja_asan_build_dir_target) $(debug_asan_executable_dir_target)

ifneq ($(CCOMPILER),)
    ifneq ($(CXXCOMPILER),)
        cmake_run_flags = -DCMAKE_C_COMPILER="$(CCOMPILER)" -DCMAKE_CXX_COMPILER="$(CXXCOMPILER)"
    endif
    ifeq ($(CXXCOMPILER),)
        cmake_run_flags = -DCMAKE_C_COMPILER="$(CCOMPILER)"
    endif
endif
ifeq ($(CCOMPILER),)
    ifneq ($(CXXCOMPILER),)
        cmake_run_flags = -DCMAKE_CXX_COMPILER="$(CXXCOMPILER)"
    endif
endif

cmake_run_flags += $(CMAKE_FLAGS)

build_debug: debug_directories
	cd ${debug_build_dir} ; mkdir EXECUTABLES; cmake -DCMAKE_BUILD_TYPE=Debug $(cmake_run_flags) -DCMAKE_C_FLAGS_DEBUG="$(CFLAGS) -g3 -O0" -DCMAKE_CXX_FLAGS_DEBUG="$(CXXFLAGS) -g3 -O0" .. ; make && if test -e EXECUTABLES ; then cd EXECUTABLES; for file in * ; do mv -v $$file ../../$(debug_executable_dir)/$$FILE ; done ; cd ..; rmdir EXECUTABLES; fi

build_debug_ninja: debug_ninja_directories
	cd ${debug_ninja_build_dir} ; mkdir EXECUTABLES; cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug $(cmake_run_flags) -DCMAKE_C_FLAGS_DEBUG="$(CFLAGS) -g3 -O0" -DCMAKE_CXX_FLAGS_DEBUG="$(CXXFLAGS) -g3 -O0" .. ; ninja && if test -e EXECUTABLES ; then cd EXECUTABLES; for file in * ; do mv -v $$file ../../$(debug_executable_dir)/$$FILE ; done ; cd ..; rmdir EXECUTABLES; fi

build_release: release_directories
	cd ${release_build_dir} ; mkdir EXECUTABLES; cmake -DCMAKE_BUILD_TYPE=Release $(cmake_run_flags) -DCMAKE_C_FLAGS_RELEASE="$(CFLAGS) -g0 -O3" -DCMAKE_CXX_FLAGS_RELEASE="$(CXXFLAGS) -g0 -O3" .. ; make && if test -e EXECUTABLES ; then cd EXECUTABLES; for file in * ; do mv -v $$file ../../$(release_executable_dir)/$$FILE ; done ; cd ..; rmdir EXECUTABLES; fi

build_release_ninja: release_ninja_directories
	cd ${release_ninja_build_dir} ; mkdir EXECUTABLES; cmake -G Ninja -DCMAKE_BUILD_TYPE=Release $(cmake_run_flags) -DCMAKE_C_FLAGS_RELEASE="$(CFLAGS) -g0 -O3" -DCMAKE_CXX_FLAGS_RELEASE="$(CXXFLAGS) -g0 -O3" .. ; ninja && if test -e EXECUTABLES ; then cd EXECUTABLES; for file in * ; do mv -v $$file ../../$(release_executable_dir)/$$FILE ; done ; cd ..; rmdir EXECUTABLES; fi



asan_build_flags = -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope -fno-common
asan_run_flags = LSAN_OPTIONS=verbosity=1:log_threads=1 ASAN_OPTIONS=verbosity=1:detect_leaks=1:detect_stack_use_after_return=1:check_initialization_order=true:strict_init_order=true

build_debug_asan: debug_asan_directories
	cd ${debug_asan_build_dir} ; mkdir EXECUTABLES; cmake -DCMAKE_BUILD_TYPE=Debug $(cmake_run_flags) -DCMAKE_C_FLAGS_DEBUG="$(CFLAGS) -g3 -O0 ${asan_build_flags}" -DCMAKE_CXX_FLAGS_DEBUG="$(CXXFLAGS) -g3 -O0 ${asan_build_flags}" .. ; make && if test -e EXECUTABLES ; then cd EXECUTABLES; for file in * ; do mv -v $$file ../../$(debug_asan_executable_dir)/$$FILE ; done ; cd ..; rmdir EXECUTABLES; fi

build_debug_ninja_asan: debug_ninja_asan_directories
	cd ${debug_ninja_asan_build_dir} ; mkdir EXECUTABLES; cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug $(cmake_run_flags) -DCMAKE_C_FLAGS_DEBUG="$(CFLAGS) -g3 -O0 ${asan_build_flags}" -DCMAKE_CXX_FLAGS_DEBUG="$(CXXFLAGS) -g3 -O0 ${asan_build_flags}" .. ; ninja && if test -e EXECUTABLES ; then cd EXECUTABLES; for file in * ; do mv -v $$file ../../$(debug_asan_executable_dir)/$$FILE ; done ; cd ..; rmdir EXECUTABLES; fi

.PHONY: all

clean: clean_debug clean_release clean_debug_asan clean_ninja

clean_ninja: clean_debug_ninja clean_release_ninja clean_debug_ninja_asan

clean_debug:
	rm -rf $(debug_build_dir) $(debug_executable_dir) $(debug_executable_dir)_TMP

clean_debug_ninja:
	rm -rf $(debug_ninja_build_dir) $(debug_executable_dir) $(debug_executable_dir)_TMP

clean_release:
	rm -rf $(release_build_dir) $(release_executable_dir) $(release_executable_dir)_TMP

clean_release_ninja:
	rm -rf $(release_ninja_build_dir) $(release_executable_dir) $(release_executable_dir)_TMP

clean_debug_asan:
	rm -rf $(debug_asan_build_dir) $(debug_asan_executable_dir) $(debug_asan_executable_dir)_TMP

clean_debug_ninja_asan:
	rm -rf $(debug_ninja_asan_build_dir) $(debug_asan_executable_dir) $(debug_asan_executable_dir)_TMP

rebuild: rebuild_debug rebuild_debug_asan rebuild_release

rebuild_ninja: rebuild_debug_ninja rebuild_debug_ninja_asan rebuild_release_ninja

rebuild_debug:
	make clean_debug
	make build_debug

rebuild_debug_ninja:
	make clean_debug_ninja
	make build_debug_ninja

rebuild_debug_asan:
	make clean_debug_asan
	make build_debug_asan

rebuild_debug_ninja_asan:
	make clean_debug_ninja_asan
	make build_debug_ninja_asan

rebuild_release:
	make clean_release
	make build_release

rebuild_release_ninja:
	make clean_release_ninja
	make build_release_ninja


test: test_debug test_debug_asan test_release
test_ninja: test_debug_ninja test_debug_ninja_asan test_release_ninja
rebuild_test: rebuild_test_debug rebuild_test_debug_asan rebuild_test_release
rebuild_test_ninja: rebuild_test_debug_ninja rebuild_test_debug_ninja_asan rebuild_test_release_ninja

test_debug: build_debug
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja: build_debug_ninja
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; if [[ -d $$file ]] ; then if [[ -e $$file/Contents ]]; then echo "opening mac os application $$file" ; open $$file; else echo "$$file is a directory but $$file/Contents does not exist"; fi; else echo "opening windows/linux application $$file" ; $$file; fi ; echo "$$file returned with code $$?" ; done

rebuild_test_debug:
	make clean_debug
	make test_debug

rebuild_test_debug_ninja:
	make clean_debug_ninja
	make test_debug_ninja


test_debug_asan: build_debug_asan
	export TEST_TMPDIR=$(debug_asan_executable_dir)_TMP; for file in $(debug_asan_executable_dir)/* ; do echo "testing $$file..." ; ${asan_run_flags} $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja_asan: build_debug_ninja_asan
	export TEST_TMPDIR=$(debug_asan_executable_dir)_TMP; for file in $(debug_asan_executable_dir)/* ; do echo "testing $$file..." ; ${asan_run_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_debug_asan:
	make clean_debug_asan
	make test_debug_asan

rebuild_test_debug_ninja_asan:
	make clean_debug_ninja_asan
	make test_debug_ninja_asan

test_release: build_release
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; $$file ; echo "$$file returned with code $$?" ; done

test_release_ninja: build_release_ninja
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_release:
	make clean_release
	make test_release

rebuild_test_release_ninja:
	make clean_release_ninja
	make test_release_ninja

valgrind_flags = --leak-check=full --show-leak-kinds=all --track-origins=yes

test_valgrind: test_debug_valgrind test_release_valgrind
test_ninja_valgrind: test_debug_ninja_valgrind test_release_ninja_valgrind
rebuild_test_valgrind: rebuild_test_debug_valgrind rebuild_test_release_valgrind
rebuild_test_ninja_valgrind: rebuild_test_debug_ninja_valgrind rebuild_test_release_ninja_valgrind

test_debug_valgrind: build_debug
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; valgrind ${valgrind_flags} $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja_valgrind: build_debug_ninja
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; valgrind ${valgrind_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_debug_valgrind:
	make clean_debug
	make test_debug_valgrind

rebuild_test_debug_ninja_valgrind:
	make clean_debug_ninja
	make test_debug_ninja_valgrind

test_release_valgrind: build_release
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; valgrind ${valgrind_flags} $$file ; echo "$$file returned with code $$?" ; done

test_release_ninja_valgrind: build_release_ninja
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; valgrind ${valgrind_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_release_valgrind:
	make clean_release
	make test_release_valgrind

rebuild_test_release_ninja_valgrind:
	make clean_release_ninja
	make test_release_ninja_valgrind

gdb_flags = -ex='set confirm on' --quiet

test_gdb: test_debug_gdb test_debug_asan_gdb test_release_gdb
test_ninja_gdb: test_debug_ninja_gdb test_debug_ninja_asan_gdb test_release_ninja_gdb
rebuild_test_gdb: rebuild_test_debug_gdb rebuild_test_debug_asan_gdb rebuild_test_release_gdb
rebuild_test_ninja_gdb: rebuild_test_debug_ninja_gdb rebuild_test_debug_ninja_asan_gdb rebuild_test_ninja_release_gdb

test_debug_gdb: build_debug
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; gdb ${gdb_flags} $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja_gdb: build_debug_ninja
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; gdb ${gdb_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_debug_gdb:
	make clean_debug
	make test_debug_gdb

rebuild_test_debug_ninja_gdb:
	make clean_debug_ninja
	make test_debug_ninja_gdb

test_debug_asan_gdb: build_debug_asan
	export TEST_TMPDIR=$(debug_asan_executable_dir)_TMP; for file in $(debug_asan_executable_dir)/* ; do echo "testing $$file..." ; ${asan_run_flags} gdb ${gdb_flags} $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja_asan_gdb: build_debug_ninja_asan
	export TEST_TMPDIR=$(debug_asan_executable_dir)_TMP; for file in $(debug_asan_executable_dir)/* ; do echo "testing $$file..." ; ${asan_run_flags} gdb ${gdb_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_debug_asan_gdb:
	make clean_debug_asan
	make test_debug_asan_gdb

rebuild_test_debug_ninja_asan_gdb:
	make clean_debug_ninja_asan
	make test_debug_ninja_asan_gdb

test_release_gdb: build_release
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; gdb ${gdb_flags} $$file ; echo "$$file returned with code $$?" ; done

test_release_ninja_gdb: build_release_ninja
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; gdb ${gdb_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_release_gdb:
	make clean_release
	make test_release_gdb

rebuild_test_release_ninja_gdb:
	make clean_release_ninja
	make test_release_ninja_gdb

lldb_flags =

test_lldb: test_debug_lldb test_debug_asan_lldb test_release_lldb
test_ninja_lldb: test_debug_ninja_lldb test_debug_ninja_asan_lldb test_release_ninja_lldb
rebuild_test_lldb: rebuild_test_debug_lldb rebuild_test_debug_asan_lldb rebuild_test_release_lldb
rebuild_test_ninja_lldb: rebuild_test_debug_ninja_lldb rebuild_test_debug_ninja_asan_lldb rebuild_test_ninja_release_lldb

test_debug_lldb: build_debug
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; lldb ${lldb_flags} $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja_lldb: build_debug_ninja
	export TEST_TMPDIR=$(debug_executable_dir)_TMP; for file in $(debug_executable_dir)/* ; do echo "testing $$file..." ; lldb ${lldb_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_debug_lldb:
	make clean_debug
	make test_debug_lldb

rebuild_test_debug_ninja_lldb:
	make clean_debug_ninja
	make test_debug_ninja_lldb

test_debug_asan_lldb: build_debug_asan
	export TEST_TMPDIR=$(debug_asan_executable_dir)_TMP; for file in $(debug_asan_executable_dir)/* ; do echo "testing $$file..." ; ${asan_run_flags} lldb ${lldb_flags} $$file ; echo "$$file returned with code $$?" ; done

test_debug_ninja_asan_lldb: build_debug_ninja_asan
	export TEST_TMPDIR=$(debug_asan_executable_dir)_TMP; for file in $(debug_asan_executable_dir)/* ; do echo "testing $$file..." ; ${asan_run_flags} lldb ${lldb_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_debug_asan_lldb:
	make clean_debug_asan
	make test_debug_asan_lldb

rebuild_test_debug_ninja_asan_lldb:
	make clean_debug_ninja_asan
	make test_debug_ninja_asan_lldb

test_release_lldb: build_release
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; lldb ${lldb_flags} $$file ; echo "$$file returned with code $$?" ; done

test_release_ninja_lldb: build_release_ninja
	export TEST_TMPDIR=$(release_executable_dir)_TMP; for file in $(release_executable_dir)/* ; do echo "testing $$file..." ; lldb ${lldb_flags} $$file ; echo "$$file returned with code $$?" ; done

rebuild_test_release_lldb:
	make clean_release
	make test_release_lldb

rebuild_test_release_ninja_lldb:
	make clean_release_ninja
	make test_release_ninja_lldb
