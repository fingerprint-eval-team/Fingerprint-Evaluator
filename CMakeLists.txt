cmake_minimum_required(VERSION 3.20)
project(FPEval LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 정적 런타임(/MT): 시험 PC에 VC++ 재배포 패키지가 없어도 실행되도록
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_options(/utf-8 /W4 /permissive- /EHsc /MP)
add_compile_definitions(UNICODE _UNICODE NOMINMAX _CRT_SECURE_NO_WARNINGS)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(FP_BITS 64)
  set(FP_ARCH x64)
else()
  set(FP_BITS 32)
  set(FP_ARCH x86)
endif()

# 모든 산출물을 bin/ 아래에 모은다 ($<0:> 는 Release/Debug 하위 폴더 생성을 막는 트릭)
set(FP_BIN ${CMAKE_SOURCE_DIR}/bin)
function(fp_output target dir)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${dir}/$<0:>"
    LIBRARY_OUTPUT_DIRECTORY "${dir}/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>")
endfunction()

# ---------- 공용 ----------
add_library(FPCommon STATIC
  src/common/PathUtil.cpp)
target_include_directories(FPCommon PUBLIC src/common)
target_link_libraries(FPCommon PUBLIC shell32 ole32)  # SHGetKnownFolderPath, CoTaskMemFree

# ---------- Worker (x86, x64 각각) ----------
add_executable(FPWorker
  src/worker/main.cpp
  src/worker/CommandExecutor.cpp)
target_link_libraries(FPWorker PRIVATE FPCommon)
set_target_properties(FPWorker PROPERTIES OUTPUT_NAME FPWorker${FP_BITS})
fp_output(FPWorker ${FP_BIN})

# ---------- 우리 팀 지문인식 DLL ----------
add_library(FPAlgo SHARED
  src/algo/BioApi.cpp
  src/algo/DllMain.cpp
  src/algo/FPAlgo.def
  src/algo/image/Image.cpp
  src/algo/enhance/Enhance.cpp
  src/algo/minutiae/Minutiae.cpp
  src/algo/templ/Template.cpp
  src/algo/match/Matcher.cpp)
target_include_directories(FPAlgo PRIVATE src/algo)
fp_output(FPAlgo ${FP_BIN}/plugins/${FP_ARCH})

# ---------- 시험용 DLL ----------
foreach(kind Dummy_OK Bad_Crash Bad_Timeout Bad_Score Bad_Size Bad_Random)
  add_library(${kind} SHARED src/testdlls/TestDll.cpp src/testdlls/TestDll.def)
  string(TOUPPER ${kind} KIND_UPPER)
  target_compile_definitions(${kind} PRIVATE TESTDLL_${KIND_UPPER})
  fp_output(${kind} ${FP_BIN}/plugins/${FP_ARCH})
endforeach()

# ---------- 평가 프로그램 (x64 전용) ----------
if(FP_BITS EQUAL 64)
  add_library(FPEvalCore STATIC
    src/core/config/Json.cpp
    src/core/config/EvalConfig.cpp
    src/core/db/TiffReader.cpp
    src/core/db/FingerprintDB.cpp
    src/core/plan/Rules.cpp
    src/core/plan/EvalPlan.cpp
    src/core/exec/WorkerProcess.cpp
    src/core/exec/EvalScheduler.cpp
    src/core/result/ResultStore.cpp
    src/core/metrics/MetricsCalculator.cpp
    src/core/chart/SvgCanvas.cpp
    src/core/chart/EmfCanvas.cpp
    src/core/chart/ChartRenderer.cpp
    src/core/report/ZipWriter.cpp
    src/core/report/HtmlReportWriter.cpp
    src/core/report/DocxReportWriter.cpp
    src/core/Evaluator.cpp)
  target_include_directories(FPEvalCore PUBLIC src/core)
  target_link_libraries(FPEvalCore PUBLIC FPCommon gdi32)

  add_executable(FPEvalCli src/cli/main.cpp)
  target_link_libraries(FPEvalCli PRIVATE FPEvalCore)
  fp_output(FPEvalCli ${FP_BIN})

  # MFC GUI (src/gui/CMakeLists.txt)
  add_subdirectory(src/gui)

  # 알고리즘 튜닝용 중간 영상 덤프 도구 (제출물 아님)
  add_executable(AlgoDebug tools/AlgoDebug.cpp
    src/algo/image/Image.cpp src/algo/enhance/Enhance.cpp src/algo/minutiae/Minutiae.cpp)
  target_include_directories(AlgoDebug PRIVATE src/algo)
  target_link_libraries(AlgoDebug PRIVATE FPEvalCore)
  fp_output(AlgoDebug ${CMAKE_BINARY_DIR}/tools)

  add_executable(CoreTests tests/CoreTests.cpp)
  target_link_libraries(CoreTests PRIVATE FPEvalCore)
  fp_output(CoreTests ${CMAKE_BINARY_DIR}/tests)
endif()
