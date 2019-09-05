#! coding=utf-8
# Format all code files according to .clang-format
# @Author: Wenyi Tang
# @Email: wenyi.tang@intel.com

from pathlib import Path
import subprocess
import argparse

def CollectFilesFromDir(path, recurse=False):
    """Glob all files (recursively) in the given path"""

    path = Path(path).absolute().resolve()
    if not path.exists():
        return None
    files = []
    for postfix in ('*.h', '*.hpp', '*.cc', '*.cpp', '*.c', '*.cxx'):
      if recurse:
          files += list(path.rglob(postfix))
      else:
          files += list(path.glob(postfix))
    return files


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('dir')
    parser.add_argument('--formatter', type=str, default=None)
    parser.add_argument('--root', type=str, default=None)
    param = parser.parse_args()

    files = CollectFilesFromDir(param.dir, True)
    for f in files:
        cmd = f"{param.formatter} -style=file -i {str(f)}"
        subprocess.call(cmd, stdout=None, shell=True)
