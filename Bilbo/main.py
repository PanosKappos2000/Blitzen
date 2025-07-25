import sys
from bilbo import run_build

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python main.py <ProjectName>")
        sys.exit(1)
    else: # else for clarity because I find it hard to read python code
        project_dir = "C:/Dev/BlitzenProjects"
        project_name = sys.argv[1]
        run_build(project_name, project_dir)
