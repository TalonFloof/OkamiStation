#include <backend.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#endif

char execPath[2048];

int main(int argc, const char *argv[]) {
  /*#if _WIN32
      int size = GetModuleFileNameA(NULL, execPath, 2047);
      execPath[size - 9] = '\0';
  #elif __linux__
      int size = readlink("/proc/self/exe", execPath, 2047);
      execPath[size - 9] = '\0';
  #elif __APPLE__
      int size = 2048;
      _NSGetExecutablePath(execPath, &size);
      execPath[strlen(execPath) - 9] = '\0';
  #else*/
  strcpy(execPath, "./");
  // #endif
  Backend backend = NewBackend();
  Backend_Run(&backend);
  return 0;
}