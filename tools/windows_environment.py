"""Windows child environments without development DLL search directories."""
from pathlib import PureWindowsPath


def portable_windows_env(environment):
    root=next((value for key,value in environment.items() if key.casefold()=='systemroot'),None)
    if not root or not PureWindowsPath(root).is_absolute():
        raise ValueError('A valid Windows SystemRoot is required for portable verification')
    # A copied mapping is case-sensitive even though Windows variable names
    # are not. Remove every spelling to avoid duplicate PATH entries.
    result={key:value for key,value in environment.items() if key.casefold()!='path'}
    directory=PureWindowsPath(root)
    result['PATH']=str(directory/'System32')+';'+str(directory)
    return result
