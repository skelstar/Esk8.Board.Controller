import subprocess
label = subprocess.check_output(["git", "branch"]).decode("utf-8")
i = 0
branch = 'not found'
for l in label.split("\n"):
  if '*' in l:
    l = l.replace("* ", "")
    branch = l.replace("feature/", "")
    # branch = "-D BRANCH_NAME=nrfnotble"
    break;
print("-DGIT_BRANCH_NAME='\"%s\"'" % branch)
print('-D VERSION=2.8')
print('-D VERSION_BOARD_COMPAT=2.6')
