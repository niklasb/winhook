### Usage

1. Install [Windows SDK 7.1](http://www.microsoft.com/en-us/download/details.aspx?id=8279)
   or any other distribution that contains the MSVC compiler (cl).
2. Open SDK Console (e.g. `Start -> All Program -> Microsoft Windows SDK 7.1 -> Windows SDK 7.1 Command Prompt`).
3. Type `setenv /x86 /debug`.
4. Use the `build` script to build the `injector.exe` and `cheat.dll`.
5. Use `injector <target_exe_name> cheat.dll` to inject the DLL into the target process with the given EXE name.

Usually the DLL will unload itself whenever it loses connection to the injector or when the `exit` command is used.
In case this should not work in some cases, you can use `injector -u <target_exe_name> cheat.dll` to force the unload manually.