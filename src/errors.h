/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_ERRORS
#define LURDS2_ERRORS

#define COMPILE_TIME_ASSERT(expr) typedef char COMP_TIME_ASSERT[(expr) ? 1 : 0];

#define BUILD_ASSERT(cond) \
	(void) sizeof(char [1 - 2*!(cond)]);

#define BUILD_ASSERT_SIZE(expr, size) \
  (void) sizeof(char[1 - 50 * ((size) - (expr))]); \
  (void) sizeof(char[1 - 50 * ((expr) - (size))]);

#define DIAGNOSTIC_ERROR(expr) ShowDiagnosticError(__FILE__, __FUNCTION__, __LINE__, (expr));
#define DIAGNOSTIC_ERROR2(e1, e2) ShowDiagnosticError2(__FILE__, __FUNCTION__, __LINE__, (e1), (e2));
#define DIAGNOSTIC_ERROR3(e1, e2, e3) ShowDiagnosticError3(__FILE__, __FUNCTION__, __LINE__, (e1), (e2), (e3));
#define DIAGNOSTIC_ERROR4(e1, e2, e3, d4) ShowDiagnosticError4(__FILE__, __FUNCTION__, __LINE__, (e1), (e2), (e3), (e4));

#define FATAL_ERROR(expr) ShowFatalErrorThenKillProcess(__FILE__, __FUNCTION__, __LINE__, (expr));
#define FATAL_ERROR2(e1, e2) ShowFatalErrorThenKillProcess2(__FILE__, __FUNCTION__, __LINE__, (e1), (e2));
#define FATAL_ERROR3(e1, e2, e3) ShowFatalErrorThenKillProcess3(__FILE__, __FUNCTION__, __LINE__, (e1), (e2), (e3));
#define FATAL_ERROR4(e1, e2, e3, e4) ShowFatalErrorThenKillProcess4(__FILE__, __FUNCTION__, __LINE__, (e1), (e2), (e3), (e4));

char* GetLastErrorMessage();
void ShowFatalErrorThenKillProcess(const char* file, const char* function, int line, const char* message);
void ShowFatalErrorThenKillProcess2(const char* file, const char* function, int line, const char* message, const char* message2);
void ShowFatalErrorThenKillProcess3(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3);
void ShowFatalErrorThenKillProcess4(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3, const char* message4);
void ShowDiagnosticError(const char* file, const char* function, int line, const char* message);
void ShowDiagnosticError2(const char* file, const char* function, int line, const char* message, const char* message2);
void ShowDiagnosticError3(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3);
void ShowDiagnosticError4(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3, const char* message4);

#endif