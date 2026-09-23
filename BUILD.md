# Building AV / EMAV with Microsoft Visual C++ (MSVC)

> [!NOTE]
> MinGW support has been deprecated. Microsoft Visual C++ (MSVC via Visual Studio 2019 / 2022 or MSBuild) is the official build system for this codebase.

---

## Method 1: Using Visual Studio IDE (Recommended)

1. Open **Visual Studio 2019** or **Visual Studio 2022**.
2. Select **File > Open > Project/Solution...** and open `VS16/av.sln`.
3. Select your desired configuration (**Debug** or **Release**) and platform (**x86**).
4. Right-click the **`emav`** (or `abrav`, `putt`) project in Solution Explorer and select **Build** (or press `Ctrl + Shift + B` for the entire solution).
5. The compiled executables will be generated in `VS16/Debug/` or `VS16/Release/`.

---

## Method 2: Using Developer Command Prompt / MSBuild

1. Open **Developer Command Prompt for VS 2019/2022** (or Developer PowerShell).
2. Navigate to the `VS16` directory:
   ```cmd
   cd VS16
   ```
3. Run MSBuild to compile the solution:
   ```cmd
   msbuild av.sln /p:Configuration=Release /p:Platform=x86
   ```
4. To build only `emav`:
   ```cmd
   msbuild emav.vcxproj /p:Configuration=Release /p:Platform=x86
   ```
