import subprocess
import os

def new_wrld(project_path : str, project_name : str, blitzen_dir: str):
        print(f"[Bilbo] Project folder '{project_name}' not found. Creating...")
        os.makedirs(project_path)
        wrld_scripts_path = os.path.join(project_path, "WrldScripts")
        os.makedirs(wrld_scripts_path)

        wrld_main_path = os.path.join(wrld_scripts_path, "wrldMain.cpp")

        # wrldMain.cpp indclude
        blitzen_include = os.path.join(blitzen_dir, "src", "Core", "BlitzenWorld", "blitzenUserInterface.h")
        blitzen_include = blitzen_include.replace("\\", "/")  # normalize for C++

        # WrldMain.cpp context
        file_content = f"""#include "{blitzen_include}"

        void WrldStart()
        {{
    
        }}
        """

        with open(wrld_main_path, 'w') as f:
            f.write(file_content)

        print(f"Created: {wrld_main_path}")

def run_build(project_name : str, project_dir : str, blitzen_dir: str):
    # CMakeLists is behind this file
    # Makes a build folder where the CMake result will be placed
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    build_dir = os.path.join(root_dir, "build")
    print("[Bilbo] Running CMake configuration...")
    os.makedirs(build_dir, exist_ok=True)

    # Base CMake command
    cmake_cmd = ["cmake", root_dir]

    # Finds project or creates if it's the first time the project loads
    # BLITZEN_START_NEW is defined for the first run
    project_path = os.path.join(project_dir, project_name)
    if not os.path.exists(project_path):
        new_wrld(project_path, project_name, blitzen_dir)
        cmake_cmd.append("-DBLITZEN_START_NEW=1 ") # CMake receives BLITZEN_START_NEW
    else:
        cmake_cmd.append("-DBLITZEN_START_NEW=0")

    cmake_cmd.append(f"-DBLITZEN_CLIENT_NAME={project_name}")
    cmake_cmd.append(f"-DBLITZEN_CLIENT_WRLD_FILEPATH={project_dir}/{project_name}/{project_name}.bwrld")
    cmake_cmd.append(f"-DBLITZEN_CLIENT_RPFMESH_DIRECTORY={project_dir}/{project_name}/WorldResources/")
    cmake_cmd.append(f"-DBLITZEN_CLIENT_WORLDMAPS_DIRECTORY={project_dir}/{project_name}/WorldMaps/")
    cmake_cmd.append(f"-DBLITZEN_CLIENT_PATH_TO_WRLD_MAIN={project_dir}/{project_name}/WrldScripts/wrldMain.cpp")
    
    # Run CMake configuration step
    result = subprocess.run(cmake_cmd, cwd=build_dir)

    if result.returncode != 0:
        print("[Bilbo] CMake configuration failed.")
        return 
    
    print("[Bilbo] CMake configuration succeeded.")

    # Step 2: CMake build
    print("[Bilbo] Starting build...")
    build_result = subprocess.run(["cmake", "--build", "."], cwd=build_dir)

    if build_result.returncode != 0:
        print("[Bilbo] Build failed.")
    else:
        print("[Bilbo] Build succeeded.")