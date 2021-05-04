import subprocess

hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'])

print("-DGIT_COMMIT_HASH='\"%s\"'" % hash)
