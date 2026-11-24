# Runs one Qt test program many times in a row and prints the Qt Test output of every
# run that fails (test stability job T2). Used by the "<name>_repeat" tests that
# tests/CMakeLists.txt adds when PINGLYMOTION_REPEAT_TESTS is more than 0.
#
#   cmake -DEXE=<test program> -DCOUNT=<runs> -DNAME=<test name> -DOUT=<folder> -P repeat.cmake
#
# Each run writes its Qt Test output to a file (on the Windows runner the output of a
# Qt Test program does not always reach ctest) and also to standard output, so the
# run is as close as possible to a plain ctest run. Fails when any run failed.

if (NOT EXE OR NOT COUNT OR NOT NAME OR NOT OUT)
  message (FATAL_ERROR "repeat.cmake needs EXE, COUNT, NAME and OUT")
endif ()
file (MAKE_DIRECTORY "${OUT}")
set (failed 0)
foreach (i RANGE 1 ${COUNT})
  set (log "${OUT}/${NAME}_${i}.txt")
  file (REMOVE "${log}")
  string (TIMESTAMP start "%s")
  execute_process (COMMAND "${EXE}" -o "${log},txt" -o "-,txt"
                   RESULT_VARIABLE code OUTPUT_VARIABLE out ERROR_VARIABLE err)
  string (TIMESTAMP end "%s")
  math (EXPR seconds "${end} - ${start}")
  if (NOT code STREQUAL "0")
    math (EXPR failed "${failed} + 1")
    message ("===== ${NAME} run ${i} of ${COUNT}: exit code ${code} after about ${seconds} s")
    if (EXISTS "${log}")
      file (READ "${log}" text)
      message ("----- output file:\n${text}")
    else ()
      message ("----- no output file")
    endif ()
    string (LENGTH "${out}" outlen)
    message ("----- standard output (${outlen} characters):\n${out}")
    message ("----- standard error:\n${err}")
  else ()
    file (REMOVE "${log}")
  endif ()
endforeach ()
message ("${NAME}: ${COUNT} runs, ${failed} failed")
if (failed GREATER 0)
  message (FATAL_ERROR "${NAME}: ${failed} of ${COUNT} runs failed")
endif ()
