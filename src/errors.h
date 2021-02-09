#ifndef LURDS2_ERRORS
#define LURDS2_ERRORS

#define COMPILE_TIME_ASSERT(expr) typedef char COMP_TIME_ASSERT[(expr) ? 1 : 0];

#define DIAGNOSTIC_ERROR(expr) ShowFatalErrorThenKillProcess(expr);

char* GetLastErrorMessage();
char* GetLastErrorMessageWithPrefix(char * prefix);
void ShowFatalErrorThenKillProcess(char * message);

#endif