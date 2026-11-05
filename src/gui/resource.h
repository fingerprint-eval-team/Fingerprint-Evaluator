// FPEval.rc 리소스 ID
#pragma once

// 대화상자
#define IDD_FPEVAL        100
#define IDD_PAGE_TARGET   101
#define IDD_PAGE_RULES    102
#define IDD_PAGE_RUN      103
#define IDD_PAGE_REPORT   104
#define IDR_MAINFRAME     128

// 탭 1: 대상
#define IDC_DLL           1001
#define IDC_DLL_BROWSE    1002
#define IDC_DLL_INFO      1003
#define IDC_DB            1004
#define IDC_DB_BROWSE     1005
#define IDC_DB_INFO       1006
#define IDC_ITYPE         1007
#define IDC_TPL           1008
#define IDC_TPL_BROWSE    1009
#define IDC_OUT           1010
#define IDC_OUT_BROWSE    1011
#define IDC_REPDIR        1012
#define IDC_REPDIR_BROWSE 1013
#define IDC_LOGDIR        1014
#define IDC_LOGDIR_BROWSE 1015

// 탭 2: 시험 규칙
#define IDC_ENROLL        1101
#define IDC_ENROLL_IDX    1102
#define IDC_GEN           1103
#define IDC_GEN_N         1104
#define IDC_IMP           1105
#define IDC_IMP_N         1106
#define IDC_IMP_IDX       1107
#define IDC_IMP_SYM       1108
#define IDC_SEED          1109
#define IDC_SEED_NEW      1110
#define IDC_PAIRS         1111
#define IDC_PAIRS_BROWSE  1112
#define IDC_ESTIMATE      1113

// 탭 3: 실행 옵션
#define IDC_WORKERS       1201
#define IDC_TIMEOUT       1202
#define IDC_ANON          1203
#define IDC_CPU_INFO      1204

// 탭 4: 리포트 (IDC_ITEM0 ~ IDC_ITEM0+14)
#define IDC_ITEM0         1301
#define IDC_HTML          1350
#define IDC_DOCX          1351
#define IDC_EER_METHOD    1352
#define IDC_DENOM         1353

// 메인 대화상자
#define IDC_TAB           1400
#define IDC_START         1401
#define IDC_STOP          1402
#define IDC_ABORT         1403
#define IDC_RESUME        1404
#define IDC_REPORT_ONLY   1405
#define IDC_SAVE_CFG      1406
#define IDC_LOAD_CFG      1407
#define IDC_OPEN_REPORT   1408
#define IDC_OPEN_FOLDER   1409
#define IDC_PROGRESS      1410
#define IDC_STATUS        1411
#define IDC_LOG           1412

// 사용자 정의 메시지
#define WM_APP_PHASE          (WM_APP + 1)
#define WM_APP_PROGRESS       (WM_APP + 2)
#define WM_APP_LOG            (WM_APP + 3)
#define WM_APP_DONE           (WM_APP + 4)
#define WM_APP_DB_CHANGED     (WM_APP + 10)
#define WM_APP_RULES_CHANGED  (WM_APP + 11)
